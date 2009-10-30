/*  cbr
 *  CBRLocationServiceCache.cpp
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
 *  * Neither the name of libprox nor the names of its contributors may
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

#include "CBRLocationServiceCache.hpp"

namespace CBR {

CBRLocationServiceCache::CBRLocationServiceCache(LocationService* locservice, bool replicas)
 : Prox::LocationServiceCache<ProxSimulationTraits>(),
   LocationServiceListener(),
   mLoc(locservice),
   mListeners(),
   mObjects(),
   mWithReplicas(replicas)
{
    assert(mLoc != NULL);
    mLoc->addListener(this);
}

CBRLocationServiceCache::~CBRLocationServiceCache() {
    mLoc->removeListener(this);
    mLoc = NULL;
    mListeners.clear();
    mObjects.clear();
}

void CBRLocationServiceCache::startTracking(const UUID& id) {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    ObjectDataMap::iterator it = mObjects.find(id);
    assert(it != mObjects.end());

    it->second.tracking = true;
}

void CBRLocationServiceCache::stopTracking(const UUID& id) {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    ObjectDataMap::iterator it = mObjects.find(id);
    if (it == mObjects.end() || it->second.tracking == false) {
        printf("Warning: stopped tracking unknown object\n");
        return;
    }
    mObjects.erase(it);
}

bool CBRLocationServiceCache::tracking(const UUID& id) const {
    return (mObjects.find(id) != mObjects.end());
}


const TimedMotionVector3f& CBRLocationServiceCache::location(const UUID& id) const {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    ObjectDataMap::const_iterator it = mObjects.find(id);
    assert(it != mObjects.end());
    return it->second.location;
}

const BoundingSphere3f& CBRLocationServiceCache::bounds(const UUID& id) const {
    // NOTE: should only be accessed by prox thread, shouldn't need lock
    ObjectDataMap::const_iterator it = mObjects.find(id);
    assert(it != mObjects.end());
    return it->second.bounds;
}

void CBRLocationServiceCache::addUpdateListener(LocationUpdateListener* listener) {
    assert( mListeners.find(listener) == mListeners.end() );
    mListeners.insert(listener);
}

void CBRLocationServiceCache::removeUpdateListener(LocationUpdateListener* listener) {
    ListenerSet::iterator it = mListeners.find(listener);
    assert( it != mListeners.end() );
    mListeners.erase(it);
}

void CBRLocationServiceCache::localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    objectAdded(uuid, loc, bounds);
}

void CBRLocationServiceCache::localObjectRemoved(const UUID& uuid) {
    objectRemoved(uuid);
}

void CBRLocationServiceCache::localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    locationUpdated(uuid, newval);
}

void CBRLocationServiceCache::localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    boundsUpdated(uuid, newval);
}

void CBRLocationServiceCache::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    if (mWithReplicas)
        objectAdded(uuid, loc, bounds);
}

void CBRLocationServiceCache::replicaObjectRemoved(const UUID& uuid) {
    if (mWithReplicas)
        objectRemoved(uuid);
}

void CBRLocationServiceCache::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    if (mWithReplicas)
        locationUpdated(uuid, newval);
}

void CBRLocationServiceCache::replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    if (mWithReplicas)
        boundsUpdated(uuid, newval);
}


void CBRLocationServiceCache::objectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    ObjectUpdateData update;
    update.type = ObjectUpdateData::Addition;
    update.object = uuid;
    update.loc = loc;
    update.bounds = bounds;
    mObjectUpdates.push(update);
}

void CBRLocationServiceCache::objectRemoved(const UUID& uuid) {
    ObjectUpdateData update;
    update.type = ObjectUpdateData::Removal;
    update.object = uuid;
    mObjectUpdates.push(update);
}

void CBRLocationServiceCache::locationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    ObjectUpdateData update;
    update.type = ObjectUpdateData::Location;
    update.object = uuid;
    update.loc = newval;
    mObjectUpdates.push(update);
}

void CBRLocationServiceCache::boundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    ObjectUpdateData update;
    update.type = ObjectUpdateData::Bounds;
    update.object = uuid;
    update.bounds = newval;
    mObjectUpdates.push(update);
}


void CBRLocationServiceCache::processObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    if (mObjects.find(uuid) != mObjects.end())
        return;

    ObjectData data;
    data.location = loc;
    data.bounds = bounds;
    data.tracking = false;
    mObjects[uuid] = data;

    for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->locationConnected(uuid, loc, bounds);
}

void CBRLocationServiceCache::processObjectRemoved(const UUID& uuid) {
    ObjectDataMap::iterator data_it = mObjects.find(uuid);
    if (data_it == mObjects.end()) return;

    if (data_it->second.tracking == false)
        mObjects.erase(data_it);

    for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->locationDisconnected(uuid);
}


void CBRLocationServiceCache::processLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    TimedMotionVector3f oldval = it->second.location;
    it->second.location = newval;

    for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->locationPositionUpdated(uuid, oldval, newval);
}

void CBRLocationServiceCache::processBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    ObjectDataMap::iterator it = mObjects.find(uuid);
    if (it == mObjects.end()) return;

    BoundingSphere3f oldval = it->second.bounds;
    it->second.bounds = newval;

    for(ListenerSet::iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->locationBoundsUpdated(uuid, oldval, newval);
}


void CBRLocationServiceCache::serviceListeners() {
    std::deque<ObjectUpdateData> updates;

    mObjectUpdates.swap(updates);

    for( std::deque<ObjectUpdateData>::iterator upit = updates.begin(); upit != updates.end(); upit++) {
        switch( upit->type ) {
          case ObjectUpdateData::Addition:
            processObjectAdded(upit->object, upit->loc, upit->bounds);
            break;
          case ObjectUpdateData::Removal:
            processObjectRemoved(upit->object);
            break;
          case ObjectUpdateData::Location:
            processLocationUpdated(upit->object, upit->loc);
            break;
          case ObjectUpdateData::Bounds:
            processBoundsUpdated(upit->object, upit->bounds);
            break;
          default:
            SILOG(loccache,error,"Got unknown internal update type: " << upit->type);
            break;
        }
    }
    updates.clear();
}

} // namespace CBR
