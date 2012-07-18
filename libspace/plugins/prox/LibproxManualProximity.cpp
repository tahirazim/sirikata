// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include "LibproxManualProximity.hpp"
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include "Protocol_Prox.pbj.hpp"
#include "Protocol_ServerProx.pbj.hpp"

#include <sirikata/core/command/Commander.hpp>
#include <json_spirit/json_spirit.h>
#include <boost/foreach.hpp>

#include <prox/manual/RTreeManualQueryHandler.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata {

#define PROXLOG(level,msg) SILOG(prox,level,msg)

using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;


LibproxManualProximity::LibproxManualProximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr)
 : LibproxProximityBase(ctx, locservice, cseg, net, aggmgr),
   mLocalHandlerPoller(mProxStrand, std::tr1::bind(&LibproxManualProximity::tickQueryHandlers, this), "LibproxManualProximity ObjectHost Handler Poll", Duration::milliseconds((int64)100))
{

    // OH Queries
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (i >= mNumQueryHandlers) {
            mLocalQueryHandler[i].handler = NULL;
            continue;
        }
        mLocalQueryHandler[i].handler = new Prox::RTreeManualQueryHandler<ObjectProxSimulationTraits>(10);
        mLocalQueryHandler[i].handler->setAggregateListener(this); // *Must* be before handler->initialize
        bool object_static_objects = (mSeparateDynamicObjects && i == OBJECT_CLASS_STATIC);
        mLocalQueryHandler[i].handler->initialize(
            mLocCache, mLocCache,
            object_static_objects, false /* not replicated */,
            std::tr1::bind(&LibproxManualProximity::handlerShouldHandleObject, this, object_static_objects, false, _1, _2, _3, _4, _5, _6)
        );
    }
}

LibproxManualProximity::~LibproxManualProximity() {
    // Make sure any remaining queries are cleared out before
    // destroying handlers
    for(int kls = 0; kls < NUM_OBJECT_CLASSES; kls++) {
        while(!mServerQueries[kls].empty())
            unregisterServerQuery( mServerQueries[kls].begin()->first );
    }
    // OH queries should be properly handled by disconnection events. Servers
    // only aren't because disconnections can't really be treated as query
    // removals since a disconnection can just be due to killing an idle
    // connection.

    // Then destroy query handlers
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        delete mLocalQueryHandler[i].handler;
    }
}

void LibproxManualProximity::start() {
    LibproxProximityBase::start();

    mContext->add(&mLocalHandlerPoller);
}

void LibproxManualProximity::poll() {

    // Get and ship OH results
    std::deque<OHResult> oh_results_copy;
    mOHResults.swap(oh_results_copy);
    mOHResultsToSend.insert(mOHResultsToSend.end(), oh_results_copy.begin(), oh_results_copy.end());

    while(!mOHResultsToSend.empty()) {
        const OHResult& msg_front = mOHResultsToSend.front();
        sendObjectHostResult(OHDP::NodeID(msg_front.first), msg_front.second);
        delete msg_front.second;
        mOHResultsToSend.pop_front();
    }


    // Get and ship Server results/commands
    std::deque<Message*> server_results_copy;
    mServerResults.swap(server_results_copy);
    mServerResultsToSend.insert(mServerResultsToSend.end(), server_results_copy.begin(), server_results_copy.end());

    bool server_sent = true;
    while(server_sent && !mServerResultsToSend.empty()) {
        Message* msg_front = mServerResultsToSend.front();
        server_sent = mProxServerMessageService->route(msg_front);
        if (server_sent)
            mServerResultsToSend.pop_front();
    }
}

void LibproxManualProximity::addQuery(UUID obj, SolidAngle sa, uint32 max_results) {
    // Ignored, this query handler only deals with ObjectHost queries
}

void LibproxManualProximity::addQuery(UUID obj, const String& params) {
    // Ignored, this query handler only deals with ObjectHost queries
}

void LibproxManualProximity::removeQuery(UUID obj) {
    // Ignored, this query handler only deals with ObjectHost queries
}


// PintoServerQuerierListener Interface
void LibproxManualProximity::onPintoServerResult(const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    mProxStrand->post(
        std::tr1::bind(&LibproxManualProximity::handleOnPintoServerResult, this, update),
        "LibproxManualProximity::handleOnPintoServerResult"
    );
}


// Note: LocationServiceListener interface is only used in order to get updates on objects which have
// registered queries, allowing us to update those queries as appropriate.  All updating of objects
// in the prox data structure happens via the LocationServiceCache
void LibproxManualProximity::localObjectRemoved(const UUID& uuid, bool agg) {
    LibproxProximityBase::localObjectRemoved(uuid, agg);
    mProxStrand->post(
        std::tr1::bind(&LibproxManualProximity::removeStaticObjectTimeout, this, ObjectReference(uuid)),
        "LibproxManualProximity::removeStaticObjectTimeout"
    );
}
void LibproxManualProximity::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
    if (mSeparateDynamicObjects)
        checkObjectClass(true, uuid, newval);
}
void LibproxManualProximity::replicaObjectRemoved(const UUID& uuid) {
    mProxStrand->post(
        std::tr1::bind(&LibproxManualProximity::removeStaticObjectTimeout, this, ObjectReference(uuid)),
        "LibproxManualProximity::removeStaticObjectTimeout"
    );
}
void LibproxManualProximity::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    if (mSeparateDynamicObjects)
        checkObjectClass(false, uuid, newval);
}


// MessageRecipient Interface

void LibproxManualProximity::receiveMessage(Message* msg) {
    assert(msg->dest_port() == SERVER_PORT_PROX);

    Sirikata::Protocol::Prox::Container prox_container;
    bool parsed = parsePBJMessage(&prox_container, msg->payload());
    if (!parsed) {
        PROXLOG(warn,"Couldn't parse message, ID=" << msg->id());
        delete msg;
        return;
    }

    ServerID source_server = msg->source_server();

    assert(!prox_container.has_query());

    if (prox_container.has_result()) {
        Sirikata::Protocol::Prox::ProximityResults prox_result_msg = prox_container.result();
        updateServerQueryResults(source_server, prox_result_msg);
    }

    if (prox_container.has_raw_query()) {
        updateServerQuery(source_server, prox_container.raw_query());
    }

    delete msg;
}

// Migration management

std::string LibproxManualProximity::migrationClientTag() {
    return "prox";
}

std::string LibproxManualProximity::generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server) {
    // There shouldn't be any object data to move since we only manage
    // ObjectHost queries
    return "";
}

void LibproxManualProximity::receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data) {
    // We should never be receiving data for migrations since we only
    // handle object host queries
    assert(data.empty());
}



// MAIN Thread -- Object host session and message management

