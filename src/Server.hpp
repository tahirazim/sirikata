
#ifndef _CBR_SERVER_HPP_
#define _CBR_SERVER_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"

#include "ObjectHostConnectionManager.hpp"
#include "TimeProfiler.hpp"
#include "PollingService.hpp"
#include "sirikata/util/SizedThreadSafeQueue.hpp"
namespace CBR
{
class Forwarder;
class LocalForwarder;

class LocationService;
class Proximity;
class MigrationMonitor;

class CoordinateSegmentation;
class ObjectSegmentation;

class ObjectConnection;
class ObjectHostConnectionManager;

class ServerIDMap;

  /** Handles all the basic services provided for objects by a server,
   *  including routing and message delivery, proximity services, and
   *  object -> server mapping.  This is a singleton for each simulated
   *  server.  Other servers are referenced by their ServerID.
   */
class Server : public MessageRecipient, public Service
{
public:
    Server(SpaceContext* ctx, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectSegmentation* oseg, Address4* oh_listen_addr);
    ~Server();

    virtual void receiveMessage(Message* msg);
private:
    // Service Implementation
    void start();
    void stop();

    // Handle a migration event generated by the MigrationMonitor
    void handleMigrationEvent(const UUID& objid);

    // Starts the process of trying to send migration messages, or continues one if it's already running.
    void startSendMigrationMessages();

    // Try to send outstanding migration messages.  This chains automatically until the queue is emptied.
    void trySendMigrationMessages();


    // Send a session message directly to the object via the OH connection manager, bypassing any restrictions on
    // the current state of the connection.  Keeps retrying until the message gets through.
    void sendSessionMessageWithRetry(const ObjectHostConnectionManager::ConnectionID& conn, CBR::Protocol::Object::ObjectMessage* msg, const Duration& retry_rate);


    // Checks if an object is connected to this server
    bool isObjectConnected(const UUID& object_id) const;

    // Callback which handles messages from object hosts -- mostly just does sanity checking
    // before using the forwarder to do routing.  Operates in the
    // network strand to allow for fast forwarding, see
    // handleObjectHostMessageRouting for continuation in main strand
    bool handleObjectHostMessage(const ObjectHostConnectionManager::ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* msg);
    // Perform forwarding for a message on the front of mRouteObjectMessage from the object host which
    // couldn't be forwarded directly by the networking code
    // (i.e. needs routing to another node)
    void handleObjectHostMessageRouting();

    // Handle Session messages from an object
    void handleSessionMessage(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, CBR::Protocol::Object::ObjectMessage* msg);
    // Handle Connect message from object
    void retryHandleConnect(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, CBR::Protocol::Object::ObjectMessage* );
    void retryObjectMessage(const UUID& obj_id, CBR::Protocol::Object::ObjectMessage* );
    void handleConnect(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Connect& connect_msg);

    // Handle connection ack message from object
    void handleConnectAck(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container);

    // Handle Migrate message from object
    void handleMigrate(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Connect& migrate_msg);

    // Performs actual migration after all the necessary information is available.
    void handleMigration(const UUID& obj_id);

    //finally deletes any object connections to obj_id
    void killObjectConnection(const UUID& obj_id);

    void finishAddObject(const UUID& obj_id);

    bool checkAlreadyMigrating(const UUID& obj_id);
    void processAlreadyMigrating(const UUID& obj_id);

    SpaceContext* mContext;
    LocationService* mLocationService;
    CoordinateSegmentation* mCSeg;
    Proximity* mProximity;
    ObjectSegmentation* mOSeg;
    LocalForwarder* mLocalForwarder;
    Forwarder* mForwarder;
    MigrationMonitor* mMigrationMonitor;
    bool mMigrationSendRunning; // Indicates whether an event chain for sending outstanding migration messages is running.
                                // Note that ideally this could be replaced by just using our own internal queue
                                // to hold messages since migration messsages aren't going to get stale

    bool mShutdownRequested;

    ObjectHostConnectionManager* mObjectHostConnectionManager;

    typedef std::map<UUID, ObjectConnection*> ObjectConnectionMap;

    ObjectConnectionMap mObjects; // NOTE: only Forwarder and LocalForwarder
                                  // should actually use the connection, this is
                                  // only still a map to handle migrations
                                  // properly
    ObjectConnectionMap mObjectsAwaitingMigration;

    typedef std::map<UUID, CBR::Protocol::Migration::MigrationMessage*> ObjectMigrationMap;
    ObjectMigrationMap mObjectMigrations;

    typedef std::set<ObjectConnection*> ObjectConnectionSet;
    ObjectConnectionSet mClosingConnections; // Connections that are closing but need to finish delivering some messages

    //std::map<UUID,ObjectConnection*>
    struct MigratingObjectConnectionsData
    {
      ObjectConnection* obj_conner;
      int milliseconds;
      ServerID migratingTo;
      TimedMotionVector3f loc;
      BoundingSphere3f bnds;
      uint64 uniqueConnId;
      bool serviceConnection;
    };

      typedef std::queue<Message*> MigrateMessageQueue;
      // Outstanding MigrateMessages, which get objects to other servers.
      MigrateMessageQueue mMigrateMessages;

    //    ObjectConnectionMap mMigratingConnections;//bftm add
    typedef std::map<UUID,MigratingObjectConnectionsData> MigConnectionsMap;
    MigConnectionsMap mMigratingConnections;//bftm add
    Timer mMigrationTimer;

    struct StoredConnection
    {
      ObjectHostConnectionManager::ConnectionID    conn_id;
      CBR::Protocol::Session::Connect             conn_msg;
    };

    typedef std::map<UUID, StoredConnection> StoredConnectionMap;
    StoredConnectionMap  mStoredConnectionData;
    struct ConnectionIDObjectMessagePair{
        ObjectHostConnectionManager::ConnectionID conn_id;
        CBR::Protocol::Object::ObjectMessage* obj_msg;
        ConnectionIDObjectMessagePair(ObjectHostConnectionManager::ConnectionID conn_id, CBR::Protocol::Object::ObjectMessage*msg) {
            this->conn_id=conn_id;
            this->obj_msg=msg;
        }
        size_t size() const{
            return obj_msg->ByteSize();
        }
    };
    Sirikata::SizedThreadSafeQueue<ConnectionIDObjectMessagePair>mRouteObjectMessage;
}; // class Server

} // namespace CBR

#endif //_CBR_SERVER_HPP_
