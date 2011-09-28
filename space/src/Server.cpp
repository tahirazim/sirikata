/*  Sirikata
 *  Server.cpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#include <sirikata/space/SpaceNetwork.hpp>
#include "Server.hpp"
#include <sirikata/space/Proximity.hpp>
#include <sirikata/space/CoordinateSegmentation.hpp>
#include <sirikata/space/ServerMessage.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/space/Authenticator.hpp>
#include "Forwarder.hpp"
#include "LocalForwarder.hpp"
#include "MigrationMonitor.hpp"

#include <sirikata/space/ObjectSegmentation.hpp>

#include "ObjectConnection.hpp"
#include "ObjectHostConnectionManager.hpp"

#include <sirikata/core/util/Random.hpp>

#include <iostream>
#include <iomanip>

#include <sirikata/core/network/IOStrandImpl.hpp>

#include <sirikata/core/odp/DelegatePort.hpp>

#define SPACE_LOG(lvl,msg) SILOG(space, lvl, msg)

namespace Sirikata
{

namespace {
// Helper for filling in version info for connection responses
void fillVersionInfo(Sirikata::Protocol::Session::IVersionInfo vers_info, SpaceContext* ctx) {
    vers_info.set_name(ctx->name());
    vers_info.set_version(SIRIKATA_VERSION);
    vers_info.set_major(SIRIKATA_VERSION_MAJOR);
    vers_info.set_minor(SIRIKATA_VERSION_MINOR);
    vers_info.set_revision(SIRIKATA_VERSION_REVISION);
    vers_info.set_vcs_version(SIRIKATA_GIT_REVISION);
}
void logVersionInfo(Sirikata::Protocol::Session::VersionInfo vers_info) {
    SPACE_LOG(info, "Object host connection " << (vers_info.has_name() ? vers_info.name() : "(unknown)") << " version " << (vers_info.has_version() ? vers_info.version() : "(unknown)") << " (" << (vers_info.has_vcs_version() ? vers_info.vcs_version() : "") << ")");
}
} // namespace


Server::Server(SpaceContext* ctx, Authenticator* auth, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectSegmentation* oseg, Address4 oh_listen_addr)
 : ODP::DelegateService( std::tr1::bind(&Server::createDelegateODPPort, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, std::tr1::placeholders::_3) ),
   mContext(ctx),
   mAuthenticator(auth),
   mLocationService(loc_service),
   mCSeg(cseg),
   mProximity(prox),
   mOSeg(oseg),
   mLocalForwarder(NULL),
   mForwarder(forwarder),
   mMigrationMonitor(),
   mMigrationSendRunning(false),
   mShutdownRequested(false),
   mObjectHostConnectionManager(NULL),
   mRouteObjectMessage(Sirikata::SizedResourceMonitor(GetOptionValue<size_t>("route-object-message-buffer"))),
   mTimeSeriesObjects(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".objects")
{
    mContext->mCSeg = mCSeg;
    mContext->mObjectSessionManager = this;

    this->addListener(static_cast<ObjectSessionListener*>(mLocationService));
    this->addListener(static_cast<ObjectSessionListener*>(mProximity));

    mTimeSyncServer = new TimeSyncServer(mContext);

    mMigrateServerMessageService = mForwarder->createServerMessageService("migrate");

    mForwarder->registerMessageRecipient(SERVER_PORT_MIGRATION, this);
    mForwarder->setODPService(this);

      mOSeg->setWriteListener((OSegWriteListener*)this);

      mMigrationMonitor = new MigrationMonitor(
          mContext, mLocationService, mCSeg,
          mContext->mainStrand->wrap(
              std::tr1::bind(&Server::handleMigrationEvent, this, std::tr1::placeholders::_1)
          )
      );

    mObjectHostConnectionManager = new ObjectHostConnectionManager(
        mContext, oh_listen_addr,
        std::tr1::bind(&Server::handleObjectHostMessage, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2),
        mContext->mainStrand->wrap(std::tr1::bind(&Server::handleObjectHostConnectionClosed, this, std::tr1::placeholders::_1))
    );

    mLocalForwarder = new LocalForwarder(mContext);
    mForwarder->setLocalForwarder(mLocalForwarder);

    mMigrationTimer.start();


    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    mContext->sstConnectionManager()->listen(
        std::tr1::bind(&Server::newStream, this, _1, _2),
        EndPoint<SpaceObjectReference>(SpaceObjectReference(SpaceID::null(), ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT)
    );
}

void Server::newStream(int err, Stream<SpaceObjectReference>::Ptr s) {
  if (err != SST_IMPL_SUCCESS){
    return;
  }

  // If we've lost the object's connection, we should just ignore this
  ObjectReference objid = s->remoteEndPoint().endPoint.object();
  if (mObjects.find(objid.getAsUUID()) == mObjects.end()) {
      s->close(false);
      return;
  }

  // Otherwise, they have a complete session
  ObjectSession* new_obj_session = new ObjectSession(objid, s);
  mObjectSessions[objid] = new_obj_session;
  notify(&ObjectSessionListener::newSession, new_obj_session);
}

Server::~Server()
{
    delete mMigrateServerMessageService;

    mForwarder->unregisterMessageRecipient(SERVER_PORT_MIGRATION, this);

    SPACE_LOG(debug, "mObjects.size=" << mObjects.size());

    for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        UUID obj_id = it->first;

        // Stop any proximity queries for this object
        mProximity->removeQuery(obj_id);

        mLocationService->removeLocalObject(obj_id);

        // Stop Forwarder from delivering via this Object's
        // connection, destroy said connection
        mForwarder->removeObjectConnection(obj_id);

        // FIXME there's probably quite a bit more cleanup to do here
    }
    mObjects.clear();

    delete mObjectHostConnectionManager;
    delete mLocalForwarder;

    delete mMigrationMonitor;

    delete mTimeSyncServer;
}

ODP::DelegatePort* Server::createDelegateODPPort(DelegateService*, const SpaceObjectReference& sor, ODP::PortID port) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    ODP::Endpoint port_ep(sor, port);
    return new ODP::DelegatePort(
        (ODP::DelegateService*)this,
        port_ep,
        std::tr1::bind(
            &Server::delegateODPPortSend, this, port_ep, _1, _2
        )
    );
}

bool Server::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload) {
    // Create new ObjectMessage
    Sirikata::Protocol::Object::ObjectMessage* msg =
        createObjectMessage(
            mContext->id(),
            source_ep.object().getAsUUID(), source_ep.port(),
            dest_ep.object().getAsUUID(), dest_ep.port(),
            String((char*)payload.data(), payload.size())
        );


    // This call needs to be thread safe, and we shouldn't be using this
    // ODP::Service to communicate with any non-local objects, so just use the
    // local forwarder.
    bool send_success = mLocalForwarder->tryForward(msg);

    // If the send failed, we need to destroy the message.
    if (!send_success)
        delete msg;

    return send_success;
}

ObjectSession* Server::getSession(const ObjectReference& objid) const {
    ObjectSessionMap::const_iterator it = mObjectSessions.find(objid);
    if (it == mObjectSessions.end()) return NULL;
    return it->second;
}

bool Server::isObjectConnected(const UUID& object_id) const {
    return (mObjects.find(object_id) != mObjects.end());
}

bool Server::isObjectConnecting(const UUID& object_id) const {
    return (mStoredConnectionData.find(object_id) != mStoredConnectionData.end());
}

void Server::sendSessionMessageWithRetry(const ObjectHostConnectionManager::ConnectionID& conn, Sirikata::Protocol::Object::ObjectMessage* msg, const Duration& retry_rate) {
    bool sent = mObjectHostConnectionManager->send( conn, msg );
    if (!sent) {
        mContext->mainStrand->post(
            retry_rate,
            std::tr1::bind(&Server::sendSessionMessageWithRetry, this, conn, msg, retry_rate)
        );
    }
}

bool Server::handleObjectHostMessage(const ObjectHostConnectionManager::ConnectionID& conn_id, Sirikata::Protocol::Object::ObjectMessage* obj_msg) {
    static UUID spaceID = UUID::null();

    // Before admitting a message, we need to do some sanity checks.  Also, some types of messages get
    // exceptions for bootstrapping purposes (namely session messages to the space).

    // 2. For connection bootstrapping purposes we need to exempt session messages destined for the space.
    // Note that we need to check this before the connected sanity check since obviously the object won't
    // be connected yet.  We dispatch directly from here since this needs information about the object host
    // connection to be passed along as well.
    bool session_msg = (obj_msg->dest_port() == OBJECT_PORT_SESSION);
    if (session_msg)
    {
        bool space_dest = (obj_msg->dest_object() == spaceID);

        // FIXME infinite queue
        mContext->mainStrand->post(
            std::tr1::bind(
                &Server::handleSessionMessage, this,
                conn_id, obj_msg
            )
        );
        return true;
    }

    // 3. Try to shortcut the main thread. Let the LocalForwarder try
    // to ship it over a connection.  This checks both the source
    // and dest objects, guaranteeing that the appropriate connections
    // exist for both.
    if (mLocalForwarder->tryForward(obj_msg))
        return true;

    // 4. Try to shortcut them main thread. Use forwarder to try to forward
    // using the cache. FIXME when we do this, we skip over some checks that
    // happen during the full forwarding
    if (mForwarder->tryCacheForward(obj_msg))
        return true;

    // 1. If the source is the space, somebody is messing with us.  Only need to
    // check this here since otherwise the other paths should fail.
    bool space_source = (obj_msg->source_object() == spaceID);
    if (space_source) {
        SILOG(cbr,error,"Got message from object host claiming to be from space.");
        delete obj_msg;
        return true;
    }

    // If it is is TimeSync message, dispatch immediately
    bool timesync_msg = (obj_msg->dest_port() == OBJECT_PORT_TIMESYNC);
    if (timesync_msg)
    {
        bool space_dest = (obj_msg->dest_object() == spaceID);
        if (space_dest) {
            String response_payload = mTimeSyncServer->getResponse(MemoryReference(obj_msg->payload()));
            if (!response_payload.empty()) {
                Sirikata::Protocol::Object::ObjectMessage* sync_response = createObjectMessage(
                    mContext->id(),
                    UUID::null(), OBJECT_PORT_TIMESYNC,
                    obj_msg->source_object(), OBJECT_PORT_TIMESYNC,
                    response_payload
                );
                bool send_success = mObjectHostConnectionManager->send(conn_id, sync_response);
                if (!send_success) delete sync_response;
            }
            delete obj_msg;
            return true;
        }
    }

    // 5. Otherwise, we're going to have to ship this to the main thread, either
    // for handling session messages, messages to the space, or to make a
    // routing decision.
    bool hit_empty;
    bool push_for_processing_success;
    {
        boost::lock_guard<boost::mutex> lock(mRouteObjectMessageMutex);
        hit_empty = (mRouteObjectMessage.probablyEmpty());
        push_for_processing_success = mRouteObjectMessage.push(ConnectionIDObjectMessagePair(conn_id,obj_msg),false);
    }
    if (!push_for_processing_success) {
        TIMESTAMP(obj_msg, Trace::SPACE_DROPPED_AT_MAIN_STRAND_CROSSING);
        TRACE_DROP(SPACE_DROPPED_AT_MAIN_STRAND_CROSSING);
        delete obj_msg;
    } else {
        if (hit_empty)
            scheduleObjectHostMessageRouting();
    }

    // NOTE: We always "accept" the data, even if we're just dropping
    // it.  This keeps packets flowing.  We could use flow control to
    // slow things down, but since the data path splits in this method
    // between local and remote, we don't want to slow the local
    // packets just because of a backup in routing.
    return true;
}

void Server::scheduleObjectHostMessageRouting() {
    mContext->mainStrand->post(
        std::tr1::bind(
            &Server::handleObjectHostMessageRouting,
            this));
}

void Server::handleObjectHostMessageRouting() {
#define MAX_OH_MESSAGES_HANDLED 100

    for(uint32 i = 0; i < MAX_OH_MESSAGES_HANDLED; i++)
        if (!handleSingleObjectHostMessageRouting())
            break;

    {
        boost::lock_guard<boost::mutex> lock(mRouteObjectMessageMutex);
        if (!mRouteObjectMessage.probablyEmpty())
            scheduleObjectHostMessageRouting();
    }
}

bool Server::handleSingleObjectHostMessageRouting() {
    ConnectionIDObjectMessagePair front(ObjectHostConnectionManager::ConnectionID(),NULL);
    if (!mRouteObjectMessage.pop(front))
        return false;

    // If we don't have a connection for the source object, we can't do anything with it.
    // The object could be migrating and we get outdated packets.  Currently this can
    // happen because we need to maintain the connection long enough to deliver the init migration
    // message.  Therefore, we check if its in the currently migrating connections as well as active
    // connections and allow messages through.
    // NOTE that we check connecting objects as well since we need to get past this point to deliver
    // Session messages.
    UUID source_object = front.obj_msg->source_object();
    bool source_connected =
        mObjects.find(source_object) != mObjects.end() ||
        mMigratingConnections.find(source_object) != mMigratingConnections.end();
    if (!source_connected)
    {
        if (mObjectsAwaitingMigration.find(source_object) == mObjectsAwaitingMigration.end() &&
            mObjectMigrations.find(source_object) == mObjectMigrations.end())
        {
            SILOG(cbr,warn,"Got message for unknown object: " << source_object.toString());
        }
        else
        {
            SILOG(cbr,warn,"Server got message from object after migration started: " << source_object.toString());
        }

        delete front.obj_msg;

        return true;
    }


    // Finally, if we've passed all these tests, then everything looks good and we can route it
    mForwarder->routeObjectHostMessage(front.obj_msg);
    return true;
}

// Handle Session messages from an object
void Server::handleSessionMessage(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, Sirikata::Protocol::Object::ObjectMessage* msg) {
    Sirikata::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromString(msg->payload());
    if (!parse_success) {
        LOG_INVALID_MESSAGE(space, error, msg->payload());
        delete msg;
        return;
    }

    // Connect or migrate messages
    if (session_msg.has_connect()) {
        if (session_msg.connect().has_version())
            logVersionInfo(session_msg.connect().version());

        if (session_msg.connect().type() == Sirikata::Protocol::Session::Connect::Fresh)
        {
            handleConnect(oh_conn_id, *msg, session_msg.connect());
        }
        else if (session_msg.connect().type() == Sirikata::Protocol::Session::Connect::Migration)
        {
            handleMigrate(oh_conn_id, *msg, session_msg.connect());
        }
        else
            SILOG(space,error,"Unknown connection message type");
    }
    else if (session_msg.has_connect_ack()) {
        handleConnectAck(oh_conn_id, *msg);
    }
    else if (session_msg.has_disconnect()) {
        ObjectConnectionMap::iterator it = mObjects.find(session_msg.disconnect().object());
        if (it != mObjects.end()) {
            handleDisconnect(session_msg.disconnect().object(), it->second);
            mContext->timeSeries->report(mTimeSeriesObjects, mObjects.size());
        }
    }

    // InitiateMigration messages
    assert(!session_msg.has_connect_response());
    assert(!session_msg.has_init_migration());

    delete msg;
}

void Server::handleObjectHostConnectionClosed(const ObjectHostConnectionManager::ConnectionID& oh_conn_id) {
    for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); ) {
        UUID obj_id = it->first;
        ObjectConnection* obj_conn = it->second;

        it++; // Iterator might get erased in handleDisconnect

        if (obj_conn->connID() != oh_conn_id)
            continue;

        handleDisconnect(obj_id, obj_conn);
    }
    mContext->timeSeries->report(mTimeSeriesObjects, mObjects.size());
}

void Server::retryHandleConnect(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, Sirikata::Protocol::Object::ObjectMessage* obj_response) {
    if (!mObjectHostConnectionManager->send(oh_conn_id,obj_response)) {
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&Server::retryHandleConnect,this,oh_conn_id,obj_response));
    }else {

    }
}

void Server::sendConnectError(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const UUID& obj_id) {
    Sirikata::Protocol::Session::Container response_container;
    Sirikata::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
    fillVersionInfo(response.mutable_version(), mContext);
    response.set_response( Sirikata::Protocol::Session::ConnectResponse::Error );

    Sirikata::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response_container)
    );

    // Sent directly via object host connection manager because we don't have an ObjectConnection
    if (!mObjectHostConnectionManager->send( oh_conn_id, obj_response )) {
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&Server::retryHandleConnect,this,oh_conn_id,obj_response));
    }
}

// Handle Connect message from object
void Server::handleConnect(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const Sirikata::Protocol::Object::ObjectMessage& container, const Sirikata::Protocol::Session::Connect& connect_msg) {
    UUID obj_id = container.source_object();

    // If the requested location isn't on this server, redirect
    // Note: on connections, we always ignore the specified time and just use
    // our local time.  The client is aware of this and handles it properly.
    TimedMotionVector3f loc( mContext->simTime(), MotionVector3f(connect_msg.loc().position(), connect_msg.loc().velocity()) );
    Vector3f curpos = loc.extrapolate(mContext->simTime()).position();
    bool in_server_region = mMigrationMonitor->onThisServer(curpos);
    ServerID loc_server = mCSeg->lookup(curpos);

    if(loc_server == NullServerID || (loc_server == mContext->id() && !in_server_region)) {
        // Either CSeg says no server handles the specified region or
        // that we should, but it doesn't actually land in our region
        // (i.e. things were probably clamped invalidly)

        if (loc_server == NullServerID)
            SILOG(cbr,warn,"[SPACE] Connecting object specified location outside of all regions.");
        else if (loc_server == mContext->id() && !in_server_region)
            SILOG(cbr,warn,"[SPACE] Connecting object was incorrectly determined to be in our region.");

        // Create and send error reply
        sendConnectError(oh_conn_id, obj_id);
        return;
    }

    if (loc_server != mContext->id()) {
        // Since we passed the previous test, this just means they tried to connect
        // to the wrong server => redirect

        // Create and send redirect reply
        Sirikata::Protocol::Session::Container response_container;
        Sirikata::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
        fillVersionInfo(response.mutable_version(), mContext);
        response.set_response( Sirikata::Protocol::Session::ConnectResponse::Redirect );
        response.set_redirect(loc_server);

        Sirikata::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
            mContext->id(),
            UUID::null(), OBJECT_PORT_SESSION,
            obj_id, OBJECT_PORT_SESSION,
            serializePBJMessage(response_container)
        );


        // Sent directly via object host connection manager because we don't have an ObjectConnection
        if (!mObjectHostConnectionManager->send( oh_conn_id, obj_response )) {
            mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&Server::retryHandleConnect,this,oh_conn_id,obj_response));
        }
        return;
    }

    // FIXME sanity check the new connection
    // -- verify object may connect, i.e. not already in system (e.g. check oseg)

    String auth_data = "";
    if (connect_msg.has_auth())
        auth_data = connect_msg.auth();
    mAuthenticator->authenticate(
        obj_id, MemoryReference(auth_data),
        std::tr1::bind(&Server::handleConnectAuthResponse, this, oh_conn_id, obj_id, connect_msg, std::tr1::placeholders::_1)
    );
}

void Server::handleConnectAuthResponse(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const UUID& obj_id, const Sirikata::Protocol::Session::Connect& connect_msg, bool authenticated) {
    if (!authenticated) {
        sendConnectError(oh_conn_id, obj_id);
        return;
    }

    // Because of unreliable messaging, we might get a double connect request
    // (if we got the initial request but the response was dropped). In that
    // case, just send them another one and ignore this
    // request. Alternatively, someone might just be trying to use the
    // same object ID.
    if (isObjectConnected(obj_id) || isObjectConnecting(obj_id)) {
        // Decide whether this is a conflict or a retry
        if  //was already connected and it was the same oh sending msg
            (isObjectConnected(obj_id) &&
                (mObjects[obj_id]->connID() == oh_conn_id))
        {
            // retry, tell them they're fine.
            sendConnectSuccess(oh_conn_id, obj_id);
        }
        else if
            // or was connecting and was the same oh sending message
            (isObjectConnecting(obj_id) &&
                mStoredConnectionData[obj_id].conn_id == oh_conn_id)
        {
            // Do nothing, they're still working on the connection. We can't
            // send success (they aren't fully connected yet) and we can't send
            // failure (they might still succeed).
        }
        else
        {
            // conflict, fail the new connection leaving existing alone
            sendConnectError(oh_conn_id, obj_id);
        }

        return;
    }

    // Update our oseg to show that we know that we have this object now. Also
    // mark it as connecting (by storing in mStoredConnectionData) so any
    // additional connection attempts will fail.
    StoredConnection sc;
    sc.conn_id = oh_conn_id;
    sc.conn_msg = connect_msg;
    mStoredConnectionData[obj_id] = sc;

    mOSeg->addNewObject(obj_id,connect_msg.bounds().radius());
}

void Server::finishAddObject(const UUID& obj_id, OSegAddNewStatus status)
{
  StoredConnectionMap::iterator storedConIter = mStoredConnectionData.find(obj_id);
  if (storedConIter != mStoredConnectionData.end())
  {
      StoredConnection sc = mStoredConnectionData[obj_id];
      if (status == OSegWriteListener::SUCCESS)
      {
          // Note: we always use local time for connections. The client
          // accounts for by using the values we return in the response
          // instead of the original values sent with the connection
          // request.
          Time local_t = mContext->simTime();
          TimedMotionVector3f loc( local_t, MotionVector3f(sc.conn_msg.loc().position(), sc.conn_msg.loc().velocity()) );
          TimedMotionQuaternion orient(
              local_t,
              MotionQuaternion( sc.conn_msg.orientation().position(), sc.conn_msg.orientation().velocity() )
          );
          BoundingSphere3f bnds = sc.conn_msg.bounds();
          // Create and store the connection
          ObjectConnection* conn = new ObjectConnection(obj_id, mObjectHostConnectionManager, sc.conn_id);
          mObjects[obj_id] = conn;
          mContext->timeSeries->report(mTimeSeriesObjects, mObjects.size());

          //TODO: assumes each server process is assigned only one region... perhaps we should enforce this constraint
          //for cleaner semantics?
          mCSeg->reportLoad(mContext->id(), mCSeg->serverRegion(mContext->id())[0] , mObjects.size()  );

          mLocalForwarder->addActiveConnection(conn);

          // Add object as local object to LocationService
          String obj_mesh = sc.conn_msg.has_mesh() ? sc.conn_msg.mesh() : "";
          String obj_phy = sc.conn_msg.has_physics() ? sc.conn_msg.physics() : "";
          mLocationService->addLocalObject(obj_id, loc, orient, bnds, obj_mesh, obj_phy);

          // Register proximity query
          uint32 query_max_results = 0;
          if (sc.conn_msg.has_query_max_count() && sc.conn_msg.query_max_count() > 0)
              query_max_results = sc.conn_msg.query_max_count();
          if (sc.conn_msg.has_query_angle())
              mProximity->addQuery(obj_id, SolidAngle(sc.conn_msg.query_angle()), query_max_results);

          // Stage the connection with the forwarder, but don't enable it until an ack is received
          mForwarder->addObjectConnection(obj_id, conn);

          sendConnectSuccess(conn->connID(), obj_id);
      }
      else
      {
          sendConnectError(sc.conn_id , obj_id);
      }
      mStoredConnectionData.erase(storedConIter);
  }
  else
  {
      SILOG(space,error,"No stored connection data for object " << obj_id.toString());
  }
}

void Server::sendConnectSuccess(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const UUID& obj_id) {
    TimedMotionVector3f loc = mLocationService->location(obj_id);
    TimedMotionQuaternion orient = mLocationService->orientation(obj_id);
    BoundingSphere3f bnds = mLocationService->bounds(obj_id);
    String obj_mesh = mLocationService->mesh(obj_id);

    // Send reply back indicating that the connection was successful
    Sirikata::Protocol::Session::Container response_container;
    Sirikata::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
    fillVersionInfo(response.mutable_version(), mContext);
    response.set_response( Sirikata::Protocol::Session::ConnectResponse::Success );
    Sirikata::Protocol::ITimedMotionVector resp_loc = response.mutable_loc();
    resp_loc.set_t( loc.updateTime() );
    resp_loc.set_position( loc.position() );
    resp_loc.set_velocity( loc.velocity() );
    Sirikata::Protocol::ITimedMotionQuaternion resp_orient = response.mutable_orientation();
    resp_orient.set_t( orient.updateTime() );
    resp_orient.set_position( orient.position() );
    resp_orient.set_velocity( orient.velocity() );
    response.set_bounds(bnds);
    response.set_mesh(obj_mesh);

    Sirikata::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response_container)
    );
    // Sent directly via object host connection manager because ObjectConnection isn't enabled yet
    sendSessionMessageWithRetry(oh_conn_id, obj_response, Duration::seconds(0.05));
}

// Handle Migrate message from object
//this is called by the receiving server.
void Server::handleMigrate(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const Sirikata::Protocol::Object::ObjectMessage& container, const Sirikata::Protocol::Session::Connect& migrate_msg)
{
    UUID obj_id = container.source_object();

    assert( !isObjectConnected(obj_id) );

    // FIXME sanity check the new connection
    // -- authentication
    // -- verify object may connect, i.e. not already in system (e.g. check oseg)
    // Verify the requested position is on this server

    // Create and store the connection
    ObjectConnection* conn = new ObjectConnection(obj_id, mObjectHostConnectionManager, oh_conn_id);
    mObjectsAwaitingMigration[obj_id] = conn;

    // Try to handle this migration if all info is available

    SILOG(space,detailed,"Received migration message from " << obj_id.toString());

    handleMigration(obj_id);

    //    handleMigration(migrate_msg.object());

}

void Server::handleConnectAck(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const Sirikata::Protocol::Object::ObjectMessage& container) {
    UUID obj_id = container.source_object();

    // Allow the forwarder to send to ship messages to this connection
    mForwarder->enableObjectConnection(obj_id);
}

// Note that the obj_id is intentionally not a const & so that we're sure it is
// valid throughout this method.
void Server::handleDisconnect(UUID obj_id, ObjectConnection* conn) {
    assert(conn->id() == obj_id);

    mOSeg->removeObject(obj_id);
    mLocalForwarder->removeActiveConnection(obj_id);
    mLocationService->removeLocalObject(obj_id);

    // Register proximity query
    mProximity->removeQuery(obj_id);

    mForwarder->removeObjectConnection(obj_id);

    mObjects.erase(obj_id);
    // Num objects is reported by the caller

    ObjectReference obj(obj_id);
    ObjectSessionMap::iterator session_it = mObjectSessions.find(obj);
    if (session_it != mObjectSessions.end()) {
        notify(&ObjectSessionListener::sessionClosed, session_it->second);
        delete session_it->second;
        mObjectSessions.erase(session_it);
    }

    delete conn;
}

void Server::osegAddNewFinished(const UUID& id, OSegAddNewStatus status) {
    // Indicates an update to OSeg finished, meaning a migration can
    // continue.
    mContext->mainStrand->post(
        std::tr1::bind(&Server::finishAddObject, this, id, status)
                               );
}

void Server::osegMigrationAcknowledged(const UUID& id) {
    // Indicates its safe to destroy the object connection since the migration
    // was successful
    mContext->mainStrand->post(
        std::tr1::bind(&Server::killObjectConnection, this, id)
                               );
}

void Server::receiveMessage(Message* msg)
{
    if (msg->dest_port() == SERVER_PORT_MIGRATION) {
        Sirikata::Protocol::Migration::MigrationMessage* mig_msg = new Sirikata::Protocol::Migration::MigrationMessage();
        bool parsed = parsePBJMessage(mig_msg, msg->payload());

        if (!parsed) {
            delete mig_msg;
        }
        else {
            const UUID obj_id = mig_msg->object();

            SILOG(space,detailed,"Received server migration message for " << obj_id.toString() << " from server " << mig_msg->source_server());

            mObjectMigrations[obj_id] = mig_msg;
            // Try to handle this migration if all the info is available
            handleMigration(obj_id);
        }
        delete msg;
    }
}

//handleMigration to this server.
void Server::handleMigration(const UUID& obj_id)
{
    if (checkAlreadyMigrating(obj_id))
    {
      processAlreadyMigrating(obj_id);
      return;
    }

    // Try to find the info in both lists -- the connection and migration information

    ObjectConnectionMap::iterator obj_map_it = mObjectsAwaitingMigration.find(obj_id);
    if (obj_map_it == mObjectsAwaitingMigration.end())
    {
        return;
    }


    ObjectMigrationMap::iterator migration_map_it = mObjectMigrations.find(obj_id);
    if (migration_map_it == mObjectMigrations.end())
    {
        return;
    }


    SILOG(space,detailed,"Finishing migration of " << obj_id.toString());

    // Get the data from the two maps
    ObjectConnection* obj_conn = obj_map_it->second;
    Sirikata::Protocol::Migration::MigrationMessage* migrate_msg = migration_map_it->second;


    // Extract the migration message data
    TimedMotionVector3f obj_loc(
        migrate_msg->loc().t(),
        MotionVector3f( migrate_msg->loc().position(), migrate_msg->loc().velocity() )
    );
    TimedMotionQuaternion obj_orient(
        migrate_msg->orientation().t(),
        MotionQuaternion( migrate_msg->orientation().position(), migrate_msg->orientation().velocity() )
    );
    BoundingSphere3f obj_bounds( migrate_msg->bounds() );
    String obj_mesh ( migrate_msg->has_mesh() ? migrate_msg->mesh() : "");
    String obj_phy ( migrate_msg->has_physics() ? migrate_msg->physics() : "");

    // Move from list waiting for migration message to active objects
    mObjects[obj_id] = obj_conn;
    mContext->timeSeries->report(mTimeSeriesObjects, mObjects.size());
    mLocalForwarder->addActiveConnection(obj_conn);


    // Update LOC to indicate we have this object locally
    mLocationService->addLocalObject(obj_id, obj_loc, obj_orient, obj_bounds, obj_mesh, obj_phy);

    //update our oseg to show that we know that we have this object now.
    ServerID idOSegAckTo = (ServerID)migrate_msg->source_server();
    mOSeg->addMigratedObject(obj_id, obj_bounds.radius(), idOSegAckTo, true);//true states to send an ack message to idOSegAckTo


    // Handle any data packed into the migration message for space components
    for(int32 i = 0; i < migrate_msg->client_data_size(); i++) {
        Sirikata::Protocol::Migration::MigrationClientData client_data = migrate_msg->client_data(i);
        std::string tag = client_data.key();
        // FIXME these should live in a map, how do we deal with ordering constraints?
        if (tag == "prox") {
            assert( tag == mProximity->migrationClientTag() );
            mProximity->receiveMigrationData(obj_id, /* FIXME */NullServerID, mContext->id(), client_data.data());
        }
        else {
            SILOG(space,error,"Got unknown tag for client migration data");
        }
    }

    // Stage this connection with the forwarder, don't enable until ack is received
    mForwarder->addObjectConnection(obj_id, obj_conn);


    // Clean out the two records from the migration maps
    mObjectsAwaitingMigration.erase(obj_map_it);
    mObjectMigrations.erase(migration_map_it);


    // Send reply back indicating that the migration was successful
    Sirikata::Protocol::Session::Container response_container;
    Sirikata::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
    fillVersionInfo(response.mutable_version(), mContext);
    response.set_response( Sirikata::Protocol::Session::ConnectResponse::Success );

    Sirikata::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response_container)
    );
    // Sent directly via object host connection manager because ObjectConnection isn't enabled yet
    sendSessionMessageWithRetry(obj_conn->connID(), obj_response, Duration::seconds(0.05));
}