void LibproxManualProximity::onObjectHostSession(const OHDP::NodeID& id, ObjectHostSessionPtr oh_sess) {
    // Setup listener for requests from object hosts. We should only
    // have one active substream at a time. Proximity sessions are
    // always initiated by the object host -- upon receiving a
    // connection we register the query and use the same substream to
    // transmit results.
    // We also pass through the seqNoPtr() since we need to extract it in this
    // thread, it shouldn't change, and we don't want to hold onto the session.
    oh_sess->stream()->listenSubstream(
        OBJECT_PORT_PROXIMITY,
        std::tr1::bind(
            &LibproxManualProximity::handleObjectHostSubstream, this,
            _1, _2, oh_sess->seqNoPtr()
        )
    );
}

void LibproxManualProximity::handleObjectHostSubstream(int success, OHDPSST::Stream::Ptr substream, SeqNoPtr seqno) {
    if (success != SST_IMPL_SUCCESS) return;

    PROXLOG(detailed, "New object host proximity session from " << substream->remoteEndPoint().endPoint);
    // Store this for sending data back
    addObjectHostProxStreamInfo(substream);
    // And register to read requests
    readFramesFromObjectHostStream(
        substream->remoteEndPoint().endPoint.node(),
        mProxStrand->wrap(
            std::tr1::bind(&LibproxManualProximity::handleObjectHostProxMessage, this, substream->remoteEndPoint().endPoint.node(), _1, seqno)
        )
    );
}

void LibproxManualProximity::onObjectHostSessionEnded(const OHDP::NodeID& id) {
    mProxStrand->post(
        std::tr1::bind(&LibproxManualProximity::handleObjectHostSessionEnded, this, id),
        "LibproxManualProximity::handleObjectHostSessionEnded"
    );
}


void LibproxManualProximity::sendReplicatedClientProxMessage(ReplicatedClient* client, const ServerID& evt_src_server, const String& msg) {
    assert(evt_src_server != mContext->id());

    if (evt_src_server == NullServerID) {
        mServerQuerier->updateQuery(msg);
    }
    else {
        Sirikata::Protocol::Prox::Container container;
        container.set_raw_query(msg);
        Message* msg = new Message(
            mContext->id(),
            SERVER_PORT_PROX,
            evt_src_server,
            SERVER_PORT_PROX,
            serializePBJMessage(container)
        );
        mServerResults.push(msg);
    }
}


int32 LibproxManualProximity::objectHostQueries() const {
    return mOHQueries[OBJECT_CLASS_STATIC].size();
}
int32 LibproxManualProximity::serverQueries() const {
    return mServerQueries[OBJECT_CLASS_STATIC].size();
}


// MAIN Thread: Server queries management

void LibproxManualProximity::updateServerQuery(ServerID sid, const String& raw_query) {
    mProxStrand->post(std::tr1::bind(&LibproxManualProximity::handleUpdateServerQuery, this, sid, raw_query));
}

void LibproxManualProximity::updateServerQueryResults(ServerID sid, const Sirikata::Protocol::Prox::ProximityResults& results) {
    mProxStrand->post(std::tr1::bind(&LibproxManualProximity::handleUpdateServerQueryResults, this, sid, results));
}



// PROX Thread

void LibproxManualProximity::aggregateCreated(ProxAggregator* handler, const ObjectReference& objid) {
    // We ignore aggregates built of dynamic objects, they aren't useful for
    // creating aggregate meshes
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateCreated(objid);
}

