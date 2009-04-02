#ifndef _RAKNET_NETWORK_HPP_
#define _RAKNET_NETWORK_HPP_

#include "Network.hpp"
#include "raknet/MessageIdentifiers.h"
#include "raknet/RakNetworkFactory.h"
#include "raknet/RakPeerInterface.h"
#include "raknet/RakNetStatistics.h"
#include "raknet/RakNetTypes.h"
#include "raknet/BitStream.h"
#include "Queue.hpp"

namespace CBR {

class RaknetNetwork :public Network{
    class AddressHasher {
    public:
        size_t operator()(const Address4& address)const{
            return std::tr1::hash<unsigned int>()(address.ip)^
                std::tr1::hash<unsigned short>()(address.port);
        }
    };
    class AddressEquals {
    public:
        size_t operator()(const Address4& a,const Address4& b)const{
            return a.ip==b.ip&&a.port==b.port;
        }
    };
    class SentQueue{
    public:
        typedef std::vector<std::pair<Sirikata::Network::Chunk,std::pair<PacketPriority,PacketReliability> > > SendQueue;
        SendQueue mToBeSent;
    };
    Sirikata::Network::Chunk *makeChunk(RakPeerInterface*,Packet*);
    RakPeerInterface *mListener;
    typedef std::tr1::unordered_map<Address4,SentQueue, AddressHasher, AddressEquals > ConnectingMap;
    ConnectingMap mConnectingSockets;
    bool sendRemainingItems(SystemAddress address);

    typedef std::map<Address4, Queue<Chunk*>*> ReceiveQueueMap;
    ReceiveQueueMap mReceiveQueues;
    Packet* mOutstandingPacket; // raknet doesn't allow us to peek at the front packet

    Packet* nextPacket();
    Queue<Chunk*>* getReceiveQueue(const Address4& addr);
public:
    RaknetNetwork();

    virtual bool send(const Address4&,const Sirikata::Network::Chunk&, bool reliable, bool ordered, int priority);
    virtual void init(void*(*)(void*));
    virtual void listen (const Address4& as_server);
    virtual Network::Chunk* receiveOne(const Address4& from, uint32 max_size);
    virtual void service(const Time& t);
};

}

#endif //_RAKNET_NETWORK_HPP_