void Server::start() {
    mForwarder->start();
}

void Server::stop() {
    mForwarder->stop();
    mObjectHostConnectionManager->shutdown();
    mShutdownRequested = true;
}

void Server::handleMigrationEvent(const UUID& obj_id) {
    // * wrap up state and send message to other server
    //     to reinstantiate the object there
    // * delete object on this side

    // Make sure we aren't getting an out of date event
    // FIXME

    if (mOSeg->clearToMigrate(obj_id)) //needs to check whether migration to this server has finished before can begin migrating to another server.
    {
        ObjectConnection* obj_conn = mObjects[obj_id];

        Vector3f obj_pos = mLocationService->currentPosition(obj_id);
        ServerID new_server_id = mCSeg->lookup(obj_pos);

        // FIXME should be this
        //assert(new_server_id != mContext->id());
        // but I'm getting inconsistencies, so we have to just trust CSeg to have the final say
        if (new_server_id != mContext->id()) {

            SILOG(space,detailed,"Starting migration of " << obj_id.toString() << " from " << mContext->id() << " to " << new_server_id);

            Sirikata::Protocol::Session::Container session_msg;
            Sirikata::Protocol::Session::IInitiateMigration init_migration_msg = session_msg.mutable_init_migration();
            init_migration_msg.set_new_server( (uint64)new_server_id );
            Sirikata::Protocol::Object::ObjectMessage* init_migr_obj_msg = createObjectMessage(
                mContext->id(),
                UUID::null(), OBJECT_PORT_SESSION,
                obj_id, OBJECT_PORT_SESSION,
                serializePBJMessage(session_msg)
            );
            // Sent directly via object host connection manager because ObjectConnection is disappearing
            sendSessionMessageWithRetry(obj_conn->connID(), init_migr_obj_msg, Duration::seconds(0.05));
            BoundingSphere3f obj_bounds=mLocationService->bounds(obj_id);
            mOSeg->migrateObject(obj_id,OSegEntry(new_server_id,obj_bounds.radius()));

            // Send out the migrate message
            Sirikata::Protocol::Migration::MigrationMessage migrate_msg;
            migrate_msg.set_source_server(mContext->id());
            migrate_msg.set_object(obj_id);
            Sirikata::Protocol::ITimedMotionVector migrate_loc = migrate_msg.mutable_loc();
            TimedMotionVector3f obj_loc = mLocationService->location(obj_id);
            migrate_loc.set_t( obj_loc.updateTime() );
            migrate_loc.set_position( obj_loc.position() );
            migrate_loc.set_velocity( obj_loc.velocity() );
            Sirikata::Protocol::ITimedMotionQuaternion migrate_orient = migrate_msg.mutable_orientation();
            TimedMotionQuaternion obj_orient = mLocationService->orientation(obj_id);
            migrate_orient.set_t( obj_orient.updateTime() );
            migrate_orient.set_position( obj_orient.position() );
            migrate_orient.set_velocity( obj_orient.velocity() );
            migrate_msg.set_bounds( obj_bounds );
            String obj_mesh = mLocationService->mesh(obj_id);
            if (obj_mesh.size() > 0)
                migrate_msg.set_mesh( obj_mesh );

            // FIXME we should allow components to package up state here
            // FIXME we should generate these from some map instead of directly
            std::string prox_data = mProximity->generateMigrationData(obj_id, mContext->id(), new_server_id);
            if (!prox_data.empty()) {
                Sirikata::Protocol::Migration::IMigrationClientData client_data = migrate_msg.add_client_data();
                client_data.set_key( mProximity->migrationClientTag() );
                client_data.set_data( prox_data );
            }

            // Stop tracking the object locally
            //            mLocationService->removeLocalObject(obj_id);
            Message* migrate_msg_packet = new Message(
                mContext->id(),
                SERVER_PORT_MIGRATION,
                new_server_id,
                SERVER_PORT_MIGRATION,
                serializePBJMessage(migrate_msg)
            );
            mMigrateMessages.push(migrate_msg_packet);

            // Stop Forwarder from delivering via this Object's
            // connection, destroy said connection

            //bftm: candidate for multiple obj connections

            //end bftm change
            //  mMigratingConnections[obj_id] = mForwarder->getObjectConnection(obj_id);
            MigratingObjectConnectionsData mocd;

            mocd.obj_conner           =   mForwarder->getObjectConnection(obj_id, mocd.uniqueConnId);
            Duration migrateStartDur  =                 mMigrationTimer.elapsed();
            mocd.milliseconds         =          migrateStartDur.toMilliseconds();
            mocd.migratingTo          =                             new_server_id;
            mocd.loc                  =        mLocationService->location(obj_id);
            mocd.bnds                 =          mLocationService->bounds(obj_id);
            mocd.serviceConnection    =                                      true;

            mMigratingConnections[obj_id] = mocd;



            // Stop tracking the object locally
            mLocationService->removeLocalObject(obj_id);

            mLocalForwarder->removeActiveConnection(obj_id);
            mObjects.erase(obj_id);
            mContext->timeSeries->report(mTimeSeriesObjects, mObjects.size());
            ObjectReference obj(obj_id);

            ObjectSessionMap::iterator session_it = mObjectSessions.find(obj);
            if (session_it != mObjectSessions.end()) {
                notify(&ObjectSessionListener::sessionClosed, session_it->second);
                delete session_it->second;
                mObjectSessions.erase(session_it);
            }
        }
    }

    startSendMigrationMessages();
}