void LibproxManualProximity::aggregateChildAdded(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateChildAdded(objid, child, bnds_center, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void LibproxManualProximity::aggregateChildRemoved(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateChildRemoved(objid, child, bnds_center, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void LibproxManualProximity::aggregateBoundsUpdated(ProxAggregator* handler, const ObjectReference& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateBoundsUpdated(objid, bnds_center, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void LibproxManualProximity::aggregateDestroyed(ProxAggregator* handler, const ObjectReference& objid) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateDestroyed(objid);
}

void LibproxManualProximity::aggregateObserved(ProxAggregator* handler, const ObjectReference& objid, uint32 nobservers) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateObserved(objid, nobservers);

    // We care about nodes being observed (i.e. having cuts through them)
    // because it means that somebody local still cares about them, so we should
    // try to keep it around as long as the server lets us. The two events we
    // care about is when the number of observers becomes 0 (all observers have
    // left) or 1 (we have an observer, so we start trying to keep it around).
    //
    // Note that we don't have to post anything here currently because we should
    // be in the same strand as the parent


    // We'll get these callbacks for the local tree, but we don't need
    // to do the same thing as we do for the replicated trees -- we
    // have the entire thing here so there's nobody to notify of
    // observations in order to trigger refinement.
    if (mAggregatorToIndexMap.find(handler) == mAggregatorToIndexMap.end()) {
        assert(static_cast<ProxQueryHandler*>(handler) == mLocalQueryHandler[OBJECT_CLASS_STATIC].handler ||
            static_cast<ProxQueryHandler*>(handler) == mLocalQueryHandler[OBJECT_CLASS_DYNAMIC].handler);
        return;
    }

    // Otherwise, get the client and notify it
    NodeProxIndexID node_indexid = mAggregatorToIndexMap[handler];
    ServerID sid = node_indexid.first;
    ProxIndexID indexid = node_indexid.second;
    assert(
        mReplicatedServerDataMap.find(sid) != mReplicatedServerDataMap.end() &&
        mReplicatedServerDataMap[sid].client &&
        mReplicatedServerDataMap[sid].handlers.find(indexid) != mReplicatedServerDataMap[sid].handlers.end()
    );
    if (nobservers == 1) {
        mReplicatedServerDataMap[sid].client->queriersAreObserving(indexid, objid);
    }
    else if (nobservers == 0) {
        mReplicatedServerDataMap[sid].client->queriersStoppedObserving(indexid, objid);
    }
}


void LibproxManualProximity::tickQueryHandlers() {
    // Not really any better place to do this. We'll call this more frequently
    // than necessary by putting it here, but hopefully it doesn't matter since
    // most of the time nothing will be done.
    processExpiredStaticObjectTimeouts();

    // We need to actually swap any objects that the previous step
    // found. However, we need to be careful because just performing
    // the addObject() and removeObject() can result in incorrect
    // results: because each class is ticked separately we could do
    // the addition and removal, then tick the handlers in the wrong
    // order such that querier q which already has object o in the
    // result set gets messages [add o, remove o] when they really
    // needed to get [remove o, add o].
    //
    // To handle this, we just do all the removals, perform a tick,
    // then do all the additions. This forces this step to only
    // generate removals, then lets the next tick generate the
    // additions.

    // Local OH queries
    Time simT = mContext->simTime();
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mLocalQueryHandler[i].handler != NULL) {
            for(ObjectIDSet::iterator it = mLocalQueryHandler[i].removals.begin(); it != mLocalQueryHandler[i].removals.end(); it++)
                mLocalQueryHandler[i].handler->removeObject(*it, true);
            mLocalQueryHandler[i].removals.clear();

            mLocalQueryHandler[i].handler->tick(simT);

            for(ObjectIDSet::iterator it = mLocalQueryHandler[i].additions.begin(); it != mLocalQueryHandler[i].additions.end(); it++)
                mLocalQueryHandler[i].handler->addObject(*it);
            mLocalQueryHandler[i].additions.clear();
        }
    }

    // Queries over remote replicated data, each <server, index> has a handler
    // to be ticked
    for(ReplicatedServerDataMap::iterator remote_server_it = mReplicatedServerDataMap.begin(); remote_server_it != mReplicatedServerDataMap.end(); remote_server_it++) {
        ReplicatedIndexQueryHandlerMap& handlers = remote_server_it->second.handlers;
        for(ReplicatedIndexQueryHandlerMap::iterator handler_it = handlers.begin(); handler_it != handlers.end(); handler_it++) {
            Time simT = mContext->simTime();
            for(ObjectIDSet::iterator it = handler_it->second.removals.begin(); it != handler_it->second.removals.end(); it++)
                handler_it->second.handler->removeObject(*it, true);
            handler_it->second.removals.clear();

            handler_it->second.handler->tick(simT);

            for(ObjectIDSet::iterator it = handler_it->second.additions.begin(); it != handler_it->second.additions.end(); it++)
                handler_it->second.handler->addObject(*it);
            handler_it->second.additions.clear();
        }
    }
}


// Helpers -- shared parsing code for query update requests
namespace {
struct QueryUpdateRequest {
    QueryUpdateRequest()
     : init(false), refine(false), coarsen(false), destroy(false) {}
    bool init, refine, coarsen, destroy;
    ProxIndexID indexid;
    std::vector<ObjectReference> nodes;
};
bool parseQueryRequest(const String& data, QueryUpdateRequest* qur_out) {
    namespace json = json_spirit;
    json::Value query_params;
    if (!json::read(data, query_params)) {
        PROXLOG(error, "Error parsing object host query request: " << data);
        return false;
    }

    String action = query_params.getString("action", String(""));
    if (action.empty()) return false;
    if (action == "init") {
        qur_out->init = true;
    }
    else if (action == "refine" || action == "coarsen") {
        if (!query_params.contains("nodes") || !query_params.get("nodes").isArray() || !query_params.contains("index") || !query_params.get("index").isInt()) {
            PROXLOG(detailed, "Invalid refine or coarsen request");
            return false;
        }

        if (action == "refine")
            qur_out->refine = true;
        else
            qur_out->coarsen = true;

        qur_out->indexid = query_params.getInt("index");
        json::Array json_nodes = query_params.getArray("nodes");
        BOOST_FOREACH(json::Value& v, json_nodes) {
            if (!v.isString()) return false;
            qur_out->nodes.push_back(ObjectReference(v.getString()));
        }
    }
    else if (action == "destroy") {
        qur_out->destroy = true;
    }

    return true;
}
} // namespace

// PROX Thread -- Server-to-server and top-level pinto

void LibproxManualProximity::handleForcedDisconnection(ServerID server) {
    // Note: we're currently these because a disconnection could just
    // mean that the other node put the connection to sleep, not that
    // they don't want updates anymore. FIXME we need a better
    // approach to SS connectivity
    PROXLOG(warn, "Ignoring forced disconnection by server " << server);
}


void LibproxManualProximity::handleUpdateServerQuery(ServerID sid, const String& raw_query) {
    QueryUpdateRequest qur;
    bool parsed = parseQueryRequest(raw_query, &qur);

    if (!parsed) {
        PROXLOG(warn, "Couldn't parse query from server " << sid << ": " << raw_query);
        return;
    }

    if (qur.init) {
        PROXLOG(detailed, "Init query for " << sid);
        // Start the query off by registering it into the top-level pinto tree
        registerServerQuery(sid);
    }
    else if (qur.refine || qur.coarsen) {
        PROXLOG(detailed, "Refine query for " << sid);
        for(int kls = 0; kls < NUM_OBJECT_CLASSES; kls++) {
            if (mLocalQueryHandler[kls].handler == NULL) continue;
            ServerQueryMap::iterator query_it = mServerQueries[kls].find(sid);
            if (query_it == mServerQueries[kls].end()) continue;
            ProxQuery* q = query_it->second;
            for(uint32 i = 0; i < qur.nodes.size(); i++) {
                if (qur.refine)
                    q->refine(qur.nodes[i]);
                else
                    q->coarsen(qur.nodes[i]);
            }
        }
    }
    else if (qur.destroy) {
        unregisterServerQuery(sid);
    }
}

void LibproxManualProximity::handleUpdateServerQueryResults(ServerID sid, const Sirikata::Protocol::Prox::ProximityResults& results) {
    ReplicatedServerDataMap::iterator rsit = mReplicatedServerDataMap.find(sid);
    if (rsit == mReplicatedServerDataMap.end()) {
        PROXLOG(warn, "Got server-to-server proximity results from " << sid << " but no longer have a record of that server.");
        return;
    }

    rsit->second.client->proxUpdate(results);
}

void LibproxManualProximity::registerServerQuery(const ServerID& querier) {
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mLocalQueryHandler[i].handler == NULL) continue;

        // FIXME we need some way of specifying the basic query
        // parameters for queries (or maybe just get rid of
        // these basic properties as they aren't even required for
        // this type of query?)
        TimedMotionVector3f pos(mContext->simTime(), MotionVector3f(Vector3f(0, 0, 0), Vector3f(0, 0, 0)));
        BoundingSphere3f bounds(Vector3f(0, 0, 0), 0);
        float max_size = 0.f;
        ProxQuery* q = mLocalQueryHandler[i].handler->registerQuery(pos, bounds, max_size);
        mServerQueries[i][querier] = q;
        mInvertedServerQueries[q] = querier;
        // Set the listener last since it can trigger callbacks
        // and we want everything to be setup already
        q->setEventListener(this);
    }
}

void LibproxManualProximity::unregisterServerQuery(const ServerID& querier) {
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mLocalQueryHandler[i].handler == NULL) continue;

        ServerQueryMap::iterator it = mServerQueries[i].find(querier);
        if (it == mServerQueries[i].end()) continue;

        ProxQuery* q = it->second;
        mServerQueries[i].erase(it);
        mInvertedServerQueries.erase(q);
        delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
    }
}

SeqNoPtr LibproxManualProximity::getOrCreateSeqNoInfo(const ServerID server_id)
{
    // server_id == querier
    ServerSeqNoInfoMap::iterator proxSeqNoIt = mServerSeqNos.find(server_id);
    if (proxSeqNoIt == mServerSeqNos.end())
        proxSeqNoIt = mServerSeqNos.insert( ServerSeqNoInfoMap::value_type(server_id, SeqNoPtr(new SeqNo())) ).first;
    return proxSeqNoIt->second;
}

void LibproxManualProximity::eraseSeqNoInfo(const ServerID server_id)
{
    // server_id == querier
    ServerSeqNoInfoMap::iterator proxSeqNoIt = mServerSeqNos.find(server_id);
    if (proxSeqNoIt == mServerSeqNos.end()) return;
    mServerSeqNos.erase(proxSeqNoIt);
}


// PROX Thread -- OH queries

