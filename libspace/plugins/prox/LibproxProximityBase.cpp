// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "LibproxProximityBase.hpp"

#include <sirikata/core/network/Frame.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <sirikata/space/AggregateManager.hpp>

#define PROXLOG(level,msg) SILOG(prox,level,"[PROX] " << msg)

namespace Sirikata {

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::readFramesFromStream(Ptr prox_stream, FrameReceivedCallback cb) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    assert(iostream);
    read_frame_cb = cb;
    iostream->registerReadCallback(
        std::tr1::bind(
            &LibproxProximityBase::ProxStreamInfo<EndpointType,StreamType>::handleRead,
            WPtr(prox_stream), _1, _2
        )
    );
}

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::handleRead(WPtr w_prox_stream, uint8* data, int size) {
    Ptr prox_stream = w_prox_stream.lock();
    if (!prox_stream) return;

    prox_stream->partial_frame.append((const char*)data, size);
    while(true) {
        String parsed = Network::Frame::parse(prox_stream->partial_frame);
        if (parsed.empty()) return;
        prox_stream->read_frame_cb(parsed);
    }
}

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::writeSomeObjectResults(Context* ctx, WPtr w_prox_stream) {
    static Duration retry_rate = Duration::milliseconds((int64)1);

    Ptr prox_stream = w_prox_stream.lock();
    if (!prox_stream) return;

    prox_stream->writing = true;

    if (!prox_stream->iostream) {
        // We're still waiting on the iostream, proxSubstreamCallback will call
        // us when it gets the iostream.
        prox_stream->writing = false;
        return;
    }

    // Otherwise, keep sending until we run out or
    while(!prox_stream->outstanding.empty()) {
        std::string& framed_prox_msg = prox_stream->outstanding.front();
        int bytes_written = prox_stream->iostream->write((const uint8*)framed_prox_msg.data(), framed_prox_msg.size());
        if (bytes_written < 0) {
            // FIXME
            break;
        }
        else if (bytes_written < (int)framed_prox_msg.size()) {
            framed_prox_msg = framed_prox_msg.substr(bytes_written);
            break;
        }
        else {
            prox_stream->outstanding.pop();
        }
    }

    if (prox_stream->outstanding.empty())
        prox_stream->writing = false;
    else
        ctx->mainStrand->post(retry_rate, prox_stream->writecb);
}

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::requestProxSubstream(LibproxProximityBase* parent, Context* ctx, const EndpointType& ep, Ptr prox_stream) {
    using std::tr1::placeholders::_1;

    // Always mark this up front. This keeps further requests from
    // occuring since the first time this method is entered, even if
    // the request is deferred, should eventually result in a stream.
    prox_stream->iostream_requested = true;

    // We need to check for a valid session here. This can be necessary because
    // we may have gotten a base session, registered a query, gotten results,
    // started to try returning them and had to wait for the base stream to the
    // object, and then had it disconnect before it ever got there. Then we'd be
    // stuck in an infinite loop, checking for the base stream and failing to
    // find it, then posting a retry.  validSession should only check for a
    // still-active connection, not the stream, so it should only kill this
    // process in this unusual case of a very short lived connection to the
    // space.
    if (!parent->validSession(ep)) return;

    StreamTypePtr base_stream = parent->getBaseStream(ep);
    if (!base_stream) {
        ctx->mainStrand->post(
            Duration::milliseconds((int64)5),
            std::tr1::bind(&LibproxProximityBase::ProxStreamInfo<EndpointType,StreamType>::requestProxSubstream, parent, ctx, ep, prox_stream)
        );
        return;
    }

    base_stream->createChildStream(
        ctx->mainStrand->wrap(
            std::tr1::bind(&LibproxProximityBase::ProxStreamInfo<EndpointType,StreamType>::proxSubstreamCallback, parent, ctx, _1, ep, base_stream, _2, prox_stream)
        ),
        (void*)NULL, 0,
        OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
    );
}

