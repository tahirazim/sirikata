/*  Sirikata
 *  LocationService.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/space/LocationService.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::LocationUpdatePolicyFactory);
AUTO_SINGLETON_INSTANCE(Sirikata::LocationServiceFactory);

namespace Sirikata {

LocationServiceListener::~LocationServiceListener() {
}



LocationUpdatePolicyFactory& LocationUpdatePolicyFactory::getSingleton() {
    return AutoSingleton<LocationUpdatePolicyFactory>::getSingleton();
}

void LocationUpdatePolicyFactory::destroy() {
    AutoSingleton<LocationUpdatePolicyFactory>::destroy();
}


LocationUpdatePolicy::LocationUpdatePolicy()
 : mLocService(NULL)
{
}

void LocationUpdatePolicy::initialize(LocationService* locservice)
{
    mLocService = locservice;
    mLocService->addListener(this, true);
    mLocMessageRouter = mLocService->context()->serverRouter()->createServerMessageService("loc-update");
}

LocationUpdatePolicy::~LocationUpdatePolicy() {
    delete mLocMessageRouter;
}


LocationServiceFactory& LocationServiceFactory::getSingleton() {
    return AutoSingleton<LocationServiceFactory>::getSingleton();
}

void LocationServiceFactory::destroy() {
    AutoSingleton<LocationServiceFactory>::destroy();
}

LocationService::LocationService(SpaceContext* ctx, LocationUpdatePolicy* update_policy)
 : PollingService(ctx->mainStrand, Duration::milliseconds((int64)10)),
   mContext(ctx),
   mUpdatePolicy(update_policy)
{
    mProfiler = mContext->profiler->addStage("Location Service");

    mUpdatePolicy->initialize(this);

    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_LOCATION, this);
    mContext->objectSessionManager()->addListener(this);
}

LocationService::~LocationService() {
    delete mProfiler;
    delete mUpdatePolicy;

    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_LOCATION, this);
    mContext->objectSessionManager()->removeListener(this);
}

void LocationService::newSession(ObjectSession* session) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    SST::Stream<SpaceObjectReference>::Ptr strm = session->getStream();

    SST::Connection<SpaceObjectReference>::Ptr conn = strm->connection().lock();
    assert(conn);

    SpaceObjectReference sourceObject = conn->remoteEndPoint().endPoint;

    strm->listenSubstream(OBJECT_PORT_LOCATION,
        std::tr1::bind(
            &LocationService::handleLocationUpdateSubstream, this,
            sourceObject.object().getAsUUID(),
            std::tr1::placeholders::_1,std::tr1::placeholders::_2
        )
    );

}

void LocationService::handleLocationUpdateSubstream(const UUID& source, int err, SSTStreamPtr s) {
    s->registerReadCallback(
        std::tr1::bind(
            &LocationService::handleLocationUpdateSubstreamRead, this,
            source, s, new std::stringstream(),
            std::tr1::placeholders::_1,std::tr1::placeholders::_2
        )
    );
}

void LocationService::handleLocationUpdateSubstreamRead(const UUID& source, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length) {
    prevdata->write((const char*)buffer, length);
    String payload(prevdata->str());
#ifdef SIRIKATA_SPACE_DELAY_APPLY_LOC_UPDATE
    static Duration delay = Duration::seconds(SIRIKATA_SPACE_DELAY_APPLY_LOC_UPDATE);
    mContext->mainStrand->post(
        delay,
        std::tr1::bind(
            &LocationService::tryHandleLocationUpdate, this,
            source, s, payload, prevdata
        )
    );
#else
    tryHandleLocationUpdate(source, s, payload, prevdata);
#endif
}

void LocationService::tryHandleLocationUpdate(const UUID& source, SSTStreamPtr s, const String& payload, std::stringstream* prevdata) {
    if (locationUpdate(source, (void*)payload.c_str(), payload.size())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s, and close the stream
        s->registerReadCallback(0);
        s->close(false);
    }
}

void LocationService::start() {
    PollingService::start();
    mUpdatePolicy->start();
}

void LocationService::stop() {
    mUpdatePolicy->stop();
    PollingService::stop();
}

void LocationService::poll() {
    mProfiler->started();
    service();
    mProfiler->finished();
}

void LocationService::addListener(LocationServiceListener* listener, bool want_aggregates) {
    ListenerInfo info;
    info.listener = listener;
    info.wantAggregates = want_aggregates;
    mListeners.insert(info);
}

void LocationService::removeListener(LocationServiceListener* listener) {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++) {
        if (it->listener == listener) {
            mListeners.erase(it);
            break;
        }
    }
}


void LocationService::subscribe(ServerID remote, const UUID& uuid, SeqNoPtr seqnoptr) {
    mUpdatePolicy->subscribe(remote, uuid, seqnoptr);
}

void LocationService::unsubscribe(ServerID remote, const UUID& uuid) {
    mUpdatePolicy->unsubscribe(remote, uuid);
}

void LocationService::unsubscribe(ServerID remote) {
    mUpdatePolicy->unsubscribe(remote);
}

void LocationService::subscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    mUpdatePolicy->subscribe(remote, uuid);
}

void LocationService::unsubscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    mUpdatePolicy->unsubscribe(remote, uuid);
}

void LocationService::unsubscribe(const OHDP::NodeID& remote) {
    mUpdatePolicy->unsubscribe(remote);
}


void LocationService::subscribe(const UUID& remote, const UUID& uuid) {
    mUpdatePolicy->subscribe(remote, uuid);
}

void LocationService::unsubscribe(const UUID& remote, const UUID& uuid) {
    mUpdatePolicy->unsubscribe(remote, uuid);
}

void LocationService::unsubscribe(const UUID& remote) {
    mUpdatePolicy->unsubscribe(remote);
}

void LocationService::notifyLocalObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localObjectAdded(uuid, agg, loc, orient, bounds, mesh, physics);
}

void LocationService::notifyLocalObjectRemoved(const UUID& uuid, bool agg) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localObjectRemoved(uuid, agg);
}


void LocationService::notifyLocalLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localLocationUpdated(uuid, agg, newval);
}

void LocationService::notifyLocalOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localOrientationUpdated(uuid, agg, newval);
}

void LocationService::notifyLocalBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localBoundsUpdated(uuid, agg, newval);
}


void LocationService::notifyLocalMeshUpdated(const UUID& uuid, bool agg, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localMeshUpdated(uuid, agg, newval);
}

void LocationService::notifyLocalPhysicsUpdated(const UUID& uuid, bool agg, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localPhysicsUpdated(uuid, agg, newval);
}


void LocationService::notifyReplicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaObjectAdded(uuid, loc, orient, bounds, mesh, physics);
}

void LocationService::notifyReplicaObjectRemoved(const UUID& uuid) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaObjectRemoved(uuid);
}

void LocationService::notifyReplicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaLocationUpdated(uuid, newval);
}

void LocationService::notifyReplicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaOrientationUpdated(uuid, newval);
}

void LocationService::notifyReplicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaBoundsUpdated(uuid, newval);
}

void LocationService::notifyReplicaMeshUpdated(const UUID& uuid, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaMeshUpdated(uuid, newval);
}

void LocationService::notifyReplicaPhysicsUpdated(const UUID& uuid, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaPhysicsUpdated(uuid, newval);
}

} // namespace Sirikata