void LibproxManualProximity::handleObjectHostProxMessage(const OHDP::NodeID& id, const String& data, SeqNoPtr seqNo) {
    // Handle the seqno update
    if (mOHSeqNos.find(id) == mOHSeqNos.end())
        mOHSeqNos.insert( OHSeqNoInfoMap::value_type(id, seqNo) );

    Protocol::Prox::QueryRequest request;
    bool parse_success = request.ParseFromString(data);
    QueryUpdateRequest qur;
    bool parsed = parseQueryRequest(request.query_parameters(), &qur);

    if (!parsed) {
        PROXLOG(warn, "Couldn't parse query from OH " << id << ": " << request.query_parameters());
        return;
    }

    if (qur.init) {
        PROXLOG(detailed, "Init query for " << id);
        // Start the query off by registering it into the top-level pinto tree
        registerOHQueryWithServerHandlers(id, NullServerID);
    }
    else if (qur.refine || qur.coarsen) {
        PROXLOG(detailed, "Refine query for " << id);
        // Either we can find replicated remote data with this index
        if (mLocalToRemoteIndexMap.find(qur.indexid) != mLocalToRemoteIndexMap.end()) {
            NodeProxIndexID node_index_id = mLocalToRemoteIndexMap[qur.indexid];
            ServerID sid = node_index_id.first;
            ProxIndexID remote_indexid = node_index_id.second;
            if (mOHRemoteQueries.find(id) != mOHRemoteQueries.end() &&
                mOHRemoteQueries[id][sid].find(remote_indexid) != mOHRemoteQueries[id][sid].end()) {
                ProxQuery* q = mOHRemoteQueries[id][sid][remote_indexid];
                for(uint32 i = 0; i < qur.nodes.size(); i++) {
                    if (qur.refine)
                        q->refine(qur.nodes[i]);
                    else
                        q->coarsen(qur.nodes[i]);
                }
            }
        }
        else { // Or we must be dealing with our local objects
            for(int kls = 0; kls < NUM_OBJECT_CLASSES; kls++) {
                if (mLocalQueryHandler[kls].handler == NULL) continue;
                OHQueryMap::iterator query_it = mOHQueries[kls].find(id);
                if (query_it == mOHQueries[kls].end()) continue;
                ProxQuery* q = query_it->second;
                for(uint32 i = 0; i < qur.nodes.size(); i++) {
                    if (qur.refine)
                        q->refine(qur.nodes[i]);
                    else
                        q->coarsen(qur.nodes[i]);
                }
            }
        }
    }
    else if (qur.destroy) {
        destroyQuery(id);
    }
}

void LibproxManualProximity::handleObjectHostSessionEnded(const OHDP::NodeID& id) {
    destroyQuery(id);
}

void LibproxManualProximity::destroyQuery(const OHDP::NodeID& querier) {
    PROXLOG(detailed, "Destroy query for " << querier);

    // Work through all remote servers. Find which servers we have
    // queries against and remove them.
    {
        ServerToIndexToQueryMap& queries_map = mOHRemoteQueries[querier];
        for(ServerToIndexToQueryMap::iterator it = queries_map.begin(); it != queries_map.end(); it++)
            unregisterOHQueryWithServerHandlers(querier, it->first);
    }
    // Clear out the entire top-level map
    mOHRemoteQueries.erase(querier);

    // And also clear out queries against this node. This is safe even
    // if there are none registered with this node
    unregisterOHQueryWithServerHandlers(querier, mContext->id());

    eraseSeqNoInfo(querier);
    mContext->mainStrand->post(
        std::tr1::bind(&LibproxManualProximity::handleRemoveAllOHLocSubscription, this, querier),
        "LibproxManualProximity::handleRemoveAllOHLocSubscription"
    );

}


void LibproxManualProximity::handleOnPintoServerResult(const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    ReplicatedServerData& replicated_tlpinto_data = mReplicatedServerDataMap[NullServerID];
    if (!replicated_tlpinto_data.client) {
        replicated_tlpinto_data.client = ReplicatedClientPtr(
            new ReplicatedClient(
                mContext, mContext->mainStrand,
                this,
                new NopTimeSynced(), NullServerID
            )
        );
        replicated_tlpinto_data.client->start();
    }

    // Before passing this along for processing, which could result in
    // OH queries refining and hitting leaf pinto nodes, leading to
    // them wanting to register with the space server, we need to make
    // sure we're setup for those queries. We need to scan through for
    // additions of leaf nodes that require us to start replicating
    // data
    for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
        Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
        if (addition.type() != Sirikata::Protocol::Prox::ObjectAddition::Object) continue;

        ServerID sid = addition.object().asUInt32();
        if (sid == mContext->id()) continue;

        ReplicatedServerData& replicated_sid_data = mReplicatedServerDataMap[sid];
        if (!replicated_sid_data.client) {
            replicated_sid_data.client = ReplicatedClientPtr(
                new ReplicatedClient(
                    mContext, mContext->mainStrand,
                    this,
                    new NopTimeSynced(), sid
                )
            );
            replicated_sid_data.client->start();
            replicated_sid_data.client->initQuery();
        }
    }

    for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
        // FIXME cleanup? Maybe we can remove the tree, but it might
        // just be temporary movement in TL-pinto...
    }


    //  Need to run the prox update to get data in before registering queries
    replicated_tlpinto_data.client->proxUpdate(update);
}

void LibproxManualProximity::onCreatedReplicatedIndex(ReplicatedClient* client, const ServerID& evt_src_server, ProxIndexID proxid, ReplicatedLocationServiceCachePtr loccache, ServerID sid, bool dynamic_objects) {
    PROXLOG(detailed, "Replicated index created for server " << evt_src_server << " with index " << proxid);

    // We can create a handler for this index now
    assert(mReplicatedServerDataMap.find(evt_src_server) != mReplicatedServerDataMap.end());
    ReplicatedServerData& replicated_data = mReplicatedServerDataMap[evt_src_server];
    assert(replicated_data.client);

    assert(replicated_data.handlers.find(proxid) == replicated_data.handlers.end());
    ProxQueryHandlerPtr new_handler(new Prox::RTreeManualQueryHandler<ObjectProxSimulationTraits>(10));
    new_handler->setAggregateListener(this); // *Must* be before handler->initialize
    new_handler->initialize(
        loccache.get(), loccache.get(),
        !dynamic_objects /* static or not? */, true /* replicated tree*/
    );
    replicated_data.handlers[proxid] = ReplicatedIndexQueryHandler(loccache, new_handler);

    // Maintain index of which server this proxid belongs to. Note
    // that this *isn't* the proxid provided to this call -- it's the
    // *new, locally generated* prox ID for the query handler using
    // this data. This is what lets us map back to the original
    // server's data
    assert(mLocalToRemoteIndexMap.find(new_handler->handlerID()) == mLocalToRemoteIndexMap.end());
    mLocalToRemoteIndexMap[new_handler->handlerID()] = std::make_pair(evt_src_server, proxid);
    // And handler/aggregator -> index ID
    mAggregatorToIndexMap[new_handler.get()] = std::make_pair(evt_src_server, proxid);

    // Register any queries which are already on this server
    for (OHQuerierSet::iterator querier_it = replicated_data.queriers.begin(); querier_it != replicated_data.queriers.end(); querier_it++)
        registerOHQueryWithServerIndexHandler(*querier_it, evt_src_server, proxid);
}

