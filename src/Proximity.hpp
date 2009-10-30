/*  cbr
 *  Proximity.hpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_PROXIMITY_HPP_
#define _CBR_PROXIMITY_HPP_

#include "ProxSimulationTraits.hpp"
#include "CBRLocationServiceCache.hpp"
#include "ServerNetwork.hpp"
#include "CoordinateSegmentation.hpp"
#include "MigrationDataClient.hpp"
#include <prox/QueryHandler.hpp>
#include <prox/LocationUpdateListener.hpp>
#include "PollingService.hpp"

namespace CBR {

class LocationService;
class ProximityInputEvent;
class ProximityOutputEvent;

class Proximity : Prox::QueryEventListener<ProxSimulationTraits>, LocationServiceListener, CoordinateSegmentation::Listener, MessageRecipient, MigrationDataClient, public PollingService {
public:
    // MAIN Thread: All public interface is expected to be called only from the main thread.
    typedef Prox::Query<ProxSimulationTraits> Query;
    typedef Prox::QueryEvent<ProxSimulationTraits> QueryEvent;

    Proximity(SpaceContext* ctx, LocationService* locservice);
    ~Proximity();

    // Initialize prox.  Must be called after everything else (specifically message router) is set up since it
    // needs to send messages.
    void initialize(CoordinateSegmentation* cseg);

    // Shutdown the proximity thread.
    void shutdown();

    // Objects
    void addQuery(UUID obj, SolidAngle sa);
    void removeQuery(UUID obj);

    // QueryEventListener Interface
    void queryHasEvents(Query* query);

    // LocationServiceListener Interface
    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void localObjectRemoved(const UUID& uuid);
    virtual void localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);

    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg);

    // MessageRecipient Interface
    virtual void receiveMessage(Message* msg);

    // MigrationDataClient Interface
    virtual std::string migrationClientTag();
    virtual std::string generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server);
    virtual void receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data);

private:

    // MAIN Thread: These are utility methods which should only be called from the main thread.

    // Update queries based on current state.
    void poll();

    // Server queries requests, generated by receiving messages
    void updateQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa);
    void removeQuery(ServerID sid);

    // Object queries
    void updateQuery(UUID obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa);

    // Setup all known servers for a server query update
    void addAllServersForUpdate();

    // Send a query add/update request to all the other servers
    void sendQueryRequests();

    // Handle events generated by the prox thread that require operations in the main thread
    void handleOutputEvent(const ProximityOutputEvent& evt);


    // PROX Thread: These are utility methods which should only be called from the prox thread.
    // The main loop for the prox processing thread
    void proxThreadMain();
    // Checks all inputs from main thread
    void handleInputEvents();
    // Handles events in the prox thread
    void handleInputEvent(const ProximityInputEvent& evt);
    // Generate query events based on results collected from query handlers
    void generateServerQueryEvents();
    void generateObjectQueryEvents();

    typedef std::set<UUID> ObjectSet;
    typedef std::map<ServerID, Query*> ServerQueryMap;
    typedef std::map<UUID, Query*> ObjectQueryMap;


    SpaceContext* mContext;

    // MAIN Thread - Should only be accessed in methods used by the main thread

    LocationService* mLocService;
    CoordinateSegmentation* mCSeg;

    // Tracks object query angles for quick access in the main thread
    // NOTE: It really sucks that we're duplicating this information
    // but we'd have to provide a safe query map and query angle accessor
    // to avoid this.
    typedef std::map<UUID, SolidAngle> ObjectQueryAngleMap;
    ObjectQueryAngleMap mObjectQueryAngles;

    // This tracks the minimum object query size, which is used
    // as the angle for queries to other servers.
    SolidAngle mMinObjectQueryAngle;
    // And this indicates whether we need to send new requests
    // out to other servers
    std::set<ServerID> mNeedServerQueryUpdate;

    std::deque<Message*> mServerResultsToSend; // server query results waiting to be sent
    std::deque<CBR::Protocol::Object::ObjectMessage*> mObjectResultsToSend; // object query results waiting to be sent


    // PROX Thread - Should only be accessed in methods used by the main thread

    typedef Prox::QueryHandler<ProxSimulationTraits> ProxQueryHandler;
    void tickQueryHandler(ProxQueryHandler* qh);

    boost::thread* mProxThread;
    IOService* mProxService;
    IOStrand* mProxStrand;
    Sirikata::AtomicValue<bool> mShutdownProxThread;

    // These track local objects and answer queries from other
    // servers.
    ServerQueryMap mServerQueries;
    CBRLocationServiceCache* mLocalLocCache;
    ProxQueryHandler* mServerQueryHandler;

    // These track all objects being reported to this server and
    // answer queries for objects connected to this server.
    ObjectQueryMap mObjectQueries;
    CBRLocationServiceCache* mGlobalLocCache;
    ProxQueryHandler* mObjectQueryHandler;

    // Threads: Thread-safe data used for exchange between threads

    Sirikata::ThreadSafeQueue<ProximityInputEvent> mInputEvents; // events from main thread that need to be handled in prox thread
    Sirikata::ThreadSafeQueue<ProximityOutputEvent> mOutputEvents; // events from prox thread that need to be handled in main thread
    // Lists which allow requests/responses to cross thread boundaries
    Sirikata::ThreadSafeQueue<Message*> mServerResults; // server query results that need to be sent
    Sirikata::ThreadSafeQueue<CBR::Protocol::Object::ObjectMessage*> mObjectResults; // object query results that need to be sent

}; //class Proximity

} // namespace CBR

#endif //_CBR_PROXIMITY_HPP_
