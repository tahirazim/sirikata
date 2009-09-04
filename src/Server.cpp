#include "Network.hpp"
#include "Server.hpp"
#include "Proximity.hpp"
#include "CoordinateSegmentation.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"
#include "ServerMessageQueue.hpp"
#include "Statistics.hpp"
#include "Options.hpp"
#include "ObjectMessageQueue.hpp"
#include "LoadMonitor.hpp"
#include "ForwarderUtilityClasses.hpp"
#include "Forwarder.hpp"

#include "ObjectSegmentation.hpp"

#include "ObjectConnection.hpp"
#include "ObjectHostConnectionManager.hpp"

#include "Random.hpp"

#include <iostream>
#include <iomanip>



namespace CBR
{

Server::Server(SpaceContext* ctx, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, ObjectSegmentation* oseg, ServerIDMap* sidmap)
 : mContext(ctx),
   mLocationService(loc_service),
   mCSeg(cseg),
   mProximity(prox),
   mOSeg(oseg),
   mForwarder(forwarder),
   mObjectHostConnectionManager(NULL)
{
      mForwarder->registerMessageRecipient(MESSAGE_TYPE_MIGRATE, this);

    mForwarder->initialize(cseg,oseg,loc_service,omq,smq,lm,mProximity);    //run initialization for forwarder

    Address4* oh_listen_addr = sidmap->lookupExternal(mContext->id);
    mObjectHostConnectionManager = new ObjectHostConnectionManager(
        mContext, *oh_listen_addr,
        std::tr1::bind(&Server::handleObjectHostMessage, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2)
    );
}

Server::~Server()
{
    delete mObjectHostConnectionManager;

    mForwarder->unregisterMessageRecipient(MESSAGE_TYPE_MIGRATE, this);

    for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        UUID obj_id = it->first;

        // Stop any proximity queries for this object
        mProximity->removeQuery(obj_id);

        mLocationService->removeLocalObject(obj_id);

        // Stop Forwarder from delivering via this Object's
        // connection, destroy said connection
        ObjectConnection* migrated_conn = mForwarder->removeObjectConnection(obj_id);
        mClosingConnections.insert(migrated_conn);

        // FIXME there's probably quite a bit more cleanup to do here
    }
    mObjects.clear();
}

ObjectConnection* Server::getObjectConnection(const UUID& object_id) const {
    ObjectConnectionMap::const_iterator it = mObjects.find(object_id);
    if(it != mObjects.end())
        return it->second;

    // XXX FIXME migrating objects?

    return NULL;
}

void Server::serviceNetwork()
{
    mForwarder->service();


    mObjectHostConnectionManager->service();

  // Tick all active connections
  for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
      ObjectConnection* conn = it->second;
      conn->service();
  }

  // Tick closing object connections, deleting them when they are
  ObjectConnectionSet persistingConnections;
  for(ObjectConnectionSet::iterator it = mClosingConnections.begin(); it != mClosingConnections.end(); it++) {
      ObjectConnection* conn = *it;
      conn->service();
      if (conn->empty())
          delete conn;
      else
          persistingConnections.insert(conn);
  }
  mClosingConnections.swap(persistingConnections);
}

void Server::handleObjectHostMessage(const ObjectHostConnectionManager::ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* obj_msg) {
    // Before admitting a message, we need to do some sanity checks.  Also, some types of messages get
    // exceptions for bootstrapping purposes (namely session messages to the space).

    // 1. If the source is the space, somebody is messing with us.
    bool space_source = (obj_msg->source_object() == UUID::null());
    if (space_source) {
        SILOG(cbr,error,"Got message from object host claiming to be from space.");
        delete obj_msg;
        return;
    }

    // 2. For connection bootstrapping purposes we need to exempt session messages destined for the space.
    // Note that we need to check this before the connected sanity check since obviously the object won't
    // be connected yet.  We dispatch directly from here since this needs information about the object host
    // connection to be passed along as well.
    bool space_dest = (obj_msg->dest_object() == UUID::null());
    bool session_msg = (obj_msg->dest_port() == OBJECT_PORT_SESSION);
    if (space_dest && session_msg) {
        handleSessionMessage(conn_id, *obj_msg);
        delete obj_msg;
        return;
    }

    // 3. If we don't have a connection for the source object, we can't do anything with it.
    // The object could be migrating and we get outdated packets.  Currently this can
    // happen because we need to maintain the connection long enough to deliver the init migration
    // message.  Currently we just discard these messages, but we may need to account for this.
    // NOTE that we check connecting objects as well since we need to get past this point to deliver
    // Session messages.
    bool source_connected = mObjects.find(obj_msg->source_object()) != mObjects.end(); // FIXME migrating objects?
    if (!source_connected)
    {
        if (mObjectsAwaitingMigration.find(obj_msg->source_object()) == mObjectsAwaitingMigration.end() &&
            mObjectMigrations.find(obj_msg->source_object()) == mObjectMigrations.end())
            SILOG(cbr,warn,"Got message for unknown object: " << obj_msg->source_object().toString());
        else
            SILOG(cbr,warn,"Server got message from object after migration started: " << obj_msg->source_object().toString());

        delete obj_msg;

        return;
    }

    // 4. Finally, if we've passed all these tests, then everything looks good and we can route it
    bool route_success = mForwarder->routeObjectHostMessage(obj_msg);
    // FIXME handle forwarding failure
}