void LibproxManualProximity::onDestroyedReplicatedIndex(ReplicatedClient* client, const ServerID& evt_src_server, ProxIndexID proxid) {
    PROXLOG(detailed, "Replicated index destroyed for server " << evt_src_server << " with index " << proxid);
    assert(mReplicatedServerDataMap.find(evt_src_server) != mReplicatedServerDataMap.end());
    ReplicatedServerData& replicated_data = mReplicatedServerDataMap[evt_src_server];
    assert(replicated_data.handlers.find(proxid) != replicated_data.handlers.end());

    // Remove any queries we've still got on this index
    for (OHQuerierSet::iterator querier_it = replicated_data.queriers.begin(); querier_it != replicated_data.queriers.end(); querier_it++)
        unregisterOHQueryWithServerIndexHandler(*querier_it, evt_src_server, proxid);

    // And clear it out of our map. shared_ptrs handle cleanup
    replicated_data.handlers.erase(proxid);
}


// Helpers for un/registering a query against an entire Server
void LibproxManualProximity::registerOHQueryWithServerHandlers(const OHDP::NodeID& querier, ServerID queried_node) {
    PROXLOG(detailed, "Registering querier " << querier << " for server " << queried_node << " indices");

    // Queries against this node are a special case as they have to go
    // into a separate set of query handlers
    if (queried_node == mContext->id()) {
        for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
            if (mLocalQueryHandler[i].handler == NULL) continue;

            // FIXME we need some way of specifying the basic query
            // parameters for OH queries (or maybe just get rid of
            // these basic properties as they aren't even required for
            // this type of query?)
            TimedMotionVector3f pos(mContext->simTime(), MotionVector3f(Vector3f(0, 0, 0), Vector3f(0, 0, 0)));
            BoundingSphere3f bounds(Vector3f(0, 0, 0), 0);
            float max_size = 0.f;
            ProxQuery* q = mLocalQueryHandler[i].handler->registerQuery(pos, bounds, max_size);
            mOHQueries[i][querier] = q;
            mInvertedOHQueries[q] = querier;
            // Set the listener last since it can trigger callbacks
            // and we want everything to be setup already
            q->setEventListener(this);
        }

        return;
    }


    ReplicatedServerData& replicated_data = mReplicatedServerDataMap[queried_node];

    // Add to the list of server queriers
    assert(replicated_data.queriers.find(querier) == replicated_data.queriers.end());
    replicated_data.queriers.insert(querier);

    // And add individual index queries for any existing indices
    for(ReplicatedIndexQueryHandlerMap::iterator handler_it = replicated_data.handlers.begin(); handler_it != replicated_data.handlers.end(); handler_it++)
        registerOHQueryWithServerIndexHandler(querier, queried_node, handler_it->first);
}

void LibproxManualProximity::unregisterOHQueryWithServerHandlers(const OHDP::NodeID& querier, ServerID queried_node) {
    PROXLOG(detailed, "Unregistering querier " << querier << " for server " << queried_node << " indices");

    // Queries against this node are a special case as they have to go
    // into a separate set of query handlers
    if (queried_node == mContext->id()) {
        for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
            if (mLocalQueryHandler[i].handler == NULL) continue;

            OHQueryMap::iterator it = mOHQueries[i].find(querier);
            if (it == mOHQueries[i].end()) continue;

            ProxQuery* q = it->second;
            mOHQueries[i].erase(it);
            mInvertedOHQueries.erase(q);
            delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
        }

        return;
    }


    assert(mReplicatedServerDataMap.find(queried_node) != mReplicatedServerDataMap.end());
    ReplicatedServerData& replicated_data = mReplicatedServerDataMap[queried_node];

    // Remove from the list of server queriers
    assert(replicated_data.queriers.find(querier) != replicated_data.queriers.end());
    replicated_data.queriers.erase(querier);

    // And remove all its individual index queries
    for(ReplicatedIndexQueryHandlerMap::iterator handler_it = replicated_data.handlers.begin(); handler_it != replicated_data.handlers.end(); handler_it++)
        unregisterOHQueryWithServerIndexHandler(querier, queried_node, handler_it->first);
}


// Helpers for un/registering a query against a single index
void LibproxManualProximity::registerOHQueryWithServerIndexHandler(const OHDP::NodeID& querier, ServerID queried_node, ProxIndexID queried_index) {
    PROXLOG(detailed, "Registering querier " << querier << " for server " << queried_node << " index " << queried_index);

    // Only for remote nodes
    assert(queried_node != mContext->id());

    // Querier info
    IndexToQueryMap& queries_by_index = mOHRemoteQueries[querier][queried_node];
    assert(queries_by_index.find(queried_index) == queries_by_index.end());

    // Query handler info
    assert(mReplicatedServerDataMap.find(queried_node) != mReplicatedServerDataMap.end());
    assert(mReplicatedServerDataMap[queried_node].handlers.find(queried_index) != mReplicatedServerDataMap[queried_node].handlers.end());
    ReplicatedIndexQueryHandler& query_handler_info = mReplicatedServerDataMap[queried_node].handlers[queried_index];

    // FIXME we need some way of specifying the basic query
    // parameters for OH queries (or maybe just get rid of
    // these basic properties as they aren't even required for
    // this type of query?)
    TimedMotionVector3f pos(mContext->simTime(), MotionVector3f(Vector3f(0, 0, 0), Vector3f(0, 0, 0)));
    BoundingSphere3f bounds(Vector3f(0, 0, 0), 0);
    float max_size = 0.f;
    ProxQuery* q = query_handler_info.handler->registerQuery(pos, bounds, max_size);
    queries_by_index[queried_index] = q;
    mInvertedOHRemoteQueries[q] = OHRemoteQueryID(querier, queried_node, queried_index);
    // Set the listener last since it can trigger callbacks
    // and we want everything to be setup already
    q->setEventListener(this);
}

void LibproxManualProximity::unregisterOHQueryWithServerIndexHandler(const OHDP::NodeID& querier, ServerID queried_node, ProxIndexID queried_index) {
    PROXLOG(detailed, "Unregistering querier " << querier << " for server " << queried_node << " index " << queried_index);

    // Only for remote nodes
    assert(queried_node != mContext->id());

    // Querier info
    IndexToQueryMap& queries_by_index = mOHRemoteQueries[querier][queried_node];
    assert(queries_by_index.find(queried_index) != queries_by_index.end());
    ProxQuery* q = queries_by_index[queried_index];

    queries_by_index.erase(queried_index);
    mInvertedOHRemoteQueries.erase(q);
    delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
}



