/*  Sirikata
 *  LocationService.hpp
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

#ifndef _SIRIKATA_LOCATION_SERVICE_HPP_
#define _SIRIKATA_LOCATION_SERVICE_HPP_

#include <sirikata/space/SpaceContext.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/space/ServerMessage.hpp>
#include <sirikata/core/service/PollingService.hpp>

#include <sirikata/core/network/SSTImpl.hpp>

#include <sirikata/space/Platform.hpp>

#include <sirikata/core/util/Factory.hpp>
#include <sirikata/space/ObjectSessionManager.hpp>

namespace Sirikata {



class LocationServiceListener;
class LocationUpdatePolicy;
class LocationService;

/** Interface for objects that need to listen for location updates. */
class SIRIKATA_SPACE_EXPORT LocationServiceListener {
public:
    virtual ~LocationServiceListener();

    virtual void localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) {}
    virtual void localObjectRemoved(const UUID& uuid, bool agg) {}
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {}
    virtual void localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) {}
    virtual void localBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval) {}
    virtual void localMeshUpdated(const UUID& uuid, bool agg, const String& newval) {}
    virtual void localPhysicsUpdated(const UUID& uuid, bool agg, const String& newval) {}

    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) {}
    virtual void replicaObjectRemoved(const UUID& uuid) {}
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {}
    virtual void replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) {}
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {}
    virtual void replicaMeshUpdated(const UUID& uuid, const String& newval) {}
    virtual void replicaPhysicsUpdated(const UUID& uuid, const String& newval) {}
}; // class LocationServiceListener


/** LocationUpdatePolicy controls when updates are sent to subscribers.
 *  It is informed of subscriptions, object location updates, and gets
 *  regular tick calls.  Based on this information it decides when to
 *  send updates to individual subscribers.
 */
class SIRIKATA_SPACE_EXPORT LocationUpdatePolicy : public LocationServiceListener, public Service {
public:
    LocationUpdatePolicy();
    virtual ~LocationUpdatePolicy();

    virtual void initialize(LocationService* loc);

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void subscribe(ServerID remote, const UUID& uuid, SeqNoPtr seqNo) = 0;
    virtual void unsubscribe(ServerID remote, const UUID& uuid) = 0;
    virtual void unsubscribe(ServerID remote) = 0;

    virtual void subscribe(const OHDP::NodeID& remote, const UUID& uuid) = 0;
    virtual void unsubscribe(const OHDP::NodeID& remote, const UUID& uuid) = 0;
    virtual void unsubscribe(const OHDP::NodeID& remote) = 0;

    virtual void subscribe(const UUID& remote, const UUID& uuid) = 0;
    virtual void unsubscribe(const UUID& remote, const UUID& uuid) = 0;
    virtual void unsubscribe(const UUID& remote) = 0;

    virtual void service() = 0;

protected:
    LocationService* mLocService; // The owner of this UpdatePolicy
    Router<Message*>* mLocMessageRouter; // Server Message Router for Loc Service
}; // class LocationUpdatePolicy

class SIRIKATA_SPACE_EXPORT LocationUpdatePolicyFactory
    : public AutoSingleton<LocationUpdatePolicyFactory>,
      public Factory2<LocationUpdatePolicy*, SpaceContext*, const String&>
{
  public:
    static LocationUpdatePolicyFactory& getSingleton();
    static void destroy();
}; // class LocationServiceFactory


/** Interface for location services.  This provides a way for other components
 *  to get the most current information about object locations.
 */
class SIRIKATA_SPACE_EXPORT LocationService : public MessageRecipient, public PollingService, public ObjectSessionListener {
public:
    enum TrackingType {
        NotTracking,
        Local,
        Aggregate,
        Replica
    };

    LocationService(SpaceContext* ctx, LocationUpdatePolicy* update_policy);
    virtual ~LocationService();

    const SpaceContext* context() const {
        return mContext;
    }

    // ObjectSessionListener Interface
    virtual void newSession(ObjectSession* session);

    /** Indicates whether this location service is tracking the given object.  It is only
     *  safe to request information */
    virtual bool contains(const UUID& uuid) const = 0;
    virtual TrackingType type(const UUID& uuid) const = 0;

    /** Methods dealing with information requests. */
    virtual TimedMotionVector3f location(const UUID& uuid) = 0;
    virtual Vector3f currentPosition(const UUID& uuid) = 0;
    virtual TimedMotionQuaternion orientation(const UUID& uuid) = 0;
    virtual Quaternion currentOrientation(const UUID& uuid) = 0;
    virtual BoundingSphere3f bounds(const UUID& uuid) = 0;
    virtual const String& mesh(const UUID& uuid) = 0;
    virtual const String& physics(const UUID& uuid) = 0;

