#include "ObjectSegmentation.hpp"
#include "Utility.hpp"
#include "Statistics.hpp"
#include "Message.hpp"
#include "Statistics.hpp"
#include "OSegLookupTraceToken.hpp"
#include <queue>

#include <sirikata/core/util/ThreadSafeQueueWithNotification.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include "BatchCraqQueue.hpp"

namespace Sirikata
{

  BatchCraqQueue::BatchCraqQueue(IOStrand* strand, ObjectSegmentation* oseg, SpaceContext* con)
    :mOSeg(oseg),
     mStrand(strand),
     ctx(con)
  {
    mQueryQueue = new QueryQueue();
  }


  BatchCraqQueue::~BatchCraqQueue()
  {
    //take off each query and delete it.
    while (! mQueryQueue->empty())
    {
      Query* toDelete = mQueryQueue->front();
      mQueryQueue->pop();
      delete toDelete;
    }
    //delete mQueryQueue;
    delete mQueryQueue;

    //delete my own strand as well.
    delete mStrand;
  }

  //just a wrapper for posting to internalAddToQueue through mStrand
  void BatchCraqQueue::addToQueue(Query* q)
  {
    //note: may have problems because this is a private function
    mStrand->post(boost::bind(&BatchCraqQueue::internalAddToQueue,this, q));
  }

  void BatchCraqQueue::internalAddToQueue(Query* q)
  {
    mQueryQueue->push(q);
    if((int)mQueryQueue->size() > MAX_LENGTH_QUEUE)
      flushQueue();
  }

  void BatchCraqQueue::flushQueue()
  {
    QueryQueue* toPass = mQueryQueue;
    mOSeg->lookupQueue(toPass);
    mQueryQueue = new QueryQueue();
  }



}