template<typename EndpointType, typename StreamType>
void LibproxProximityBase::ProxStreamInfo<EndpointType, StreamType>::proxSubstreamCallback(LibproxProximityBase* parent, Context* ctx, int x, const EndpointType& ep, StreamTypePtr parent_stream, StreamTypePtr substream, Ptr prox_stream_info) {
    if (!substream) {
        // If they disconnected, ignore
        if (!parent->validSession(ep)) return;

        // Retry
        PROXLOG(warn,"Error opening Prox substream, retrying...");

        parent_stream->createChildStream(
            ctx->mainStrand->wrap(
                std::tr1::bind(&LibproxProximityBase::ProxStreamInfo<EndpointType,StreamType>::proxSubstreamCallback, parent, ctx, _1, ep, parent_stream, _2, prox_stream_info)
            ),
            (void*)NULL, 0,
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
        );

        return;
    }

    prox_stream_info->iostream = substream;
    assert(!prox_stream_info->writing);
    ProxStreamInfo::writeSomeObjectResults(ctx, prox_stream_info);
}




LibproxProximityBase::LibproxProximityBase(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr)
 : Proximity(ctx, locservice, cseg, net, aggmgr, Duration::milliseconds((int64)100)),
   mProxStrand(ctx->ioService->createStrand()),
   mLocCache(NULL)
{
    mProxServerMessageService = mContext->serverRouter()->createServerMessageService("proximity");

    // Location cache, for both types of queries
    mLocCache = new CBRLocationServiceCache(mProxStrand, locservice, true);

    // Deal with static/dynamic split
    mSeparateDynamicObjects = GetOptionValue<bool>(OPT_PROX_SPLIT_DYNAMIC);
    mNumQueryHandlers = (mSeparateDynamicObjects ? 2 : 1);
}

LibproxProximityBase::~LibproxProximityBase() {
    delete mProxServerMessageService;
    delete mLocCache;
    delete mProxStrand;
}


const String& LibproxProximityBase::ObjectClassToString(ObjectClass c) {
    static String static_ = "static";
    static String dynamic_ = "dynamic";
    static String unknown_ = "unknown";

    switch(c) {
      case OBJECT_CLASS_STATIC: return static_; break;
      case OBJECT_CLASS_DYNAMIC: return dynamic_; break;
      default: return unknown_; break;
    }
}

BoundingBox3f LibproxProximityBase::aggregateBBoxes(const BoundingBoxList& bboxes) {
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);
    return bbox;
}

bool LibproxProximityBase::velocityIsStatic(const Vector3f& vel) {
    // These values are arbitrary, just meant to indicate that the object is,
    // for practical purposes, not moving.
    return (
        vel.x < .05f &&
        vel.y < .05f &&
        vel.z < .05f
    );
}


// MAIN Thread

void LibproxProximityBase::readFramesFromObjectStream(const ObjectReference& oref, ProxObjectStreamInfo::FrameReceivedCallback cb) {
    ObjectProxStreamMap::iterator prox_stream_it = mObjectProxStreams.find(oref.getAsUUID());
    assert(prox_stream_it != mObjectProxStreams.end());
    prox_stream_it->second->readFramesFromStream(prox_stream_it->second, cb);
}

void LibproxProximityBase::readFramesFromObjectHostStream(const OHDP::NodeID& node, ProxObjectHostStreamInfo::FrameReceivedCallback cb) {
    ObjectHostProxStreamMap::iterator prox_stream_it = mObjectHostProxStreams.find(node);
    assert(prox_stream_it != mObjectHostProxStreams.end());
    prox_stream_it->second->readFramesFromStream(prox_stream_it->second, cb);
}