void Server::startSendMigrationMessages() {
    if (mMigrationSendRunning)
        return;

    trySendMigrationMessages();
}
void Server::trySendMigrationMessages() {
    if (mShutdownRequested)
        return;

    mMigrationSendRunning = true;

    // Send what we can right now
    while(!mMigrateMessages.empty()) {
        bool sent = mMigrateServerMessageService->route(mMigrateMessages.front());
        if (!sent)
            break;
        mMigrateMessages.pop();
    }

    // If nothing is left, the call chain ends
    if (mMigrateMessages.empty()) {
        mMigrationSendRunning = false;
        return;
    }

    // Otherwise, we need to set ourselves up to try again later
    mContext->mainStrand->post(
        std::tr1::bind(&Server::trySendMigrationMessages, this)
    );
}

/*
  This function migrates an object to this server that was in the process of migrating away from this server (except the killconn message hasn't come yet.

  Assumes that checks that the object was in migration have occurred

*/
void Server::processAlreadyMigrating(const UUID& obj_id)
{

    ObjectConnectionMap::iterator obj_map_it = mObjectsAwaitingMigration.find(obj_id);
    if (obj_map_it == mObjectsAwaitingMigration.end())
    {
        return;
    }


    ObjectMigrationMap::iterator migration_map_it = mObjectMigrations.find(obj_id);
    if (migration_map_it == mObjectMigrations.end())
    {
        return;
    }


    // Get the data from the two maps
    ObjectConnection* obj_conn = obj_map_it->second;
    Sirikata::Protocol::Migration::MigrationMessage* migrate_msg = migration_map_it->second;


    // Extract the migration message data
    TimedMotionVector3f obj_loc(
        migrate_msg->loc().t(),
        MotionVector3f( migrate_msg->loc().position(), migrate_msg->loc().velocity() )
    );
    TimedMotionQuaternion obj_orient(
        migrate_msg->orientation().t(),
        MotionQuaternion( migrate_msg->orientation().position(), migrate_msg->orientation().velocity() )
    );
    BoundingSphere3f obj_bounds( migrate_msg->bounds() );
    String obj_mesh ( migrate_msg->has_mesh() ? migrate_msg->mesh() : "");
    String obj_phy ( migrate_msg->has_physics() ? migrate_msg->physics() : "");

    // Remove the previous connection from the local forwarder
    mLocalForwarder->removeActiveConnection( obj_id );
    // Move from list waiting for migration message to active objects
    mObjects[obj_id] = obj_conn;
    mContext->timeSeries->report(mTimeSeriesObjects, mObjects.size());
    mLocalForwarder->addActiveConnection(obj_conn);


    // Update LOC to indicate we have this object locally
    mLocationService->addLocalObject(obj_id, obj_loc, obj_orient, obj_bounds, obj_mesh, obj_phy);

    //update our oseg to show that we know that we have this object now.
    OSegEntry idOSegAckTo ((ServerID)migrate_msg->source_server(),migrate_msg->bounds().radius());
    mOSeg->addMigratedObject(obj_id, idOSegAckTo.radius(), idOSegAckTo.server(), true);//true states to send an ack message to idOSegAckTo



    // Handle any data packed into the migration message for space components
    for(int32 i = 0; i < migrate_msg->client_data_size(); i++) {
        Sirikata::Protocol::Migration::MigrationClientData client_data = migrate_msg->client_data(i);
        std::string tag = client_data.key();
        // FIXME these should live in a map, how do we deal with ordering constraints?
        if (tag == "prox") {
            assert( tag == mProximity->migrationClientTag() );
            mProximity->receiveMigrationData(obj_id, /* FIXME */NullServerID, mContext->id(), client_data.data());
        }
        else {
            SILOG(space,error,"Got unknown tag for client migration data");
        }
    }


    //remove the forwarding connection that already exists for that object
    ObjectConnection* migrated_conn_old = mForwarder->removeObjectConnection(obj_id);
    delete migrated_conn_old;

 //change the boolean value associated with object so that you know not to keep servicing the connection associated with this object in mMigratingConnections
   mMigratingConnections[obj_id].serviceConnection = false;

   // Stage this connection with the forwarder, enabled when ack received
   mForwarder->addObjectConnection(obj_id, obj_conn);

    // Clean out the two records from the migration maps
    mObjectsAwaitingMigration.erase(obj_map_it);
    mObjectMigrations.erase(migration_map_it);


    // Send reply back indicating that the migration was successful
    Sirikata::Protocol::Session::Container response_container;
    Sirikata::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
    fillVersionInfo(response.mutable_version(), mContext);
    response.set_response( Sirikata::Protocol::Session::ConnectResponse::Success );

    Sirikata::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response_container)
    );
    // Sent directly via object host connection manager because ObjectConnection isn't enabled yet
    sendSessionMessageWithRetry(obj_conn->connID(), obj_response, Duration::seconds(0.05));
}