bool LibproxManualProximity::handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const ObjectReference& obj_id, bool is_local, bool is_aggregate, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize) {
    // We just need to decide whether the query handler should handle
    // the object. We need to consider local vs. replica and static
    // vs. dynamic.  All must 'vote' for handling the object for us to
    // say it should be handled, so as soon as we find a negative
    // response we can return false.

    // First classify by local vs. replica. Only say no on a local
    // handler looking at a replica.
    if (!is_local && !is_global_handler) return false;

    // If we're not doing the static/dynamic split, then this is a non-issue
    if (!mSeparateDynamicObjects) return true;

    // If we are splitting them, check velocity against is_static_handler. The
    // value here as arbitrary, just meant to indicate such small movement that
    // the object is effectively
    bool is_static = velocityIsStatic(pos.velocity());
    if ((is_static && is_static_handler) ||
        (!is_static && !is_static_handler))
        return true;
    else
        return false;
}


void LibproxManualProximity::handleCheckObjectClassForHandlers(const ObjectReference& objid, bool is_static, ProxQueryHandlerData handlers[NUM_OBJECT_CLASSES]) {
    if ( (is_static && handlers[OBJECT_CLASS_STATIC].handler->containsObject(objid)) ||
        (!is_static && handlers[OBJECT_CLASS_DYNAMIC].handler->containsObject(objid)) )
        return;

    // Validate that the other handler has the object.
    assert(
        (is_static && handlers[OBJECT_CLASS_DYNAMIC].handler->containsObject(objid)) ||
        (!is_static && handlers[OBJECT_CLASS_STATIC].handler->containsObject(objid))
    );

    // If it wasn't in the right place, switch it.
    int swap_out = is_static ? OBJECT_CLASS_DYNAMIC : OBJECT_CLASS_STATIC;
    int swap_in = is_static ? OBJECT_CLASS_STATIC : OBJECT_CLASS_DYNAMIC;
    PROXLOG(debug, "Swapping " << objid.toString() << " from " << ObjectClassToString((ObjectClass)swap_out) << " to " << ObjectClassToString((ObjectClass)swap_in));
    handlers[swap_out].removals.insert(objid);
    handlers[swap_in].additions.insert(objid);
}

void LibproxManualProximity::trySwapHandlers(bool is_local, const ObjectReference& objid, bool is_static) {
    handleCheckObjectClassForHandlers(objid, is_static, mLocalQueryHandler);
}



SeqNoPtr LibproxManualProximity::getSeqNoInfo(const OHDP::NodeID& node)
{
    OHSeqNoInfoMap::iterator proxSeqNoIt = mOHSeqNos.find(node);
    assert (proxSeqNoIt != mOHSeqNos.end());
    return proxSeqNoIt->second;
}

void LibproxManualProximity::eraseSeqNoInfo(const OHDP::NodeID& node)
{
    OHSeqNoInfoMap::iterator proxSeqNoIt = mOHSeqNos.find(node);
    if (proxSeqNoIt == mOHSeqNos.end()) return;
    mOHSeqNos.erase(proxSeqNoIt);
}

