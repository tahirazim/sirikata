// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include "LibproxManualProximity.hpp"
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include "Protocol_Prox.pbj.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <prox/manual/RTreeManualQueryHandler.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata {

#define PROXLOG(level,msg) SILOG(prox,level,"[PROX] " << msg)

using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;


LibproxManualProximity::LibproxManualProximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr)
 : LibproxProximityBase(ctx, locservice, cseg, net, aggmgr),
   mOHQueries(),
   mOHHandlerPoller(mProxStrand, std::tr1::bind(&LibproxManualProximity::tickQueryHandler, this, mOHQueryHandler), Duration::milliseconds((int64)100))
{

    // OH Queries
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (i >= mNumQueryHandlers) {
            mOHQueryHandler[i] = NULL;
            continue;
        }
        mOHQueryHandler[i] = new Prox::RTreeManualQueryHandler<ObjectProxSimulationTraits>(10);
        mOHQueryHandler[i]->setAggregateListener(this); // *Must* be before handler->initialize
        bool object_static_objects = (mSeparateDynamicObjects && i == OBJECT_CLASS_STATIC);
        mOHQueryHandler[i]->initialize(
            mLocCache, mLocCache, object_static_objects,
            std::tr1::bind(&LibproxManualProximity::handlerShouldHandleObject, this, object_static_objects, false, _1, _2, _3, _4, _5)
        );
    }
}

LibproxManualProximity::~LibproxManualProximity() {
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        delete mOHQueryHandler[i];
    }
}

void LibproxManualProximity::start() {
    Proximity::start();

    mContext->add(&mOHHandlerPoller);
}