//returns true if the object associated with obj_id is in the process of migrating from this server
//returns false otherwise.
bool Server::checkAlreadyMigrating(const UUID& obj_id)
{
  if (mMigratingConnections.find(obj_id) != mMigratingConnections.end())
    return true; //it is already migrating

  return false;  //it isn't.
}


//This shouldn't get called yet.
void Server::killObjectConnection(const UUID& obj_id)
{
  MigConnectionsMap::iterator objConMapIt = mMigratingConnections.find(obj_id);

  if (objConMapIt != mMigratingConnections.end())
  {
    uint64 connIDer;
    mForwarder->getObjectConnection(obj_id,connIDer);

    if (connIDer == objConMapIt->second.uniqueConnId)
    {
      //means that the object did not undergo an intermediate migrate.  Should go ahead and remove this connection from forwarder
      mForwarder->removeObjectConnection(obj_id);
    }
    else
    {
        SPACE_LOG(error, "Object " << obj_id.toString() << " has already re-migrated");
    }

    //log the event's completion.
    Duration currentDur = mMigrationTimer.elapsed();
    Duration timeTakenMs = Duration::milliseconds(currentDur.toMilliseconds() - mMigratingConnections[obj_id].milliseconds);
    ServerID migTo  = mMigratingConnections[obj_id].migratingTo;
    CONTEXT_SPACETRACE(objectMigrationRoundTrip, obj_id, mContext->id(), migTo , timeTakenMs);

    mMigratingConnections.erase(objConMapIt);
  }
}



} // namespace Sirikata