void LibproxManualProximity::queryHasEvents(ProxQuery* query) {
    uint32 max_count = GetOptionValue<uint32>(PROX_MAX_PER_RESULT);

    // This function handles both OH queries (against remote
    // replicated trees and the local tree) and server queries
    // (against the local tree).
    enum QueryHandlerType {
        QUERY_HANDLER_TYPE_SERVER,
        QUERY_HANDLER_TYPE_OH,
    };
    QueryHandlerType qhandler_type;
    ServerID server_query_id;
    OHDP::NodeID query_id;
    bool local_query;
    ServerID object_origin_server;
    assert(mInvertedOHQueries.find(query) != mInvertedOHQueries.end() ||
        mInvertedOHRemoteQueries.find(query) != mInvertedOHRemoteQueries.end() ||
        mInvertedServerQueries.find(query) != mInvertedServerQueries.end() );
    if (mInvertedOHQueries.find(query) != mInvertedOHQueries.end()) {
        qhandler_type = QUERY_HANDLER_TYPE_OH;
        server_query_id = NullServerID;
        query_id = mInvertedOHQueries[query];
        local_query = true;
        object_origin_server = mContext->id();
    }
    else if (mInvertedOHRemoteQueries.find(query) != mInvertedOHRemoteQueries.end()) {
        OHRemoteQueryID remote_query_id = mInvertedOHRemoteQueries[query];
        qhandler_type = QUERY_HANDLER_TYPE_OH;
        server_query_id = NullServerID;
        query_id = std::tr1::get<0>(remote_query_id);
        local_query = false;
        object_origin_server = std::tr1::get<1>(remote_query_id);
    }
    else {
        assert(mInvertedServerQueries.find(query) != mInvertedServerQueries.end());
        qhandler_type = QUERY_HANDLER_TYPE_SERVER;
        server_query_id = mInvertedServerQueries[query];
        query_id = OHDP::NodeID::null();
        local_query = true;
        object_origin_server = mContext->id();
    }
    String object_origin_server_str = boost::lexical_cast<String>(object_origin_server);
    SeqNoPtr seqNoPtr = (qhandler_type == QUERY_HANDLER_TYPE_SERVER ? getOrCreateSeqNoInfo(server_query_id) : getSeqNoInfo(query_id));
    ExtendedLocationServiceCache* loccache = dynamic_cast<ExtendedLocationServiceCache*>(query->handler()->locationCache());
    assert(loccache);

    QueryEventList evts;
    query->popEvents(evts);

    PROXLOG(detailed, evts.size() << " events for query " << query_id);
    while(!evts.empty()) {
        // We need to support encoding both server messages, which
        // want a Container, and object messages, which want just
        // ProximityResults. We'l always put things into a container,
        // but possibly only encode the ProximityResults if that's all
        // we need, keeping the code simpler by combining both server
        // and OH events.
        Sirikata::Protocol::Prox::Container container;
        Sirikata::Protocol::Prox::IProximityResults prox_results = container.mutable_result();

        prox_results.set_t(mContext->simTime());

        uint32 count = 0;
        while(count < max_count && !evts.empty()) {
            const ProxQueryEvent& evt = evts.front();
            Sirikata::Protocol::Prox::IProximityUpdate event_results = prox_results.add_update();

            // We always want to tag this with the unique query handler index ID
            // so the client can properly group the replicas
            Sirikata::Protocol::Prox::IIndexProperties index_props = event_results.mutable_index_properties();
            index_props.set_id(evt.indexID());

            for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
                ObjectReference oobjid = evt.additions()[aidx].id();
                UUID objid = oobjid.getAsUUID();
                if (loccache->tracking(oobjid)) { // If the cache already lost it, we can't do anything
                    count++;

                    PROXLOG(detailed, "Reporting addition of " << oobjid);

                    if (qhandler_type == QUERY_HANDLER_TYPE_OH) {
                        mContext->mainStrand->post(
                            std::tr1::bind(&LibproxManualProximity::handleAddOHLocSubscriptionWithID, this, query_id, objid, evt.indexID()),
                            "LibproxManualProximity::handleAddOHLocSubscription"
                        );
                    }
                    else if (qhandler_type == QUERY_HANDLER_TYPE_SERVER) {
                        mContext->mainStrand->post(
                            std::tr1::bind(&LibproxManualProximity::handleAddServerLocSubscriptionWithID, this, server_query_id, objid, evt.indexID(), seqNoPtr),
                            "LibproxManualProximity::handleAddServerLocSubscription"
                        );
                    }

                    Sirikata::Protocol::Prox::IObjectAddition addition = event_results.add_addition();
                    addition.set_object( objid );


                    //query_id contains the uuid of the object that is receiving
                    //the proximity message that obj_id has been added.
                    uint64 seqNo = (*seqNoPtr)++;
                    addition.set_seqno (seqNo);


                    Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
                    TimedMotionVector3f loc = loccache->location(oobjid);
                    motion.set_t(loc.updateTime());
                    motion.set_position(loc.position());
                    motion.set_velocity(loc.velocity());

                    TimedMotionQuaternion orient = loccache->orientation(oobjid);
                    Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
                    msg_orient.set_t(orient.updateTime());
                    msg_orient.set_position(orient.position());
                    msg_orient.set_velocity(orient.velocity());

                    Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = addition.mutable_aggregate_bounds();
                    AggregateBoundingInfo bnds = loccache->bounds(oobjid);
                    msg_bounds.set_center_offset(bnds.centerOffset);
                    msg_bounds.set_center_bounds_radius(bnds.centerBoundsRadius);
                    msg_bounds.set_max_object_size(bnds.maxObjectRadius);

                    String mesh = loccache->mesh(oobjid).toString();
                    if (mesh.size() > 0)
                        addition.set_mesh(mesh);
                    const String& phy = loccache->physics(oobjid);
                    if (phy.size() > 0)
                        addition.set_physics(phy);

                    // We should either include the parent ID, or if it's empty,
                    // then this is a root and we should include basic tree
                    // properties. However, we only need to include the details
                    // if this is the first time we're seeing the root, in which
                    // case we'll get a lone addition of the root.
                    ObjectReference parentid = evt.additions()[aidx].parent();
                    if (parentid != ObjectReference::null()) {
                        addition.set_parent(parentid.getAsUUID());
                    }
                    else if (/*lone addition*/ aidx == 0 && evt.additions().size() == 1 && evt.removals().size() == 0) {
                        // NOTE: this isn't perfect, you can get this
                        // if we're adding a new root!

                        // The tree ID identifies where this tree goes in some
                        // larger structure. In our case it'll be a server ID
                        // indicating which server the objects (and tree) are
                        // replicated from or NullServerID to fit with.
                        index_props.set_index_id(object_origin_server_str);

                        // And whether it's static or not, which actually also
                        // is important in determining a "full" tree id
                        // (e.g. objects from server A that are dynamic) but
                        // which we want to keep separate and explicit so the
                        // other side can perform optimizations for static
                        // object trees
                        index_props.set_dynamic_classification(
                            query->handler()->staticOnly()
                            ? Sirikata::Protocol::Prox::IndexProperties::Static
                            : Sirikata::Protocol::Prox::IndexProperties::Dynamic
                        );
                    }
                    addition.set_type(
                        (evt.additions()[aidx].type() == ProxQueryEvent::Normal) ?
                        Sirikata::Protocol::Prox::ObjectAddition::Object :
                        Sirikata::Protocol::Prox::ObjectAddition::Aggregate
                    );

                    // If we hit a leaf node in the top-level tree, we
                    // need to push down to the next tree
                    if (object_origin_server == NullServerID && evt.additions()[aidx].type() == ProxQueryEvent::Normal) {
                        assert(qhandler_type == QUERY_HANDLER_TYPE_OH);
                        // Need to unpack real ID from the UUID
                        ServerID leaf_server = (ServerID)objid.asUInt32();
                        PROXLOG(detailed, "Query " << query_id << " reached leaf top-level node " << oobjid << "(SS " << leaf_server << "), registering query against that server");
                        registerOHQueryWithServerHandlers(query_id, leaf_server);
                    }
                }
                else {
                    PROXLOG(detailed, "Ignoring object addition " << oobjid << " because it's not available in the location service cache");
                }
            }
            for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
                ObjectReference oobjid = evt.removals()[ridx].id();
                UUID objid = oobjid.getAsUUID();
                count++;

                PROXLOG(detailed, "Reporting removal of " << oobjid);

                // Clear out seqno and let main strand remove loc
                // subcription

                if (qhandler_type == QUERY_HANDLER_TYPE_OH) {
                    mContext->mainStrand->post(
                        std::tr1::bind(&LibproxManualProximity::handleRemoveOHLocSubscriptionWithID, this, query_id, objid, evt.indexID()),
                        "LibproxManualProximity::handleRemoveOHLocSubscription"
                    );
                }
                else if (qhandler_type == QUERY_HANDLER_TYPE_SERVER) {
                    mContext->mainStrand->post(
                        std::tr1::bind(&LibproxManualProximity::handleRemoveServerLocSubscriptionWithID, this, query_id, objid, evt.indexID()),
                        "LibproxManualProximity::handleRemoveServerLocSubscription"
                    );
                }

                Sirikata::Protocol::Prox::IObjectRemoval removal = event_results.add_removal();
                removal.set_object( objid );
                uint64 seqNo = (*seqNoPtr)++;
                removal.set_seqno (seqNo);
                removal.set_type(
                    (evt.removals()[ridx].permanent() == ProxQueryEvent::Permanent)
                    ? Sirikata::Protocol::Prox::ObjectRemoval::Permanent
                    : Sirikata::Protocol::Prox::ObjectRemoval::Transient
                );

                // If we're removing a leaf node in the top-level tree, we
                // need to pull up from that space server's tree
                if (object_origin_server == NullServerID &&
                    loccache->tracking(oobjid) && loccache->aggregate(oobjid))
                {
                    assert(qhandler_type == QUERY_HANDLER_TYPE_OH);
                    // Need to unpack real ID from the UUID
                    ServerID leaf_server = (ServerID)objid.asUInt32();
                    PROXLOG(detailed, "Query " << query_id << " moved cut above leaf top-level node " << oobjid << "(SS " << leaf_server << "), unregistering query against that server");
                    unregisterOHQueryWithServerHandlers(query_id, leaf_server);
                }
            }

            if (event_results.addition_size() == 0 && event_results.removal_size() == 0) {
                PROXLOG(error, "Generated update with no additions or removals, possibly because objects that were removed from the location cache");
            }

            evts.pop_front();
        }

        if (qhandler_type == QUERY_HANDLER_TYPE_OH) {
            // Note null ID's since these are OHDP messages.
            Sirikata::Protocol::Object::ObjectMessage* obj_msg = createObjectMessage(
                mContext->id(),
                UUID::null(), OBJECT_PORT_PROXIMITY,
                UUID::null(), OBJECT_PORT_PROXIMITY,
                serializePBJMessage(prox_results)
            );
            mOHResults.push( OHResult(query_id, obj_msg) );
        }
        else if (qhandler_type == QUERY_HANDLER_TYPE_SERVER) {
            Message* msg = new Message(
                mContext->id(),
                SERVER_PORT_PROX,
                server_query_id,
                SERVER_PORT_PROX,
                serializePBJMessage(container)
            );
            mServerResults.push(msg);
        }
    }
}