void LibproxManualProximity::poll() {

    // Get and ship OH results
    std::deque<OHResult> oh_results_copy;
    mOHResults.swap(oh_results_copy);
    mOHResultsToSend.insert(mOHResultsToSend.end(), oh_results_copy.begin(), oh_results_copy.end());

    bool oh_sent = true;
    while(oh_sent && !mOHResultsToSend.empty()) {
        const OHResult& msg_front = mOHResultsToSend.front();
        sendObjectHostResult(OHDP::NodeID(msg_front.first), msg_front.second);
        delete msg_front.second;
        mOHResultsToSend.pop_front();
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

// Object host session and message management

void LibproxManualProximity::onObjectHostSession(const OHDP::NodeID& id, OHDPSST::Stream::Ptr oh_stream) {
    // Setup listener for requests from object hosts. We should only
    // have one active substream at a time. Proximity sessions are
    // always initiated by the object host -- upon receiving a
    // connection we register the query and use the same substream to
    // transmit results.
    oh_stream->listenSubstream(
        OBJECT_PORT_PROXIMITY,
        std::tr1::bind(
            &LibproxManualProximity::handleObjectHostSubstream, this,
            _1, _2
        )
    );
}

void LibproxManualProximity::handleObjectHostSubstream(int success, OHDPSST::Stream::Ptr substream) {
    if (success != SST_IMPL_SUCCESS) return;

    PROXLOG(detailed, "New object host proximity session from " << substream->remoteEndPoint().endPoint);
    // Store this for sending data back
    addObjectHostProxStreamInfo(substream);
    // And register to read requests
    readFramesFromObjectHostStream(
        substream->remoteEndPoint().endPoint.node(),
        mProxStrand->wrap(
            std::tr1::bind(&LibproxManualProximity::handleObjectHostProxMessage, this, substream->remoteEndPoint().endPoint.node(), _1)
        )
    );
}

void LibproxManualProximity::onObjectHostSessionEnded(const OHDP::NodeID& id) {
    mProxStrand->post(
        std::tr1::bind(&LibproxManualProximity::handleObjectHostSessionEnded, this, id)
    );
}


void LibproxManualProximity::aggregateCreated(ProxAggregator* handler, const UUID& objid) {
}

void LibproxManualProximity::aggregateChildAdded(ProxAggregator* handler, const UUID& objid, const UUID& child, const BoundingSphere3f& bnds) {
}

void LibproxManualProximity::aggregateChildRemoved(ProxAggregator* handler, const UUID& objid, const UUID& child, const BoundingSphere3f& bnds) {
}

void LibproxManualProximity::aggregateBoundsUpdated(ProxAggregator* handler, const UUID& objid, const BoundingSphere3f& bnds) {
}

void LibproxManualProximity::aggregateDestroyed(ProxAggregator* handler, const UUID& objid) {
}

void LibproxManualProximity::aggregateObserved(ProxAggregator* handler, const UUID& objid, uint32 nobservers) {
}



int32 LibproxManualProximity::objectHostQueries() const {
    return mOHQueries[OBJECT_CLASS_STATIC].size();
}






// PROX Thread

void LibproxManualProximity::tickQueryHandler(ProxQueryHandler* qh[NUM_OBJECT_CLASSES]) {
    Time simT = mContext->simTime();
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (qh[i] != NULL)
            qh[i]->tick(simT);
    }
}

void LibproxManualProximity::handleObjectHostProxMessage(const OHDP::NodeID& id, const String& data) {
    Protocol::Prox::QueryRequest request;
    bool parse_success = request.ParseFromString(data);

    using namespace boost::property_tree;
    ptree pt;
    try {
        std::stringstream phy_json(request.query_parameters());
        read_json(phy_json, pt);
    }
    catch(json_parser::json_parser_error exc) {
        PROXLOG(error, "Error parsing object host query request: " << request.query_parameters() << " (" << exc.what() << ")");
        return;
    }

    String action = pt.get("action", String(""));
    if (action.empty()) return;
    if (action == "init") {
        PROXLOG(detailed, "Init query for " << id);

        for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
            if (mOHQueryHandler[i] == NULL) continue;

            // FIXME we need some way of specifying the basic query
            // parameters for OH queries (or maybe just get rid of
            // these basic properties as they aren't even required for
            // this type of query?)
            TimedMotionVector3f pos(mContext->simTime(), MotionVector3f(Vector3f(0, 0, 0), Vector3f(0, 0, 0)));
            BoundingSphere3f bounds(Vector3f(0, 0, 0), 0);
            float max_size = 0.f;
            ProxQuery* q = mOHQueryHandler[i]->registerQuery(pos, bounds, max_size);
            mOHQueries[i][id] = q;
            mInvertedOHQueries[q] = id;
            // Set the listener last since it can trigger callbacks
            // and we want everything to be setup already
            q->setEventListener(this);
        }
    }
    else if (action == "refine") {
        PROXLOG(detailed, "Refine query for " << id);
    }
    else if (action == "coarsen") {
        PROXLOG(detailed, "Coarsen query for " << id);
    }
    else if (action == "destroy") {
        destroyQuery(id);
    }
}

void LibproxManualProximity::handleObjectHostSessionEnded(const OHDP::NodeID& id) {
    destroyQuery(id);
}

void LibproxManualProximity::destroyQuery(const OHDP::NodeID& id) {
    PROXLOG(detailed, "Destroy query for " << id);
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mOHQueryHandler[i] == NULL) continue;

        OHQueryMap::iterator it = mOHQueries[i].find(id);
        if (it == mOHQueries[i].end()) continue;

        ProxQuery* q = it->second;
        mOHQueries[i].erase(it);
        mInvertedOHQueries.erase(q);
        delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
    }
}



