


#include "ObjectSegmentation.hpp"
#include "Message.hpp"
#include <map>
#include <vector>
#include "Statistics.hpp"
#include "Timer.hpp"
#include "CraqObjectSegmentation.hpp"
#include "craq_oseg/asyncCraq.hpp"
#include "craq_oseg/asyncUtil.hpp"
#include "craq_oseg/asyncConnection.hpp"


#include "CoordinateSegmentation.hpp"
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include "CraqCacheGood.hpp"
#include <boost/thread/mutex.hpp>

namespace CBR
{

  /*
    Basic constructor
  */
  CraqObjectSegmentation::CraqObjectSegmentation (SpaceContext* ctx, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> getInitArgs, std::vector<CraqInitializeArgs> setInitArgs, char prefixID, IOStrand* o_strand, IOStrand* strand_to_post_to)
 : ObjectSegmentation(ctx, o_strand),
   mCSeg (cseg),
   craqDhtGet(ctx, o_strand, this),
   craqDhtSet(ctx, o_strand, this),
   postingStrand(strand_to_post_to),
   mStrand(o_strand)
  {
  
    //registering with the dispatcher.  can now receive messages addressed to it.
    mContext->dispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_MOVE,this);
    mContext->dispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->dispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);

    craqDhtGet.initialize(getInitArgs);
    craqDhtSet.initialize(setInitArgs);

    myUniquePrefixKey = prefixID;

    numCacheHits     = 0;
    numOnThisServer  = 0;
    numLookups       = 0;
    numCraqLookups   = 0;
    numTimeElapsedCacheEviction = 0;
    numMigrationNotCompleteYet  = 0;
    numAlreadyLookingUp         = 0;
    numServices                 = 0;
    numLookingUpDebug           = 0;
    mServiceTimer.start();
    mTimer.start();

    checkOwnTimeDur   = 0;
    checkOwnTimeCount = 0;

    mAtomicTrackID    = 10;
  }

  /*
    Destructor
  */
  CraqObjectSegmentation::~CraqObjectSegmentation()
  {
    mContext->dispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_MOVE,this);
    mContext->dispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->dispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);

    //    should delete not found queu;
    while (mNfData.size() != 0)
    {
      NotFoundData* nfd = mNfData.front();
      mNfData.pop();
      delete nfd;
    }
    

    for (TrackedMessageMapAdded::iterator tmessmapit  = trackedAddMessages.begin(); tmessmapit != trackedAddMessages.end(); ++tmessmapit)
      delete tmessmapit->second.msgAdded;

    trackedAddMessages.clear();
    

    //delete retries
    for (int s=0; s < (int) reTryAddedMessage.size(); ++s)
      delete reTryAddedMessage[s];
    reTryAddedMessage.clear();

    for (int s=0; s < (int) reTryMigAckMessage.size(); ++s)
      delete reTryMigAckMessage[s];
    reTryMigAckMessage.clear();

    for (int s=0; s < (int) reTryKillConnMessage.size(); ++s)
      delete reTryKillConnMessage[s];
    reTryKillConnMessage.clear();
    

    
    
    std::cout<<"\n\n\nIN DESTRUCTOR \n\n";
    mContext->trace()->processOSegShutdownEvents(mContext->time,
                                                 mContext->id(),
                                                 numLookups,
                                                 numOnThisServer,
                                                 numCacheHits,
                                                 numCraqLookups,
                                                 numTimeElapsedCacheEviction,
                                                 numMigrationNotCompleteYet);

    printf("\n\nREMAINING IN QUEUE GET:  %i     SET:  %i\n\n", (int) craqDhtGet.queueSize(),(int) craqDhtSet.queueSize());
    fflush(stdout);
    printf("\n\nNUM ALREADY LOOKING UP:  %i\n\n",numAlreadyLookingUp);
    fflush(stdout);

    std::cout<<"\n\nCheckOwnTimeDur "<<checkOwnTimeDur<<"   checkOwnTimeCount  "<<checkOwnTimeCount<<"  avg: "<<((double)checkOwnTimeDur)/((double)checkOwnTimeCount)<<"\n\n";
    
  }


  /*
    This function checks to see whether the obj_id is hosted on this space server
  */
  bool CraqObjectSegmentation::checkOwn(const UUID& obj_id)
  {
    Duration beginningDur = mTimer.elapsed();
    
    if (std::find(mObjects.begin(),mObjects.end(), obj_id) == mObjects.end())
    {
      //means that the object isn't hosted on this space server

      Duration endingDur = mTimer.elapsed();
      checkOwnTimeDur += endingDur.toMilliseconds() - beginningDur.toMilliseconds();
      ++checkOwnTimeCount;
      
      return false;
    }

    //means that the object *is* hosted on this space server

    Duration endingDur = mTimer.elapsed();
    checkOwnTimeDur += endingDur.toMilliseconds() - beginningDur.toMilliseconds();
    ++ checkOwnTimeCount;
    return true;
  }


  /*
    If returns true, means that the object isn't already undergoing a migration, and is therefore clear to migrate.
  */
  bool CraqObjectSegmentation::clearToMigrate(const UUID& obj_id)
  {
    //set a mutex lock.
    inTransOrLookup_m.lock();
    std::map<UUID,TransLookup>::iterator mInTransIter = mInTransitOrLookup.find(obj_id);

    bool migratingFromHere = false;
    if (mInTransIter != mInTransitOrLookup.end())
    {
      //means that the object is either being looked up or in transit
      if (mInTransIter->second.sID != CRAQ_OSEG_LOOKUP_SERVER_ID)
      {
        //means that the object is migrating from here.
        migratingFromHere = true;
      }
    }
    inTransOrLookup_m.unlock();
    
    receivingObjects_m.lock();
    bool migratingToHere = std::find(mReceivingObjects.begin(), mReceivingObjects.end(), obj_id) != mReceivingObjects.end();
    //means that the object migrating to here has not yet received an acknowledge, and therefore shouldn't begin migrating again.
    receivingObjects_m.unlock();
    
    //clear to migrate only if migrating from here and migrating to here are false.
    return (!migratingFromHere) && (!migratingToHere);
  }



  //this call returns true if the object is migrating from this server to another server, but hasn't yet received an ack message (which will disconnect the object connection.)
  bool CraqObjectSegmentation::checkMigratingFromNotCompleteYet(const UUID& obj_id)
  {
    inTransOrLookup_m.lock();
    std::map<UUID,TransLookup>::const_iterator iterInTransOrLookup = mInTransitOrLookup.find(obj_id);

    if (iterInTransOrLookup == mInTransitOrLookup.end())
    {
      inTransOrLookup_m.unlock();
      return false;
    }
    if (iterInTransOrLookup->second.sID != CRAQ_OSEG_LOOKUP_SERVER_ID)
    {
      inTransOrLookup_m.unlock();
      return true;
    }

    inTransOrLookup_m.unlock();
    return false;
  }


  //checks value against cache.
  //should only be called from the "postingStrand"
  ServerID CraqObjectSegmentation::satisfiesCache(const UUID& obj_id)
  {

#ifndef CRAQ_CACHE
    return NullServerID;
#endif

    return mCraqCache.get(obj_id);
  }


  int CraqObjectSegmentation::getUniqueTrackID()
  {
    atomic_track_id_m.lock();
      int returner = mAtomicTrackID;
      ++mAtomicTrackID;
    atomic_track_id_m.unlock();

    return returner;
  }

  
  void CraqObjectSegmentation::newObjectAdd(const UUID& obj_id)
  {
    std::cout<<"\n\nRequest to add new object:  "<<obj_id.toString()<<"\n\n";
    
    CraqDataKey cdk;
    convert_obj_id_to_dht_key(obj_id,cdk);

    CraqDataSetGet cdSetGet(cdk, mContext->id() ,true,CraqDataSetGet::SET);

    TrackedSetResultsDataAdded tsrda;
    tsrda.msgAdded  = generateAddedMessage(obj_id);
    tsrda.dur       = mTimer.elapsed();

    int trackID = getUniqueTrackID();
    craqDhtSet.set(cdSetGet, trackID);
    trackedAddMessages[trackID] = tsrda;
  }


  CBR::Protocol::OSeg::AddedObjectMessage* CraqObjectSegmentation::generateAddedMessage(const UUID& obj_id)
  {
    CBR::Protocol::OSeg::AddedObjectMessage* oadd = new CBR::Protocol::OSeg::AddedObjectMessage();
    oadd->set_m_objid(obj_id);
    return oadd;
  }


  /*
    After insuring that the object isn't in transit, the lookup should querry the dht.
    Only called from postingStrand
  */
  ServerID CraqObjectSegmentation::lookup(const UUID& obj_id)
  {
    ++numLookups;

    if (checkOwn(obj_id))  //this call just checks through to see whether the object is on this space server.
    {
      ++numOnThisServer;
      return mContext->id();
    }

    //log the request.
    mContext->trace()->objectSegmentationLookupNotOnServerRequest(mContext->time,
                                                                  obj_id,
                                                                  mContext->id()); 


    if (checkMigratingFromNotCompleteYet(obj_id))//this call just checks to see whether the object is migrating from this server to another server.  If it is, but hasn't yet received an ack message to disconnect the object connection.
    {
      ++numMigrationNotCompleteYet;
      return mContext->id();
    }


    ServerID cacheReturn = satisfiesCache(obj_id);
    if ((cacheReturn != NullServerID) && (cacheReturn != mContext->id())) //have to perform second check to prevent accidentally infinitely re-routing to this server when the object doesn't reside here: if the object resided here, then one of the first two conditions would have triggered.
    {
      mContext->trace()->osegCacheResponse(mContext->time,
                                           cacheReturn,
                                           obj_id);
      
      ++numCacheHits;
      return cacheReturn;
    }

    oStrand->post(boost::bind(&CraqObjectSegmentation::beginCraqLookup,this,obj_id));

    return NullServerID;
  }

  void CraqObjectSegmentation::beginCraqLookup(const UUID& obj_id)
  {
    UUID tmper = obj_id;
    std::map<UUID,TransLookup>::const_iterator iter = mInTransitOrLookup.find(tmper);

    if (iter == mInTransitOrLookup.end()) //means that the object isn't already being looked up and the object isn't already in transit
    {
      //if object is not in transit, lookup its location in the dht.  returns -1 if object doesn't exist.
      //add the mapping of a craqData Key to a uuid.

      ++numCraqLookups;

      std::string indexer = "";
      indexer.append(1,myUniquePrefixKey);
      indexer.append(tmper.rawHexData());

      CraqDataSetGet cdSetGet (indexer,0,false,CraqDataSetGet::GET); //bftm modified

      mapDataKeyToUUID[indexer] = tmper; //changed here.

      craqDhtGet.get(cdSetGet); //calling the craqDht to do a get.

      mContext->trace()->objectSegmentationCraqLookupRequest(mContext->time,
                                                             obj_id,
                                                             mContext->id());

      ++numLookingUpDebug;

      //puts object in transit or lookup.
      Duration timerDur = mTimer.elapsed();
      TransLookup tmpTransLookup;
      tmpTransLookup.sID = CRAQ_OSEG_LOOKUP_SERVER_ID;  //means that we're performing a lookup, rather than a migrate.
      tmpTransLookup.timeAdmitted = (int)timerDur.toMilliseconds();
      
      mInTransitOrLookup[tmper] = tmpTransLookup; //just says that we are performing a lookup on the object
    }
    else
    {
      ++numAlreadyLookingUp;
    }
  }

  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  CBR::Protocol::OSeg::MigrateMessageAcknowledge* CraqObjectSegmentation::generateAcknowledgeMessage(const UUID &obj_id,ServerID sID_to)
  {
    CBR::Protocol::OSeg::MigrateMessageAcknowledge* oseg_ack_msg = new CBR::Protocol::OSeg::MigrateMessageAcknowledge();
    oseg_ack_msg->set_m_servid_from(mContext->id());
    oseg_ack_msg->set_m_servid_to(sID_to);
    oseg_ack_msg->set_m_message_destination(sID_to);
    oseg_ack_msg->set_m_message_from(mContext->id());
    oseg_ack_msg->set_m_objid(obj_id);

    return oseg_ack_msg;
  }


  /*
    This is the function that we use to add an object.  generateAck is true if you want to generate an ack message to server idServerAckTo

    If you're initially adding an object to the world, you should use the newObjectAdd function instead.
  */
  void CraqObjectSegmentation::addObject(const UUID& obj_id, const ServerID idServerAckTo, bool generateAck)
  {
    if (generateAck)
    {
      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      CraqDataSetGet cdSetGet(cdk, mContext->id() ,true,CraqDataSetGet::SET);

      receivingObjects_m.lock();

      //shouldn't need to check if it already exists, but may as well.
      if (std::find(mReceivingObjects.begin(),mReceivingObjects.end(),obj_id) == mReceivingObjects.end())
      {
        //means that this object has been pushed to this server, but its migration isn't complete yet.
        mReceivingObjects.push_back(obj_id);  
      }
      receivingObjects_m.unlock();

      TrackedSetResultsData tsrd;
      tsrd.migAckMsg = generateAcknowledgeMessage(obj_id,idServerAckTo);
      tsrd.dur       = mTimer.elapsed();

      int trackID = getUniqueTrackID();
      craqDhtSet.set(cdSetGet,trackID);
      trackingMessages[trackID] = tsrd;
    }
    else
    {
      //potentially clean up: we should really only be adding objects without generating acks using the newObjectAdd interface
      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      CraqDataSetGet cdSetGet(cdk, mContext->id() ,false,CraqDataSetGet::SET);
      int trackID = getUniqueTrackID();
      craqDhtSet.set(cdSetGet, trackID);


      if (std::find(mObjects.begin(), mObjects.end(), obj_id) == mObjects.end())
      {
        mObjects.push_back(obj_id);
        std::cout<<"\n\nAdding object:  "<<obj_id.toString()<<"\n";
      }
    }
  }


  /*
    If we get a message to move an object that our server holds, then we add the object's id to mInTransit.
    Whatever calls this must verify that the object is on this server.
    I can do the check in the function by querrying bambooDht as well
  */
  void CraqObjectSegmentation::migrateObject(const UUID& obj_id, const ServerID new_server_id)
  {
    //log the message.
    mContext->trace()->objectBeginMigrate(mContext->time,
                                          obj_id,mContext->id(),
                                          new_server_id); 

    std::map<UUID,TransLookup>::const_iterator transIter = mInTransitOrLookup.find(obj_id);

    TransLookup tmpTransLookup;
    tmpTransLookup.sID = new_server_id;

    Duration tmpDurer= mTimer.elapsed();
    tmpTransLookup.timeAdmitted = (int)tmpDurer.toMilliseconds();

    mInTransitOrLookup[obj_id] = tmpTransLookup;

    //erases the local copy of obj_id
    UUID tmp_id = obj_id; //note: can probably delete this line

    //erase the local copy of the object.
    std::vector<UUID>::iterator iter = std::find(mObjects.begin(),mObjects.end(),obj_id);
    if (iter != mObjects.end())
    {
      //erase the local copy of the object.
      mObjects.erase (iter);
    }
    else
    {

#ifdef CRAQ_DEBUG
      std::cout<<"\n\nThe object clearly wasn't registered on this server.  This is obj id:  " <<  obj_id.toString() <<  ".  This is time:   " <<mContext->time.raw() << " Oh no.   ";

      if (clearToMigrate(obj_id))
        std::cout<<"  Likely a problem with clear to migrate\n\n";
      else
        std::cout<<"\n\n clear to migrate is fine migration is being called from somewhere besides server.cpp\n\n";
#endif
    }
  }



  void CraqObjectSegmentation::poll()
  {
    static int counter = 0;
    ++counter;
    
    mServiceStage->started();
    //shouldn't really need to do anything in here.
    //    well, maybe try to re-send the messages that failed;
    //    maybe also deal with not founds.


    if (counter > 10000)
    {
      std::cout<<"\n\ninside of craqobjseg poll\n\n";
      counter = 0;
    }
    
    
    mServiceStage->finished();
  }


  
  //should be called from inside of o_strand and posts to postingStrand mainStrand->post.
  void CraqObjectSegmentation::callOsegLookupCompleted(const UUID& obj_id, const ServerID& sID)
  {
    postingStrand->post(std::tr1::bind (&OSegListener::osegLookupCompleted,mListener,obj_id,sID));
  }

  void CraqObjectSegmentation::convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const
  {
    returner[0] = myUniquePrefixKey;
    strncpy(returner+1,obj_id.rawHexData().c_str(),obj_id.rawHexData().size() + 1);
  }


  //This receives messages oseg migrate and acknowledge messages
  void CraqObjectSegmentation::receiveMessage(Message* msg)
  {
      if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_MOVE) {
          CBR::Protocol::OSeg::MigrateMessageMove oseg_move_msg;
          bool parsed = parsePBJMessage(&oseg_move_msg, msg->payload());
          if (parsed)
          {
            printf("\n\nMigrate message move in oseg is a deprecated message.  Killing\n\n");
            assert(false);
          }
      }
      else if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE) {
          CBR::Protocol::OSeg::MigrateMessageAcknowledge oseg_ack_msg;
          bool parsed = parsePBJMessage(&oseg_ack_msg, msg->payload());
          if (parsed)
            mStrand->post(std::tr1::bind(&CraqObjectSegmentation::processMigrateMessageAcknowledge, this,oseg_ack_msg));
      }
      else if (msg->dest_port() == SERVER_PORT_OSEG_UPDATE) {
          CBR::Protocol::OSeg::UpdateOSegMessage update_oseg_msg;
          bool parsed = parsePBJMessage(&update_oseg_msg, msg->payload());
          if (parsed)
            mStrand->post(std::tr1::bind(&CraqObjectSegmentation::processUpdateOSegMessage,this,update_oseg_msg));
      }
      else {
          printf("\n\nReceived the wrong type of message in receiveMessage of craqobjectsegmentation.cpp.\n\n ");
      }

      delete msg;
  }



  //called from within o_strand
  void CraqObjectSegmentation::processUpdateOSegMessage(const CBR::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg)
  {
    postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, update_oseg_msg.m_objid(), update_oseg_msg.servid_obj_on()));
  }

  //called from within o_strand
  void CraqObjectSegmentation::processMigrateMessageAcknowledge(const CBR::Protocol::OSeg::MigrateMessageAcknowledge& msg)
  {
    ServerID serv_from, serv_to;
    UUID obj_id;

    serv_from = msg.m_servid_from();
    serv_to   = msg.m_servid_to();
    obj_id    = msg.m_objid();


    std::map<UUID,TransLookup>::iterator inTransIt;

    postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, obj_id, serv_from));
    
    inTransOrLookup_m.lock();
    inTransIt = mInTransitOrLookup.find(obj_id);
    if (inTransIt != mInTransitOrLookup.end())
    {
      //means that we now remove the obj_id from mInTransit
      mInTransitOrLookup.erase(inTransIt);
      //add it to mFinishedMove.  serv_from

      //put the value in the cache!
      postingStrand->post(std::tr1::bind(&CraqCacheGood::insert, &mCraqCache, obj_id, serv_from));
      callOsegLookupCompleted(obj_id,serv_from);

      
      //log reception of acknowled message
      mContext->trace()->objectAcknowledgeMigrate(mContext->time,
                                                  obj_id,serv_from,
                                                  mContext->id());
    }
    inTransOrLookup_m.unlock();
    
    //send a message to the server that object should now disconnect
    // FIXME is this really supposed to go to the same server? If so, there *must* be a better way to accomplish this
    CBR::Protocol::ObjConnKill::ObjConnKill kill_msg_contents;
    kill_msg_contents.set_m_objid(obj_id);
    Message* kill_msg = new Message(
        mContext->id(),
        SERVER_PORT_KILL_OBJ_CONN,
        mContext->id(),
        SERVER_PORT_KILL_OBJ_CONN,
        serializePBJMessage(kill_msg_contents)
    );
    bool sent = mContext->router()->route(MessageRouter::MIGRATES, kill_msg);//this will route the message to the server so that this computer can disconnect from the object connection.

    if (!sent)
      reTryKillConnMessage.push_back(kill_msg);

  }


  //This function tries to re-send all the messages that failed to be delivered
  //should probably be called from within o_strand
  void CraqObjectSegmentation::checkReSends()
  {
    Timer checkSendTimer;
    checkSendTimer.start();


    std::vector<Message*> reReTryAddedMessage;
    std::vector<Message*> reReTryMigAckMessage;
    std::vector<Message*> reReTryKillConnMessage;


    //trying to resend faild added messages
    for (int s=0;s < (int)reTryAddedMessage.size(); ++s)
    {
      bool sent = mContext->router()->route(MessageRouter::MIGRATES,reTryAddedMessage[s],false);

      if (!sent)
        reReTryAddedMessage.push_back(reTryAddedMessage[s]);

    }


    //trying to re-send failed mig ack messages
    for (int s=0; s< (int)reTryMigAckMessage.size(); ++s)
    {
      bool sent= mContext->router()->route(MessageRouter::MIGRATES,reTryMigAckMessage[s],false);
      if (!sent)
        reReTryMigAckMessage.push_back(reTryMigAckMessage[s]);

    }

    //trying to re-send failed kill conn messages
    for (int s=0; s <(int) reTryKillConnMessage.size(); ++s)
    {
      bool sent = mContext->router()->route(MessageRouter::MIGRATES,reTryKillConnMessage[s]);//this will route the message to the server so that this computer can disconnect from the object connection.

      if (!sent)
        reTryKillConnMessage.push_back(reTryKillConnMessage[s]);

    }

    //load the messages that failed from this set of operations back so that can try to resend them next time this function is called.
    reTryAddedMessage.clear();
    reReTryAddedMessage.swap(reTryAddedMessage);

    reTryMigAckMessage.clear();
    reReTryMigAckMessage.swap(reTryMigAckMessage);

    reTryKillConnMessage.clear();
    reReTryKillConnMessage.swap(reTryKillConnMessage);
  }


  //this function tells us what to do with all the ids that just weren't found in craq.
  void CraqObjectSegmentation::notFoundFunction(CraqOperationResult* nf)
  {
    //turn the id into a uuid and then push it onto the end of  mNfData queue.
    if (mapDataKeyToUUID.find(nf->idToString()) == mapDataKeyToUUID.end())
      return; //means that we really have no record of this objects' even having been requested.

    NotFoundData* nfd = new NotFoundData();
    nfd->obj_id =  mapDataKeyToUUID[nf->idToString()];
    nfd->dur    =  mTimer.elapsed();
    mNfData.push(nfd);
  }


  //this function tells us if there are any ids that have waited long enough for it to be safe to query for the again.
  void CraqObjectSegmentation::checkNotFoundData()
  {
    //look at the first element of the queue.  if the time hasn't been sufficiently long, return.  if the time has been sufficiently long, then go ahead and directly request asyncCraq to perform another lookup.
    
    if (mNfData.size() == 0)
      return;

    std::cout<<"\n\nGOT NOT FOUND\n\n";

    Duration tmpDur = mTimer.elapsed();
    
    bool queueDataOldEnough = true;
    NotFoundData* nfd;

    while (queueDataOldEnough)
    {
      nfd = mNfData.front();
      queueDataOldEnough = false;

      if ((tmpDur.toMilliseconds() - nfd->dur.toMilliseconds()) > CRAQ_NOT_FOUND_SIT_OUT) //we wait 500 ms.
      {
        queueDataOldEnough = true;

        //perform a direct craq get call
        std::string indexer = "";
        indexer.append(1,myUniquePrefixKey);
        indexer.append(nfd->obj_id.rawHexData());
        
        CraqDataSetGet cdSetGet (indexer,0,false,CraqDataSetGet::GET); //bftm modified
        craqDhtGet.get(cdSetGet); //calling the craqDht to do a get.

        mNfData.pop(); //remove the item from the queue.

        delete nfd; //memory managment
      }
      queueDataOldEnough = queueDataOldEnough && (mNfData.size() > 0);  //don't keep going if you're out of objects.
    }
  }

  //gets posted to from asyncCraqGet.  Should get inside of o_strand from the post.
  void CraqObjectSegmentation::craqGetResult(CraqOperationResult* cor)
  {
    if (cor->servID == NullServerID)
    {
      notFoundFunction(cor);
      delete cor;

      std::cout<<"\n\nWe have an object that does not exist in craq system.  This shouldn't have really been called.\n\n";

      std::cout<<"Object:  "<<cor->objID<<"  server id:  "<<cor->servID<<"  tracking  "<<cor->tracking<<"  suceeded: "<<cor->succeeded<<"  GET or set:   "<<cor->whichOperation<<"\n\n";
        
      assert(false);
      return;
    }

    UUID tmper = mapDataKeyToUUID[cor->idToString()];

    //put the value in the cache!
    postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, tmper, cor->servID));
    
    
    inTransOrLookup_m.lock();
    std::map<UUID,TransLookup>::iterator iter = mInTransitOrLookup.find(tmper);
    
    if (iter != mInTransitOrLookup.end()) //means that the object was already being looked up or in transit
    {
      //log message stating that object was processed.
      Duration timerDur = mTimer.elapsed();
      mContext->trace()->objectSegmentationProcessedRequest(mContext->time,
                                                            tmper,
                                                            cor->servID,
                                                            mContext->id(),
                                                            (uint32) (((int) timerDur.toMilliseconds()) - (int)(iter->second.timeAdmitted)),
                                                            (uint32) craqDhtGet.queueSize()  );


      uint32 timeItTook = (uint32) (((int) timerDur.toMilliseconds()) - (int)(iter->second.timeAdmitted));
      std::cout<<"\n\nProcessed a lookup request:  " << tmper.toString()<<"    serv id:"<<cor->servID<<" duration:  "<< timeItTook<<  " \n\n";
      
      if(iter->second.sID ==  CRAQ_OSEG_LOOKUP_SERVER_ID)
      {
        //means that after receiving a lookup request, we did not intermediarilly receive a migrate request.
        //if above predicate is not true, means that we did receive a migrate request, and therefore should not delete that migrate request from mInTransitOrLookup (ie do nothing).
        mInTransitOrLookup.erase(iter);
      }
    }

    inTransOrLookup_m.unlock();
    callOsegLookupCompleted(tmper,cor->servID);

    delete cor;
  }


  //locks intransor lookup and removes the
  //which we already know has an entry in trackingMessages
  void CraqObjectSegmentation::removeFromInTransOrLookup(const UUID& obj_id)
  {
    inTransOrLookup_m.lock();
      
    //delete the mInTransitOrLookup entry for this object sequence because now we know where it is.
    std::map<UUID,TransLookup>::iterator inTransLookIter = mInTransitOrLookup.find(obj_id);

    if (inTransLookIter != mInTransitOrLookup.end())
    {
      //means that object can now be removed from mInTransitOrLookup
      mInTransitOrLookup.erase(inTransLookIter);
    }

    //finished deleting from mInTransitOrLookup
    inTransOrLookup_m.unlock();
  }


  //Assumes that the result as an entry in trackingMessages
  void CraqObjectSegmentation::removeFromReceivingObjects(const UUID& obj_id)
  {
    //remove this object from mReceivingObjects,
    //this removal indicates that this object can now be safely migrated from this server to another if need be.
    receivingObjects_m.lock();
    std::vector<UUID>::iterator recObjIter = std::find(mReceivingObjects.begin(), mReceivingObjects.end(), obj_id);
    if (recObjIter != mReceivingObjects.end())
    {
      //the object should be removed from receiving objects
      mReceivingObjects.erase(recObjIter);
    }
    
    //done removing from receivingObjects.
    receivingObjects_m.unlock();
  }

  
  //gets posted to from asyncCraqSet.  Should get inside of o_strand from the post.
  void CraqObjectSegmentation::craqSetResult(CraqOperationResult* trackedSetResult)
  {
    if(trackedSetResult->trackedMessage == 0)  //means that we weren't supposed to be tracking this message
    {
      delete trackedSetResult;
      return;
    }

    
    if (trackingMessages.find(trackedSetResult->trackedMessage) != trackingMessages.end())
    {
      //means that we have a record of locally tracking this message and that we'll have to send an ack.

      UUID obj_id = trackingMessages[trackedSetResult->trackedMessage].migAckMsg->m_objid();
      
      //add this to the cache
      postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, obj_id, mContext->id()));

      //add tomObjects the uuid associated with trackedMessage (ie, now we know that we own the object.)
      mObjects.push_back(obj_id);

      //delete the mInTransitOrLookup entry for this object sequence because now we know where it is.
      removeFromInTransOrLookup(obj_id);

      //remove this object from mReceivingObjects,
      removeFromReceivingObjects(obj_id);

      //log event
      Duration procTrackedSetRes = mTimer.elapsed();
      int durMs = procTrackedSetRes.toMilliseconds() - trackingMessages[trackedSetResult->trackedMessage].dur.toMilliseconds();
      
      mContext->trace()->processOSegTrackedSetResults(mContext->time,
                                                      obj_id,
                                                      mContext->id(),
                                                      durMs);

      //send an acknowledge message to space server that formerly hosted object.
      Message* to_send = new Message(
                                     mContext->id(),
                                     SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,
                                     trackingMessages[trackedSetResult->trackedMessage].migAckMsg->m_message_destination(),
                                     SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,
                                     serializePBJMessage( *(trackingMessages[trackedSetResult->trackedMessage].migAckMsg) )
                                     );
          
      bool sent= mContext->router()->route(MessageRouter::MIGRATES,to_send,false);

      if (!sent)
        reTryMigAckMessage.push_back(to_send); //will try to re-send the tracking message

      trackingMessages.erase(trackedSetResult->trackedMessage);//stop tracking this message.
    }
    else if (trackedAddMessages.find(trackedSetResult->trackedMessage) != trackedAddMessages.end())
    {
      
      //means that we just finished adding first object
      mObjects.push_back(trackedAddMessages[trackedSetResult->trackedMessage].msgAdded->m_objid() );//need to add obj_id

      Message* to_send = new Message(
                                     mContext->id(),
                                     SERVER_PORT_OSEG_ADDED_OBJECT,
                                     mContext->id(),
                                     SERVER_PORT_OSEG_ADDED_OBJECT,
                                     serializePBJMessage( *(trackedAddMessages[trackedSetResult->trackedMessage].msgAdded) )
                                     );

      bool sent = mContext->router()->route(MessageRouter::MIGRATES, to_send,false);

      if (!sent)
        reTryAddedMessage.push_back(to_send);  //will try to re-send the add message
    }

    delete trackedSetResult;
    return;
  }

}//namespace CBR
