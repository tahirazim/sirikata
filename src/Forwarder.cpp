
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

#include "Proximity.hpp"

#include "ObjectConnection.hpp"

#include "Random.hpp"

namespace CBR
{


  /*
    Constructor for Forwarder

  */
Forwarder::Forwarder(ServerID id)
 : mTrace(NULL),
   mCSeg(NULL),
   mOSeg(NULL),
   mLocationService(NULL),
   mObjectMessageQueue(NULL),
   mServerMessageQueue(NULL),
   mLoadMonitor(NULL),
   mProximity(NULL),
   m_serv_ID(id),
   mCurrentTime(NULL),
   mLastSampleTime(Time::null()),
   mSampleRate( GetOption(STATS_SAMPLE_RATE)->as<Duration>() )
{
    //no need to initialize mSelfMessages and mOutgoingMessages.

    // Messages destined for objects are subscribed to here so we can easily pick them
    // out and decide whether they can be delivered directly or need forwarding
    this->registerMessageRecipient(MESSAGE_TYPE_OBJECT, this);
}

  //Don't need to do anything special for destructor
  Forwarder::~Forwarder()
  {
    this->unregisterMessageRecipient(MESSAGE_TYPE_OBJECT, this);
  }

  /*
    Assigning time and mObjects, which should have been constructed in Server's constructor.
  */
void Forwarder::initialize(Trace* trace, CoordinateSegmentation* cseg, ObjectSegmentation* oseg, LocationService* locService, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, Time* currTime, Proximity* prox)
  {
      mTrace = trace;
      mCSeg = cseg;
      mOSeg = oseg;
      mLocationService = locService;
      mObjectMessageQueue = omq;
      mServerMessageQueue =smq;
      mLoadMonitor = lm;
    mCurrentTime = currTime;
    mProximity = prox;
  }

  //used to access server id, which was given in initialization
  const ServerID& Forwarder::serv_id() const
  {
    return m_serv_ID;
  }



  /*
  void Forwarder::getOSegMessages()
  {
    std::vector<Message*> messToSendFromOSegToForwarder;
    std::vector<ServerID> destServers;


    mOSeg -> getMessages (messToSendFromOSegToForwarder,destServers);

    for (int s=0; s < (signed)messToSendFromOSegToForwarder.size(); ++s)
    {
      //route
      route(messToSendFromOSegToForwarder[s],destServers[s],false);
    }

  }
  */


  /*
    Sends a tick to OSeg.  Receives messages from oseg.  Adds them to mOutgoingQueue.
  */
  void Forwarder::tickOSeg(const Time&t)
  {
    //    getOSegMessages();
    std::cout<<"\n\nI got into tickOSeg in forwarder.cpp\n\n";

    //bftm tmp, nothing
    //    mOSeg -> tick(t,mOutgoingMessages);
    //objects from
    std::map<UUID,ServerID> updatedObjectLocations;
    std::map<UUID,ServerID>::iterator iter;
    std::map<UUID,ObjectMessageList>::iterator iterObjectsInTransit;

    std::vector<Message*>messagesToSend;
    
    mOSeg -> tick(t,updatedObjectLocations,messagesToSend);


    //deliver acknowledge messages.


    
    if (messagesToSend.size() !=0)
    {
      std::cout<<"\n\nbftm debug: inside of forwarder.cpp this is the number of messages that I have to send:  "<<messagesToSend.size()<<"\n\n";
    }

    if (updatedObjectLocations.size() !=0)
    {
      std::cout<<"\n\nbftm debug: inside of forwarder.cpp this is the number of updatedObjectLocations:  "<<updatedObjectLocations.size()<<"\n\n";
    }

    std::cout<<"\n\nThis is the size of the objects in transit:   "<<mObjectsInTransit.size()<<"\n\n";

    
    for (unsigned int s=0; s < messagesToSend.size(); ++s)
    {
      if (messagesToSend[s] != NULL)
      {
        route (messagesToSend[s],(dynamic_cast<OSegMigrateMessage*>(messagesToSend[s]))->getMessageDestination(),false);
      }
    }
    
    
    //    cross-check updates against held messages.  
    for (iter = updatedObjectLocations.begin();  iter != updatedObjectLocations.end(); ++iter)
    {
      iterObjectsInTransit = mObjectsInTransit.find(iter->first);
      if (iterObjectsInTransit != mObjectsInTransit.end())
      {
        //means that we can route messages being held in mObjectsInTransit

        for (int s=0; s < (signed)((iterObjectsInTransit->second).size()); ++s)
        {
          std::cout<<"\n\nbftm debug:  inside of forwarder.  Actually routing a message that I had saved up\n\n";
          route((iterObjectsInTransit->second)[s],iter->second,false);
        }

        //remove the messages from the objects in transit
        mObjectsInTransit.erase(iterObjectsInTransit);

      }
      else
      {
        //        std::cout<<"\n\nbftm debug:  Inside of forwarder.cpp, couldn't find stored message that matched "<<iter->first.toString()<<"\n\n";
        //        std::cout<<"\n\nbftm debug: Inside of forwarder.cpp couldn't find stored message that matched. \n\n";
        
      }
    }
  }