bool LibproxManualProximity::handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const UUID& obj_id, bool is_local, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize) {
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

SeqNoPtr LibproxManualProximity::getOrCreateSeqNoInfo(const OHDP::NodeID& node)
{
    OHSeqNoInfoMap::iterator proxSeqNoIt = mOHSeqNos.find(node);
    if (proxSeqNoIt == mOHSeqNos.end())
        proxSeqNoIt = mOHSeqNos.insert( OHSeqNoInfoMap::value_type(node, SeqNoPtr(new SeqNo())) ).first;
    return proxSeqNoIt->second;
}

void LibproxManualProximity::eraseSeqNoInfo(const OHDP::NodeID& node)
{
    OHSeqNoInfoMap::iterator proxSeqNoIt = mOHSeqNos.find(node);
    if (proxSeqNoIt == mOHSeqNos.end()) return;
    mOHSeqNos.erase(proxSeqNoIt);
}

void LibproxManualProximity::queryHasEvents(ProxQuery* query) {
    typedef std::deque<ProxQueryEvent> QueryEventList;

    uint32 max_count = GetOptionValue<uint32>(PROX_MAX_PER_RESULT);

    OHDP::NodeID query_id = mInvertedOHQueries[query];
    SeqNoPtr seqNoPtr = getOrCreateSeqNoInfo(query_id);

    QueryEventList evts;
    query->popEvents(evts);

    PROXLOG(detailed, evts.size() << " events for query " << query_id);
    while(!evts.empty()) {
        Sirikata::Protocol::Prox::ProximityResults prox_results;
        prox_results.set_t(mContext->simTime());

        uint32 count = 0;
        while(count < max_count && !evts.empty()) {
            const ProxQueryEvent& evt = evts.front();
            Sirikata::Protocol::Prox::IProximityUpdate event_results = prox_results.add_update();

            for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
                UUID objid = evt.additions()[aidx].id();
                if (mLocCache->tracking(objid)) { // If the cache already lost it, we can't do anything
                    count++;

                    // FIXME
                    //mContext->mainStrand->post(
                    //    std::tr1::bind(&LibproxManualProximity::handleAddObjectLocSubscription, this, query_id, objid)
                    //);

                    Sirikata::Protocol::Prox::IObjectAddition addition = event_results.add_addition();
                    addition.set_object( objid );


                    //query_id contains the uuid of the object that is receiving
                    //the proximity message that obj_id has been added.
                    uint64 seqNo = (*seqNoPtr);
                    addition.set_seqno (seqNo);


                    Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
                    TimedMotionVector3f loc = mLocCache->location(objid);
                    motion.set_t(loc.updateTime());
                    motion.set_position(loc.position());
                    motion.set_velocity(loc.velocity());

                    TimedMotionQuaternion orient = mLocCache->orientation(objid);
                    Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
                    msg_orient.set_t(orient.updateTime());
                    msg_orient.set_position(orient.position());
                    msg_orient.set_velocity(orient.velocity());

                    addition.set_bounds( mLocCache->bounds(objid) );
                    const String& mesh = mLocCache->mesh(objid);
                    if (mesh.size() > 0)
                        addition.set_mesh(mesh);
                    const String& phy = mLocCache->physics(objid);
                    if (phy.size() > 0)
                        addition.set_physics(phy);
                }
            }
            for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
                UUID objid = evt.removals()[ridx].id();
                count++;
                // Clear out seqno and let main strand remove loc
                // subcription
                // FIXME
                //mContext->mainStrand->post(
                //    std::tr1::bind(&LibproxManualProximity::handleRemoveObjectLocSubscription, this, query_id, objid)
                //);

                Sirikata::Protocol::Prox::IObjectRemoval removal = event_results.add_removal();
                removal.set_object( objid );
                uint64 seqNo = (*seqNoPtr)++;
                removal.set_seqno (seqNo);
                removal.set_type(
                    (evt.removals()[ridx].permanent() == ProxQueryEvent::Permanent)
                    ? Sirikata::Protocol::Prox::ObjectRemoval::Permanent
                    : Sirikata::Protocol::Prox::ObjectRemoval::Transient
                );
            }
            evts.pop_front();
        }

        // Note null ID's since these are OHDP messages.
        Sirikata::Protocol::Object::ObjectMessage* obj_msg = createObjectMessage(
            mContext->id(),
            UUID::null(), OBJECT_PORT_PROXIMITY,
            UUID::null(), OBJECT_PORT_PROXIMITY,
            serializePBJMessage(prox_results)
        );
        mOHResults.push( OHResult(query_id, obj_msg) );
    }
}

} // namespace Sirikata