    /** Methods dealing with local objects. */
    virtual void addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) = 0;
    virtual void removeLocalObject(const UUID& uuid) = 0;

    /** Aggregate objects are handled separately from other local objects.  All
     *  the information is tracked, but listeners are notified about them
     *  separately. This is necessary since not filtering them would cause them
     *  to filter back into whatever system generated them.  In general, users
     *  other than Proximity should learn about aggregates and treat them as
     *  normal objects. Proximity ignores them since it is the one that
     *  generates them.
     */
    virtual void addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) = 0;
    virtual void removeLocalAggregateObject(const UUID& uuid) = 0;
    virtual void updateLocalAggregateLocation(const UUID& uuid, const TimedMotionVector3f& newval) = 0;
    virtual void updateLocalAggregateOrientation(const UUID& uuid, const TimedMotionQuaternion& newval) = 0;
    virtual void updateLocalAggregateBounds(const UUID& uuid, const BoundingSphere3f& newval) = 0;
    virtual void updateLocalAggregateMesh(const UUID& uuid, const String& newval) = 0;
    virtual void updateLocalAggregatePhysics(const UUID& uuid, const String& newval) = 0;

    /** Methods dealing with replica objects. */
    virtual void addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) = 0;
    virtual void removeReplicaObject(const Time& t, const UUID& uuid) = 0;

    /** Methods dealing with listeners. */
    virtual void addListener(LocationServiceListener* listener, bool want_aggregates);
    virtual void removeListener(LocationServiceListener* listener);

    /** Subscriptions for other servers. */
    virtual void subscribe(ServerID remote, const UUID& uuid, SeqNoPtr seq_no_ptr);
    virtual void unsubscribe(ServerID remote, const UUID& uuid);
    /** Unsubscripe the given server from all its location subscriptions. */
    virtual void unsubscribe(ServerID remote);

    /** Subscriptions for connected object hosts. */
    virtual void subscribe(const OHDP::NodeID& remote, const UUID& uuid);
    virtual void unsubscribe(const OHDP::NodeID& remote, const UUID& uuid);
    /** Unsubscripe the given object host from all its location subscriptions. */
    virtual void unsubscribe(const OHDP::NodeID& remote);

    /** Subscriptions for local objects. */
    virtual void subscribe(const UUID& remote, const UUID& uuid);
    virtual void unsubscribe(const UUID& remote, const UUID& uuid);
    /** Unsubscripe the given server from all its location subscriptions. */
    virtual void unsubscribe(const UUID& remote);


    /** MessageRecipient Interface. */
    virtual void receiveMessage(Message* msg) = 0;

    virtual bool locationUpdate(UUID source, void* buffer, uint32 length) = 0;

protected:
    virtual void start();
    virtual void stop();
    virtual void poll();
    virtual void service() = 0;

    void notifyLocalObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) const;
    void notifyLocalObjectRemoved(const UUID& uuid, bool agg) const;
    void notifyLocalLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) const;
    void notifyLocalOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) const;
    void notifyLocalBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval) const;
    void notifyLocalMeshUpdated(const UUID& uuid, bool agg, const String& newval) const;
    void notifyLocalPhysicsUpdated(const UUID& uuid, bool agg, const String& newval) const;

    void notifyReplicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) const;
    void notifyReplicaObjectRemoved(const UUID& uuid) const;
    void notifyReplicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) const;
    void notifyReplicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) const;
    void notifyReplicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) const;
    void notifyReplicaMeshUpdated(const UUID& uuid, const String& newval) const;
    void notifyReplicaPhysicsUpdated(const UUID& uuid, const String& newval) const;

    // Helper for listening for datagrams
    void handleLocationUpdateDatagram(UUID source, void* buffer, uint32 length);
    // Helpers for listening to streams
    typedef SST::Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;
    void handleLocationUpdateSubstream(const UUID& source, int err, SSTStreamPtr s);
    void handleLocationUpdateSubstreamRead(const UUID& source, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length);

    SpaceContext* mContext;
private:
    TimeProfiler::Stage* mProfiler;
protected:
    struct ListenerInfo {
        LocationServiceListener* listener;
        bool wantAggregates;

        bool operator<(const ListenerInfo& rhs) const { return listener < rhs.listener; }
    };
    typedef std::set<ListenerInfo> ListenerList;
    ListenerList mListeners;

    LocationUpdatePolicy* mUpdatePolicy;
}; // class LocationService

class SIRIKATA_SPACE_EXPORT LocationServiceFactory
    : public AutoSingleton<LocationServiceFactory>,
      public Factory3<LocationService*, SpaceContext*, LocationUpdatePolicy*, const String&>
{
  public:
    static LocationServiceFactory& getSingleton();
    static void destroy();
}; // class LocationServiceFactory

} // namespace Sirikata

#endif //_SIRIKATA_LOCATION_SERVICE_HPP_