  void Forwarder::tick(const Time&t)
  {
    std::cout<<"\n\nInside of forwarder tick\n\n";
    
    if (t - mLastSampleTime > mSampleRate) {
          mServerMessageQueue->reportQueueInfo(t);
          mLastSampleTime = t;
      }

    mLoadMonitor->tick(t);

    tickOSeg(t);//updates oseg


    std::deque<SelfMessage> self_messages;
    self_messages.swap( mSelfMessages );
    while (!self_messages.empty())
    {
      processChunk(self_messages.front().data, this->serv_id(), self_messages.front().forwarded);
      self_messages.pop_front();
    }

    while(!mOutgoingMessages.empty())
    {
        OutgoingMessage& next_msg = mOutgoingMessages.front();
        bool send_success = mServerMessageQueue->addMessage(next_msg.dest, next_msg.data);
        if (!send_success) break;
        mOutgoingMessages.pop_front();
    }

    // XXXXXXXXXXXXXXXXXXXXXX Generate noise

    if (GetOption(NOISE)->as<bool>()) {
        if (GetOption(SERVER_QUEUE)->as<String>() == "fair") {
            for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
                if (sid == m_serv_ID) continue;
                while(true) {
                    NoiseMessage* noise_msg = new NoiseMessage(m_serv_ID, (uint32)(50 + 200*randFloat())); // FIXME control size from options?

                    uint32 offset = 0;
                    Network::Chunk msg_serialized;
                    offset = noise_msg->serialize(msg_serialized, offset);

                    bool sent_success = mServerMessageQueue->addMessage(sid, msg_serialized);
                    if (sent_success)
                        mTrace->serverDatagramQueued((*mCurrentTime), sid, noise_msg->id(), offset);
                    delete noise_msg;
                    if (!sent_success) break;
                }
            }
        }
        else {
            // For FIFO we generate for random servers because they all
            // share a single internal queue.
            uint32 nfail = 0;
            uint32 nservers = mCSeg->numServers();
            while(nfail < nservers) {
                ServerID sid = randInt<uint32>(1, nservers);
                if (sid == m_serv_ID) continue;

                NoiseMessage* noise_msg = new NoiseMessage(m_serv_ID, (uint32)(50 + 200*randFloat())); // FIXME control size from options?

                uint32 offset = 0;
                Network::Chunk msg_serialized;
                offset = noise_msg->serialize(msg_serialized, offset);

                bool sent_success = mServerMessageQueue->addMessage(sid, msg_serialized);
                if (sent_success)
                    mTrace->serverDatagramQueued((*mCurrentTime), sid, noise_msg->id(), offset);
                delete noise_msg;
                if (!sent_success) nfail++;
            }
        }
    }
    // XXXXXXXXXXXXXXXXXXXXXXXX

    mObjectMessageQueue->service(t);
    mServerMessageQueue->service(t);


