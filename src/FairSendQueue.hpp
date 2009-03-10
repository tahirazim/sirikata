#ifndef _CBR_FAIRSENDQUEUE_HPP
#define _CBR_FAIRSENDQUEUE_HPP
#include "SendQueue.hpp"
#include "FairMessageQueue.hpp"
namespace CBR {
class FairSendQueue:public SendQueue {
    class ServerMessagePair {
    private:
        std::pair<ServerID,Network::Chunk> mPair;
    public:
        ServerMessagePair(const ServerID&sid, const Network::Chunk&data):mPair(sid,data){}
        //destructively modifies the data chunk to quickly place it in the queue
        ServerMessagePair(const ServerID&sid, Network::Chunk&data):mPair(sid,Network::Chunk()){
            mPair.second.swap(data);
        }
        unsigned int size()const {
            return mPair.second.size();
        }

        ServerID dest() const {
            return mPair.first;
        }

        const Network::Chunk data() const {
            return mPair.second;
        }
    };
    FairMessageQueue<ServerMessagePair,UUID> mClientQueues;
    std::vector<ServerMessagePair*> mClientServerBuffer;
    FairMessageQueue<ServerMessagePair, ServerID > mServerQueues;
    Network * mNetwork;
public:

    FairSendQueue(Network*net, uint32 bytes_per_second, BandwidthStatistics* bstats);

    void registerServer(ServerID, float weight);
    void removeServer(ServerID);
    void registerClient(UUID,float weight);
    void removeClient(UUID);

    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg);
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj);
    virtual void service(const Time&t);
};
}
#endif
