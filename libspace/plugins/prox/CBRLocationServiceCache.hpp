/*  Sirikata
 *  CBRLocationServiceCache.hpp
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

#ifndef _SIRIKATA_CBR_LOCATION_SERVICE_CACHE_HPP_
#define _SIRIKATA_CBR_LOCATION_SERVICE_CACHE_HPP_

#include <sirikata/pintoloc/ProxSimulationTraits.hpp>
#include <sirikata/space/LocationService.hpp>
#include <prox/base/LocationServiceCache.hpp>
#include <prox/base/ZernikeDescriptor.hpp>

namespace Sirikata {

/* Implementation of LocationServiceCache which serves Prox libraries;
 * works by listening for updates from our LocationService.  Note that
 * CBR should only be using the LocationServiceListener methods in normal
 * operation -- all other threads are to be used by libprox classes and
 * will only be accessed in the proximity thread. Therefore, most of the
 * work happens in the proximity thread, with the callbacks just storing
 * information to be picked up in the next iteration.
 */
class CBRLocationServiceCache : public Prox::LocationServiceCache<UUIDProxSimulationTraits>, public LocationServiceListener {
public:
    typedef Prox::LocationUpdateListener<UUIDProxSimulationTraits> LocationUpdateListener;

    /** Constructs a CBRLocationServiceCache which caches entries from locservice.  If
     *  replicas is true, then it caches replica entries from locservice, in addition
     *  to the local entries it always caches.
     */
    CBRLocationServiceCache(Network::IOStrand* strand, LocationService* locservice, bool replicas);
    virtual ~CBRLocationServiceCache();

    /* LocationServiceCache members. */
    virtual void addPlaceholderImposter(
        const ObjectID& id,
        const Vector3f& center_offset,
        const float32 center_bounds_radius,
        const float32 max_size,
        const String& zernike,
        const String& mesh
    );
    virtual Iterator startTracking(const ObjectID& id);
    virtual void stopTracking(const Iterator& id);

    bool tracking(const ObjectID& id);

    virtual TimedMotionVector3f location(const Iterator& id);
    virtual Vector3f centerOffset(const Iterator& id);
    virtual float32 centerBoundsRadius(const Iterator& id);
    virtual float32 maxSize(const Iterator& id);
    virtual bool isLocal(const Iterator& id);
    Prox::ZernikeDescriptor& zernikeDescriptor(const Iterator& id);
    String mesh(const Iterator& id);

    virtual const UUID& iteratorID(const Iterator& id);

    virtual void addUpdateListener(LocationUpdateListener* listener);
    virtual void removeUpdateListener(LocationUpdateListener* listener);

    // We also provide accessors by ID for Proximity generate results.
    const TimedMotionVector3f& location(const ObjectID& id) const;
    const TimedMotionQuaternion& orientation(const ObjectID& id) const;
    const AggregateBoundingInfo& bounds(const ObjectID& id) const;
    const String& mesh(const ObjectID& id) const;
    const String& physics(const ObjectID& id) const;

    const bool isAggregate(const ObjectID& id) const;


    /* LocationServiceListener members. */
    virtual void localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& physics, const String& zernike);
    virtual void localObjectRemoved(const UUID& uuid, bool agg);
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    virtual void localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval);
    virtual void localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval);
    virtual void localMeshUpdated(const UUID& uuid, bool agg, const String& newval);
    virtual void localPhysicsUpdated(const UUID& uuid, bool agg, const String& newval);
    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& physics, const String& zernike);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval);
    virtual void replicaMeshUpdated(const UUID& uuid, const String& newval);
    virtual void replicaPhysicsUpdated(const UUID& uuid, const String& newval);

private:
    // Object data is only accessed in the prox thread (by libprox
    // and by this class when updates are passed by the main thread).
    // Therefore, this data does *NOT* need to be locked for access.
    struct ObjectData {
        TimedMotionVector3f location;
        TimedMotionQuaternion orientation;
        AggregateBoundingInfo bounds;
        // Whether the object is local or a replica
        bool isLocal;
        String mesh;
        String physics;
        Prox::ZernikeDescriptor zernike;
        bool exists; // Exists, i.e. xObjectRemoved hasn't been called
        int16 tracking; // Ref count to support multiple users
        bool isAggregate;
    };


    // These generate and queue up updates from the main thread
    void objectAdded(const UUID& uuid, bool islocal, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& physics, const String& zernike);
    void objectRemoved(const UUID& uuid, bool agg);
    void locationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    void orientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval);
    void boundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval);
    void meshUpdated(const UUID& uuid, bool agg, const String& newval);
    void physicsUpdated(const UUID& uuid, bool agg, const String& newval);

    // These do the actual work for the LocationServiceListener methods.  Local versions always
    // call these, replica versions only call them if replica tracking is
    // on. Although we now have to lock in these, we put them on the strand
    // instead of processing directly in the methods above so that they don't
    // block any other work.
    void processObjectAdded(const UUID& uuid, ObjectData data);
    void processObjectRemoved(const UUID& uuid, bool agg);
    void processLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    void processOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval);
    void processBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval);
    void processMeshUpdated(const UUID& uuid, bool agg, const String& newval);
    void processPhysicsUpdated(const UUID& uuid, bool agg, const String& newval);


    CBRLocationServiceCache();

    typedef boost::recursive_mutex Mutex;
    typedef boost::lock_guard<Mutex> Lock;
    Mutex mMutex;

    Network::IOStrand* mStrand;
    LocationService* mLoc;

    typedef std::set<LocationUpdateListener*> ListenerSet;
    ListenerSet mListeners;

    typedef std::tr1::unordered_map<UUID, ObjectData, UUID::Hasher> ObjectDataMap;
    ObjectDataMap mObjects;
    bool mWithReplicas;

    bool tryRemoveObject(ObjectDataMap::iterator& obj_it);

    // Data contained in our Iterators. We maintain both the UUID and the
    // iterator because the iterator can become invalidated due to ordering of
    // events in the prox thread.
    struct IteratorData {
        IteratorData(const UUID& _objid, ObjectDataMap::iterator _it)
         : objid(_objid), it(_it) {}

        const UUID objid;
        ObjectDataMap::iterator it;
    };

};

} // namespace Sirikata

#endif //_SIRIKATA_CBR_LOCATION_SERVICE_CACHE_HPP_