    Sirikata::Network::Chunk *c=NULL;
    ServerID source_server;
    while(mServerMessageQueue->receive(&c, &source_server))
    {
        processChunk(*c, source_server, false);
        delete c;
    }
  }



  // Routing interface for servers.  This is used to route messages that originate from
  // a server provided service, and thus don't have a source object.  Messages may be destined
  // for either servers or objects.  The second form will simply automatically do the destination
  // server lookup.
  // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
  void Forwarder::route(Message* msg, const ServerID& dest_server, bool is_forward)
  {

    uint32 offset = 0;
    Network::Chunk msg_serialized;
    offset = msg->serialize(msg_serialized, offset);

    printf("\n\nbftm debug.  I am routing a message in 1 to %i\n\n", dest_server);
    
    if (dest_server==serv_id())
    {
      if (!is_forward)
      {
        mTrace->serverDatagramQueued((*mCurrentTime), dest_server, msg->id(), offset);
        mTrace->serverDatagramSent((*mCurrentTime), (*mCurrentTime), 0 , dest_server, msg->id(), offset); // self rate is infinite => start and end times are identical
      }
      else
      {
          // The more times the message goes through a forward to self loop, the more times this will record, causing an explosion
          // in trace size.  It's probably not worth recording this information...
          //mTrace->serverDatagramQueued((*mCurrentTime), dest_server, msg->id(), offset);
      }
      mSelfMessages.push_back( SelfMessage(msg_serialized, is_forward) );
    }
    else
    {
      mTrace->serverDatagramQueued((*mCurrentTime), dest_server, msg->id(), offset);
      mOutgoingMessages.push_back( OutgoingMessage(msg_serialized, dest_server) );
    }
    delete msg;
  }

  //here's what I think that should be replaced with:

  void Forwarder::route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward)
  {
    UUID dest_obj = msg->dest_object();

    //    ServerID dest_server_id = lookup(dest_obj);
    mOSeg->lookup(dest_obj);
    printf("\n\nbftm debug.  I am routing a message \n\n");
    
    //add message to objects in transit.
    //      if (mObjectsInTransit[dest_obj] == null)
    if (mObjectsInTransit.find(dest_obj) == mObjectsInTransit.end())
    {
      //no messages have been queued for mObjectsInTransit
      //add this as the beginning of a vector of message pointers.
      std::vector<Message*> tmp;
      tmp.push_back(msg);
      mObjectsInTransit[dest_obj] = tmp;
    }
    else
    {
      //add message to queue of objects in transit.
      mObjectsInTransit[dest_obj].push_back(msg);
    }
  }
  //end what i think it should be replaced with
  



bool Forwarder::routeObjectHostMessage(CBR::Protocol::Object::ObjectMessage* obj_msg) {
    // Messages destined for the space skip the object message queue and just get dispatched
    if (obj_msg->dest_object() == UUID::null()) {
        dispatchMessage(*obj_msg);
        delete obj_msg;
        return true;
    }

    return mObjectMessageQueue->send(obj_msg);
}
  
  //ewen changes.
// void Forwarder::route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward)
// {
//     UUID dest_obj = msg->dest_object();
//     ServerID dest_server_id = mOSeg->lookup(dest_obj);
//     route(msg, dest_server_id, is_forward);
// }


// void Forwarder::route(CBR::Protocol::Object::ObjectMessage* msg, ServerID dest_serv, bool is_forward) {
//     if (dest_serv == OBJECT_IN_TRANSIT)


  void Forwarder::route(Message* msg, const UUID& dest_obj, bool is_forward)
  {

    std::cout<<"\n\nbftm debug.  I am routing a message for object:  "<< dest_obj.toString()<<"\n\n";
    
    mOSeg->lookup(dest_obj);

    
    //add message to objects in transit.
    //      if (mObjectsInTransit[dest_obj] == null)
    if (mObjectsInTransit.find(dest_obj) == mObjectsInTransit.end())
    {
      //no messages have been queued for mObjectsInTransit
      //add this as the beginning of a vector of message pointers.
      std::vector<Message*> tmp;
      tmp.push_back(msg);
      mObjectsInTransit[dest_obj] = tmp;

    }
    else
    {
      //add message to queue of objects in transit.
      mObjectsInTransit[dest_obj].push_back(msg);
    }

    

    
  }


  
  

  void Forwarder::processChunk(const Network::Chunk&chunk, const ServerID& source_server, bool forwarded_self_msg)
  {
    Message* result;
    uint32 offset=0;

    do
    {
      offset = Message::deserialize(chunk,offset,&result);

      if (!forwarded_self_msg)
        mTrace->serverDatagramReceived((*mCurrentTime), (*mCurrentTime), source_server, result->id(), offset);

      deliver(result);

      //if (delivered)
      //  mTrace->serverDatagramReceived((*mCurrentTime), (*mCurrentTime), source_server, result->id(), offset);
    }while (offset<chunk.size());
  }



  // Delivery interface.  This should be used to deliver received messages to the correct location -
  // the server or object it is addressed to.
  void Forwarder::deliver(Message* msg)
  {
    switch(msg->type()) {
      case MESSAGE_TYPE_OBJECT:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_NOISE:
          {
              NoiseMessage* noise_msg = dynamic_cast<NoiseMessage*>(msg);
              assert(noise_msg != NULL);
              delete noise_msg;
          }
        break;
      case MESSAGE_TYPE_MIGRATE:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_CSEG_CHANGE:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_LOAD_STATUS:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_OSEG_MIGRATE:
        {
          
          OSegMigrateMessage* oseg_change_msg = dynamic_cast<OSegMigrateMessage*> (msg);
          assert(oseg_change_msg != NULL);

          if (oseg_change_msg->getMessageDestination() != this->serv_id())
          {
            //route it on to another server
            route(oseg_change_msg, oseg_change_msg->getMessageDestination(),true);
          }
          else
          {
            //otherwise, deliver to local oseg.

            //bftm debug
            mOSeg->osegMigrateMessage(oseg_change_msg);
            delete oseg_change_msg;
          }
        }
        break;
      case MESSAGE_TYPE_OSEG_LOOKUP:
        {
          printf("\n\nI caugt an oseg lookup message exception in forwarder.cpp\n\n");
          assert(false);
        }
        break;
      case MESSAGE_TYPE_SERVER_PROX_QUERY:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_SERVER_PROX_RESULT:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_BULK_LOCATION:
          {
              dispatchMessage(msg);
          }
          break;
      default:
        assert(false);
        break;
    }
  }