// Command handlers
void LibproxManualProximity::commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    // Properties
    result.put("name", "libprox-manual");
    result.put("settings.handlers", mNumQueryHandlers);
    result.put("settings.dynamic_separate", mSeparateDynamicObjects);
    if (mSeparateDynamicObjects)
        result.put("settings.static_heuristic", mMoveToStaticDelay.toString());

    // Current state

    // Properties of objects
    int32 oh_query_objects = (mNumQueryHandlers == 2 ? (mLocalQueryHandler[0].handler->numObjects() + mLocalQueryHandler[1].handler->numObjects()) : mLocalQueryHandler[0].handler->numObjects());
    result.put("objects.properties.local_count", oh_query_objects);
    result.put("objects.properties.remote_count", 0);
    result.put("objects.properties.count", oh_query_objects);
    result.put("objects.properties.max_size", mMaxObject);

    // Properties of queries
    result.put("queries.oh.count", mOHQueries[0].size());
    // Technically not thread safe, but these should be simple
    // read-only accesses.
    uint32 oh_messages = 0;
    for(ObjectHostProxStreamMap::iterator prox_stream_it = mObjectHostProxStreams.begin(); prox_stream_it != mObjectHostProxStreams.end(); prox_stream_it++)
        oh_messages += prox_stream_it->second->outstanding.size();
    result.put("queries.oh.messages", oh_messages);

    cmdr->result(cmdid, result);
}

void LibproxManualProximity::commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    // Handlers over local objects -- OH queries
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mLocalQueryHandler[i].handler != NULL) {
            String key = String("handlers.oh.") + ObjectClassToString((ObjectClass)i) + ".";
            result.put(key + "name", String("oh-queries.") + ObjectClassToString((ObjectClass)i) + "-objects");
            result.put(key + "queries", mLocalQueryHandler[i].handler->numQueries());
            result.put(key + "objects", mLocalQueryHandler[i].handler->numObjects());
            result.put(key + "nodes", mLocalQueryHandler[i].handler->numNodes());
        }
    }

    // Handlers over replicated remote trees
    for(ReplicatedServerDataMap::iterator server_data_it = mReplicatedServerDataMap.begin(); server_data_it != mReplicatedServerDataMap.end(); server_data_it++) {
        ReplicatedServerData& server_data = server_data_it->second;
        for(ReplicatedIndexQueryHandlerMap::iterator handler_it = server_data.handlers.begin(); handler_it != server_data.handlers.end(); handler_it++) {
            ProxQueryHandlerPtr handler = handler_it->second.handler;
            String key = String("handlers.oh.replicated-server-") +
                boost::lexical_cast<String>(server_data_it->first) +
                String("-index-") + boost::lexical_cast<String>(handler_it->first) + ".";
            result.put(key + "name",
                String("oh-queries.replicated-server-") +
                boost::lexical_cast<String>(server_data_it->first) +
                String("-index-") + boost::lexical_cast<String>(handler_it->first)
            );
            result.put(key + "queries", handler->numQueries());
            result.put(key + "objects", handler->numObjects());
            result.put(key + "nodes", handler->numNodes());
        }
    }


    cmdr->result(cmdid, result);
}

bool LibproxManualProximity::parseHandlerName(const String& name, ProxQueryHandler** handler_out) {
    // Should be of the form xxx-queries.yyy-objects or
    // xxx-queries.replicated-server-yyy-index-zzz, containing only 1
    // '.' in both case
    std::size_t dot_pos = name.find('.');
    if (dot_pos == String::npos || name.rfind('.') != dot_pos)
        return false;

    String handler_part = name.substr(0, dot_pos);
    if (handler_part != "oh-queries") return false;

    String class_part = name.substr(dot_pos+1);
    // Check for local query handlers
    if (class_part == "dynamic-objects") {
        *handler_out = mLocalQueryHandler[OBJECT_CLASS_DYNAMIC].handler;
        return true;
    }
    if (class_part == "static-objects") {
        *handler_out = mLocalQueryHandler[OBJECT_CLASS_STATIC].handler;
        return true;
    }

    // Otherwise, lookup the replicated tree query handlers
    // Parse the format replicated-server-yyy-index-zzz
    if (class_part.find("replicated-server-") != 0) return false;
    std::size_t index_pos = class_part.find("-index-");
    if (index_pos == String::npos) return false;
    String server_str = class_part.substr(String("replicated-server-").size(), index_pos-String("replicated-server-").size());
    ServerID server = boost::lexical_cast<ProxIndexID>(server_str);
    String index_str = class_part.substr(index_pos + String("-index-").size());
    ProxIndexID index = boost::lexical_cast<ProxIndexID>(index_str);

    if (mReplicatedServerDataMap.find(server) == mReplicatedServerDataMap.end() ||
        mReplicatedServerDataMap[server].handlers.find(index) == mReplicatedServerDataMap[server].handlers.end())
        return false;

    *handler_out = mReplicatedServerDataMap[server].handlers[index].handler.get();
    return true;
}

void LibproxManualProximity::commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    ProxQueryHandler* handler = NULL;
    if (!cmd.contains("handler") ||
        !parseHandlerName(cmd.getString("handler"), &handler))
    {
        result.put("error", "Ill-formatted request: handler not specified or invalid.");
        cmdr->result(cmdid, result);
        return;
    }


    result.put("error", "Rebuilding manual proximity processors isn't supported yet.");
    cmdr->result(cmdid, result);
}

void LibproxManualProximity::commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    ProxQueryHandler* handler = NULL;
    if (!cmd.contains("handler") ||
        !parseHandlerName(cmd.getString("handler"), &handler))
    {
        result.put("error", "Ill-formatted request: handler not specified or invalid.");
        cmdr->result(cmdid, result);
        return;
    }

    result.put( String("nodes"), Command::Array());
    Command::Array& nodes_ary = result.getArray("nodes");
    for(ProxQueryHandler::NodeIterator nit = handler->nodesBegin(); nit != handler->nodesEnd(); nit++) {
        nodes_ary.push_back( Command::Object() );
        nodes_ary.back().put("id", nit.id().toString());
        nodes_ary.back().put("parent", nit.parentId().toString());
        BoundingSphere3f bounds = nit.bounds(mContext->simTime());
        nodes_ary.back().put("bounds.center.x", bounds.center().x);
        nodes_ary.back().put("bounds.center.y", bounds.center().y);
        nodes_ary.back().put("bounds.center.z", bounds.center().z);
        nodes_ary.back().put("bounds.radius", bounds.radius());
        nodes_ary.back().put("cuts", nit.cuts());
    }

    cmdr->result(cmdid, result);
}



} // namespace Sirikata