// Handle Session messages from an object
void Server::handleSessionMessage(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& msg) {
    CBR::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromString(msg.payload());
    assert(parse_success);

    // Connect messages
    if (session_msg.has_connect())
        handleConnect(oh_conn_id, msg, session_msg.connect());

    // InitiateMigration messages
    assert(!session_msg.has_init_migration());

    // Migrate messages
    if (session_msg.has_migrate())
        handleMigrate(msg, session_msg.migrate());
}

// Handle Connect message from object
void Server::handleConnect(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Connect& connect_msg) {
    UUID obj_id = container.source_object();
    assert( getObjectConnection(obj_id) == NULL );

    // FIXME sanity check the new connection
    // -- authentication
    // -- verify object may connect, i.e. not already in system (e.g. check oseg)

    // Create and store the connection
    ObjectConnection* conn = new ObjectConnection(obj_id, mObjectHostConnectionManager, oh_conn_id);
    mObjects[obj_id] = conn;

    // Add object as local object to LocationService
    TimedMotionVector3f loc( connect_msg.loc().t(), MotionVector3f(connect_msg.loc().position(), connect_msg.loc().velocity()) );
    mLocationService->addLocalObject(obj_id, loc, connect_msg.bounds());
    //update our oseg to show that we know that we have this object now.
    mOSeg->addObject(obj_id, mContext->id);
    // Register proximity query
    mProximity->addQuery(obj_id, SolidAngle(connect_msg.query_angle()));
    // Allow the forwarder to send to ship messages to this connection
    mForwarder->addObjectConnection(obj_id, conn);

    // Send reply back indicating that the connection was successful
    CBR::Protocol::Session::ConnectResponse response;
    response.set_response( CBR::Protocol::Session::ConnectResponse::Success );

    CBR::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id,
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response)
    );

    conn->send( obj_response );
}

// Handle Migrate message from object
void Server::handleMigrate(const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Migrate& migrate_msg) {
    ObjectConnection* conn = getObjectConnection(container.source_object());

    assert(conn->id() == migrate_msg.object());
    mObjectsAwaitingMigration[migrate_msg.object()] = conn;

    // Try to handle this migration if all info is available
    handleMigration(migrate_msg.object());
}

void Server::receiveMessage(Message* msg) {
    MigrateMessage* migrate_msg = dynamic_cast<MigrateMessage*>(msg);
    if (migrate_msg != NULL) {
        const UUID obj_id = migrate_msg->contents.object();
        mObjectMigrations[obj_id] = new CBR::Protocol::Migration::MigrationMessage( migrate_msg->contents );
        // Try to handle this migration if all the info is available
        handleMigration(obj_id);
    }

    delete msg;
}

void Server::handleMigration(const UUID& obj_id) {
    // Try to find the info in both lists -- the connection and migration information
    ObjectConnectionMap::iterator obj_map_it = mObjectsAwaitingMigration.find(obj_id);
    if (obj_map_it == mObjectsAwaitingMigration.end())
        return;

    ObjectMigrationMap::iterator migration_map_it = mObjectMigrations.find(obj_id);
    if (migration_map_it == mObjectMigrations.end())
        return;

    // Get the data from the two maps
    ObjectConnection* obj_conn = obj_map_it->second;
    CBR::Protocol::Migration::MigrationMessage* migrate_msg = migration_map_it->second;


    // Extract the migration message data
    TimedMotionVector3f obj_loc(
        migrate_msg->loc().t(),
        MotionVector3f( migrate_msg->loc().position(), migrate_msg->loc().velocity() )
    );
    BoundingSphere3f obj_bounds( migrate_msg->bounds() );
    SolidAngle obj_query_angle( migrate_msg->query_angle() );

    // Move from list waiting for migration message to active objects
    mObjects[obj_id] = obj_conn;


    // Update LOC to indicate we have this object locally
    mLocationService->addLocalObject(obj_id, obj_loc, obj_bounds);

    //update our oseg to show that we know that we have this object now.
    mOSeg->addObject(obj_id, mContext->id);

    //    mOSeg->addObject(obj_id,migrate_msg->id());
    //We also send an oseg message to the server that the object was formerly hosted on.  This is an acknwoledge message that says, we're handling the object now...that's going to be the server with the origin tag affixed.

//     ServerID idOSegAckTo = (ServerID)migrate_msg->source_server();
//     Message* oseg_ack_msg;
//     //              mOSeg->generateAcknowledgeMessage(obj_id, idOSegAckTo,oseg_ack_msg);
//     oseg_ack_msg = mOSeg->generateAcknowledgeMessage(obj_id, idOSegAckTo);

//     if (oseg_ack_msg != NULL)
//         mForwarder->route(oseg_ack_msg, (dynamic_cast <OSegMigrateMessage*>(oseg_ack_msg))->getMessageDestination(),false);


    // Finally, subscribe the object for proximity queries
    mProximity->addQuery(obj_id, obj_query_angle);

    // Allow the forwarder to deliver to this connection
    mForwarder->addObjectConnection(obj_id, obj_conn);


    // Clean out the two records from the migration maps
    mObjectsAwaitingMigration.erase(obj_map_it);
    mObjectMigrations.erase(migration_map_it);
}

