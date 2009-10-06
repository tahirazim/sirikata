#ifndef _CBR_SENDQUEUE_HPP
#define _CBR_SENDQUEUE_HPP

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"
#include "ServerIDMap.hpp"
#include "ServerProtocolMessagePair.hpp"
#include "ServerMessagePair.hpp"

namespace CBR{

typedef struct QueueInfo{
  uint32 mTXSize;
  uint32 mTXUsed;
  float mTXWeight;

  uint32 mRXSize;
  uint32 mRXUsed;
  float mRXWeight;

  QueueInfo(uint32 tx_size, uint32 tx_used, float tx_weight,
	    uint32 rx_size, uint32 rx_used, float rx_weight
	   )
  {
    mTXSize = tx_size;
    mTXUsed = tx_used;
    mTXWeight = tx_weight;

    mRXSize = rx_size;
    mRXUsed = rx_used;
    mRXWeight = rx_weight;
  }


} QueueInfo;

class ServerMessageQueue {
public:
    ServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap)
     : mContext(ctx),
       mNetwork(net),
       mServerIDMap(sidmap)
    {
        // start the network listening
        Address4* listen_addy = mServerIDMap->lookupInternal(mContext->id());
        assert(listen_addy != NULL);
        net->listen(*listen_addy);
    }

    virtual ~ServerMessageQueue(){}

    /** Try to add the given message to this queue.
     *  \param msg the message to try to push onto the queue.
     *  \returns true if the message was added, false otherwise
     *  \note If successful, the queue takes possession of the message and ensures it is disposed of.
     *        If unsuccessful, the message is still owned by the caller.
     */
    virtual bool addMessage(Message* msg)=0;
    /** Check if a message could be pushed on the queue.  If this returns true, an immediate subsequent
     *  call to addMessage() will always be successful.
     *  \param msg the message to try to push onto the queue.
     *  \returns true if the message was added, false otherwise
     */
    virtual bool canAddMessage(const Message* msg)=0;

    virtual bool receive(Network::Chunk** chunk_out, ServerID* source_server_out) = 0;
    virtual void service() = 0;

    virtual void setServerWeight(ServerID sid, float weight) = 0;

    virtual void reportQueueInfo(const Time& t) const = 0;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const = 0;
protected:
    bool canSend(const ServerMessagePair*msg) {
        if (msg->dest()==mContext->id()) return true;
        Address4* addy = mServerIDMap->lookupInternal(msg->dest());

        assert(addy != NULL);
        return mNetwork->canSend(*addy,msg->data().size(),false,true,1);
    }

    SpaceContext* mContext;
    Network* mNetwork;
    ServerIDMap* mServerIDMap;
};
}

#endif