void LibproxProximityBase::sendObjectResult(Sirikata::Protocol::Object::ObjectMessage* msg) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    // Try to find stream info for the object
    ObjectProxStreamMap::iterator prox_stream_it = mObjectProxStreams.find(msg->dest_object());
    if (prox_stream_it == mObjectProxStreams.end()) {
        prox_stream_it = mObjectProxStreams.insert( ObjectProxStreamMap::value_type(msg->dest_object(), ProxObjectStreamInfoPtr(new ProxObjectStreamInfo())) ).first;
        prox_stream_it->second->writecb = std::tr1::bind(
            &LibproxProximityBase::ProxObjectStreamInfo::writeSomeObjectResults, mContext, ProxObjectStreamInfo::WPtr(prox_stream_it->second)
        );
    }
    ProxObjectStreamInfoPtr prox_stream = prox_stream_it->second;

    // If we don't have a stream yet, try to build it
    if (!prox_stream->iostream_requested)
        ProxObjectStreamInfo::requestProxSubstream(this, mContext, ObjectReference(msg->dest_object()), prox_stream);

    // Build the result and push it into the stream
    // FIXME this is an infinite sized queue, but we don't really want to drop
    // proximity results....
    prox_stream->outstanding.push(Network::Frame::write(msg->payload()));

    // If writing isn't already in progress, start it up
    if (!prox_stream->writing)
        ProxObjectStreamInfo::writeSomeObjectResults(mContext, prox_stream);
}

void LibproxProximityBase::sendObjectHostResult(const OHDP::NodeID& node, Sirikata::Protocol::Object::ObjectMessage* msg) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    // Try to find stream info for the object
    ObjectHostProxStreamMap::iterator prox_stream_it = mObjectHostProxStreams.find(node);
    if (prox_stream_it == mObjectHostProxStreams.end()) {
        prox_stream_it = mObjectHostProxStreams.insert( ObjectHostProxStreamMap::value_type(node, ProxObjectHostStreamInfoPtr(new ProxObjectHostStreamInfo())) ).first;
        prox_stream_it->second->writecb = std::tr1::bind(
            &LibproxProximityBase::ProxObjectHostStreamInfo::writeSomeObjectResults, mContext, ProxObjectHostStreamInfo::WPtr(prox_stream_it->second)
        );
    }
    ProxObjectHostStreamInfoPtr prox_stream = prox_stream_it->second;

    // If we don't have a stream yet, try to build it
    if (!prox_stream->iostream_requested)
        ProxObjectHostStreamInfo::requestProxSubstream(this, mContext, node, prox_stream);

    // Build the result and push it into the stream
    // FIXME this is an infinite sized queue, but we don't really want to drop
    // proximity results....
    prox_stream->outstanding.push(Network::Frame::write(msg->payload()));

    // If writing isn't already in progress, start it up
    if (!prox_stream->writing)
        ProxObjectHostStreamInfo::writeSomeObjectResults(mContext, prox_stream);
}


bool LibproxProximityBase::validSession(const ObjectReference& oref) const {
    return (mContext->objectSessionManager()->getSession(oref) != NULL);
}

bool LibproxProximityBase::validSession(const OHDP::NodeID& node) const {
    return mContext->ohSessionManager()->getSession(node);
}

LibproxProximityBase::ProxObjectStreamPtr LibproxProximityBase::getBaseStream(const ObjectReference& oref) const {
    ObjectSession* session = mContext->objectSessionManager()->getSession(oref);
    return (session != NULL ? session->getStream() : ProxObjectStreamPtr());
}

LibproxProximityBase::ProxObjectHostStreamPtr LibproxProximityBase::getBaseStream(const OHDP::NodeID& node) const {
    ObjectHostSessionPtr session = mContext->ohSessionManager()->getSession(node);
    return (session ? session->stream() : ProxObjectHostStreamPtr());
}

void LibproxProximityBase::addObjectProxStreamInfo(ODPSST::Stream::Ptr strm) {
    UUID objid = strm->remoteEndPoint().endPoint.object().getAsUUID();
    assert(mObjectProxStreams.find(objid) == mObjectProxStreams.end());

    mObjectProxStreams.insert(
        ObjectProxStreamMap::value_type(
            objid,
            ProxObjectStreamInfoPtr(new ProxObjectStreamInfo(strm))
        )
    );
}

void LibproxProximityBase::addObjectHostProxStreamInfo(OHDPSST::Stream::Ptr strm) {
    OHDP::NodeID nodeid = strm->remoteEndPoint().endPoint.node();
    assert(mObjectHostProxStreams.find(nodeid) == mObjectHostProxStreams.end());

    mObjectHostProxStreams.insert(
        ObjectHostProxStreamMap::value_type(
            nodeid,
            ProxObjectHostStreamInfoPtr(new ProxObjectHostStreamInfo(strm))
        )
    );
}