void Forwarder::receiveMessage(Message* msg) {
    // Forwarder only subscribes as a recipient for object messages
    // so it can easily check whether it can deliver directly
    // or needs to forward them.
    assert(msg->type() == MESSAGE_TYPE_OBJECT);
    ObjectMessage* obj_msg = dynamic_cast<ObjectMessage*>(msg);
    assert(obj_msg != NULL);

    UUID dest = obj_msg->contents.dest_object();

    // Special case: Object 0 is the space itself
    if (dest == UUID::null()) {
        dispatchMessage(obj_msg->contents);
        return;
    }

    // Otherwise, either deliver or forward it, depending on whether the destination object is attached to this server
    ObjectConnection* conn = getObjectConnection(dest);
    if (conn != NULL)
        conn->deliver(obj_msg->contents);
    else {
        CBR::Protocol::Object::ObjectMessage* obj_msg_cpy = new CBR::Protocol::Object::ObjectMessage(obj_msg->contents);
        route(obj_msg_cpy, true);
    }

    delete msg;
}



  void Forwarder::forward(Message* msg, const UUID& dest_obj)
  {

    mOSeg->lookup(dest_obj);
    
    printf("\n\nbftm debug.  I am forwarding a message to \n\n");

    //add message to objects in transit.
    //      if (mObjectsInTransit[dest_obj] == null)
    if (mObjectsInTransit.find(dest_obj) == mObjectsInTransit.end())
    {
      //no messages have been queued for mObjectsInTransit
      //add this as the beginning of a vector of message pointers.
      std::vector<Message*> tmp;
      tmp.push_back(msg);
      mObjectsInTransit[dest_obj] = tmp;
    }
    else
    {
      //add message to queue of objects in transit.
      mObjectsInTransit[dest_obj].push_back(msg);
    }
  }



  ServerID Forwarder::lookup(const Vector3f& pos)
  {
    ServerID sid = mCSeg->lookup(pos);
    return sid;
  }

  /*
    May return that it's in transit.
  */
  /*
  ServerID Forwarder::lookup(const UUID& obj_id)
  {

    ServerID sid = mOSeg->lookup(obj_id);
    //need to change this lookup function to query oseg instead.

//    Vector3f pos = mLocationService->currentPosition(obj_id);
//    ServerID sid = mCSeg->lookup(pos);

    return sid;
  }
  */

void Forwarder::addObjectConnection(const UUID& dest_obj, ObjectConnection* conn) {
    mObjectConnections[dest_obj] = conn;
    mObjectMessageQueue->registerClient(dest_obj, 1); // FIXME weight?
}

ObjectConnection* Forwarder::removeObjectConnection(const UUID& dest_obj) {
    mObjectMessageQueue->unregisterClient(dest_obj);
    ObjectConnection* conn = mObjectConnections[dest_obj];
    mObjectConnections.erase(dest_obj);
    return conn;
}

ObjectConnection* Forwarder::getObjectConnection(const UUID& dest_obj) {
    ObjectConnectionMap::iterator it = mObjectConnections.find(dest_obj);
    return (it == mObjectConnections.end()) ? NULL : it->second;
}

} //end namespace