void Server::service() {
    mLocationService->service();
    serviceProximity();
    serviceNetwork();
    checkObjectMigrations();
}

void Server::serviceProximity() {
    // If we have global introduction, then we can just ignore proximity evaluation.
    if (GetOption(OBJECT_GLOBAL)->as<bool>() == true)
        return;

  // Check for proximity updates
  mProximity->service();
}

void Server::checkObjectMigrations()
{
    // * check for objects crossing server boundaries
    // * wrap up state and send message to other server
    //     to reinstantiate the object there
    // * delete object on this side


  std::vector<UUID> migrated_objects;
  for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); it++)
  {
    const UUID& obj_id = it->first;
    ObjectConnection* obj_conn = it->second;

    Vector3f obj_pos = mLocationService->currentPosition(obj_id);
    ServerID new_server_id = lookup(obj_pos);


    if (new_server_id != mContext->id)
    {
      // FIXME While we're working on the transition to a separate object host
      // we do 2 things when we detect a server boundary crossing:
      // 1. Generate a message which is sent to the object host telling it to
      //    transition to the new server
      // 2. Generate the migrate message which contains the current state of
      //    the object which will be reconstituted on the other space server.


      // Part 1
      CBR::Protocol::Session::Container session_msg;
      CBR::Protocol::Session::IInitiateMigration init_migration_msg = session_msg.mutable_init_migration();
      init_migration_msg.set_new_server( (uint64)new_server_id );
      CBR::Protocol::Object::ObjectMessage* init_migr_obj_msg = createObjectMessage(
          mContext->id,
          UUID::null(), OBJECT_PORT_SESSION,
          obj_id, OBJECT_PORT_SESSION,
          serializePBJMessage(session_msg)
      );
      obj_conn->send(init_migr_obj_msg); // Note that we can't go through the forwarder since it will stop delivering to this object connection right after this
      //printf("Routed init migration msg to %s\n", obj_id.toString().c_str());

      // Part 2

      //means that we're migrating from this server to another
      //bftm
      mOSeg->migrateObject(obj_id,new_server_id);
      //says that



      // Send out the migrate message
      MigrateMessage* migrate_msg = new MigrateMessage(mContext->id);
      migrate_msg->contents.set_source_server(mContext->id);
      migrate_msg->contents.set_object(obj_id);
      CBR::Protocol::Migration::ITimedMotionVector migrate_loc = migrate_msg->contents.mutable_loc();
      TimedMotionVector3f obj_loc = mLocationService->location(obj_id);
      migrate_loc.set_t( obj_loc.updateTime() );
      migrate_loc.set_position( obj_loc.position() );
      migrate_loc.set_velocity( obj_loc.velocity() );
      migrate_msg->contents.set_bounds( mLocationService->bounds(obj_id) );


      // Stop any proximity queries for this object
      mProximity->removeQuery(obj_id);

      // Stop tracking the object locally
      mLocationService->removeLocalObject(obj_id);

      //      printf("\n\nbftm debug: Inside of server.cpp.  generating a migrate message.\n\n");
      mForwarder->route( migrate_msg , new_server_id);

      // Stop Forwarder from delivering via this Object's
      // connection, destroy said connection
      ObjectConnection* migrated_conn = mForwarder->removeObjectConnection(obj_id);
      mClosingConnections.insert(migrated_conn);

      migrated_objects.push_back(obj_id);
    }
  }

  for (std::vector<UUID>::iterator it = migrated_objects.begin(); it != migrated_objects.end(); it++){
    ObjectConnectionMap::iterator obj_map_it = mObjects.find(*it);
    mObjects.erase(obj_map_it);

  }
}

ServerID Server::lookup(const Vector3f& pos)
{
  ServerID sid = mCSeg->lookup(pos);
  return sid;
}

/*
ServerID Server::lookup(const UUID& obj_id)
{
  Vector3f pos = mLocationService->currentPosition(obj_id);
  ServerID sid = mCSeg->lookup(pos);
  return sid;
}
*/




} // namespace CBR