void LibproxProximityBase::handleAddObjectLocSubscription(const UUID& subscriber, const UUID& observed) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed);
}

void LibproxProximityBase::handleRemoveObjectLocSubscription(const UUID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}

void LibproxProximityBase::handleRemoveAllObjectLocSubscription(const UUID& subscriber) {
    mLocService->unsubscribe(subscriber);
}

void LibproxProximityBase::handleAddOHLocSubscription(const OHDP::NodeID& subscriber, const UUID& observed) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed);
}

void LibproxProximityBase::handleRemoveOHLocSubscription(const OHDP::NodeID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}

void LibproxProximityBase::handleRemoveAllOHLocSubscription(const OHDP::NodeID& subscriber) {
    mLocService->unsubscribe(subscriber);
}

void LibproxProximityBase::handleAddServerLocSubscription(const ServerID& subscriber, const UUID& observed, SeqNoPtr seqPtr) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed, seqPtr);
}

void LibproxProximityBase::handleRemoveServerLocSubscription(const ServerID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}

void LibproxProximityBase::handleRemoveAllServerLocSubscription(const ServerID& subscriber) {
    mLocService->unsubscribe(subscriber);
}



// PROX Thread

void LibproxProximityBase::aggregateCreated(const UUID& objid) {
    // On addition, an "aggregate" will have no children, i.e. its zero sized.

    mContext->mainStrand->post(
        std::tr1::bind(
            &LocationService::addLocalAggregateObject, mLocService,
            objid,
            TimedMotionVector3f(mContext->simTime(), MotionVector3f()),
            TimedMotionQuaternion(mContext->simTime(), MotionQuaternion()),
            BoundingSphere3f(),
            "",
            ""
        )
    );

    mAggregateManager->addAggregate(objid);
}

void LibproxProximityBase::aggregateChildAdded(const UUID& objid, const UUID& child, const BoundingSphere3f& bnds) {
    mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximityBase::updateAggregateLoc, this,
            objid, bnds
        )
    );

    mAggregateManager->addChild(objid, child);
}

void LibproxProximityBase::aggregateChildRemoved(const UUID& objid, const UUID& child, const BoundingSphere3f& bnds) {
    // Loc cares only about this chance to update state of aggregate
    mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximityBase::updateAggregateLoc, this,
            objid, bnds
        )
    );

    mAggregateManager->removeChild(objid, child);
}

void LibproxProximityBase::aggregateBoundsUpdated(const UUID& objid, const BoundingSphere3f& bnds) {
    mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximityBase::updateAggregateLoc, this,
            objid, bnds
        )
    );

    mAggregateManager->generateAggregateMesh(objid, Duration::seconds(300.0+rand()%300));
}

void LibproxProximityBase::aggregateDestroyed(const UUID& objid) {
    mContext->mainStrand->post(
        std::tr1::bind(
            &LocationService::removeLocalAggregateObject, mLocService, objid
        )
    );
    mAggregateManager->removeAggregate(objid);
}

void LibproxProximityBase::aggregateObserved(const UUID& objid, uint32 nobservers) {
    mAggregateManager->aggregateObserved(objid, nobservers);
}

// MAIN strand
void LibproxProximityBase::updateAggregateLoc(const UUID& objid, const BoundingSphere3f& bnds) {
    // TODO(ewencp) This comparison looks wrong, but might be due to
    // the way we're setting location and bounds and both are using
    // bnds.center(). Shouldn't we be using the center for position
    // and make bounds origin centered? But apparently this was
    // working, so leaving it for now...
    if (mLocService->contains(objid) && mLocService->bounds(objid) != bnds) {
        mLocService->updateLocalAggregateLocation(
            objid,
            TimedMotionVector3f(mContext->simTime(), MotionVector3f(bnds.center(), Vector3f(0,0,0)))
        );
        mLocService->updateLocalAggregateBounds(
            objid,
            BoundingSphere3f(bnds.center(), bnds.radius())
        );
    }
}

} // namespace Sirikata
