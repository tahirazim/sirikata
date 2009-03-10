#include "RaknetNetwork.hpp"
#include "ServerIDMap.hpp"

       #include <sys/socket.h>
       #include <netinet/in.h>
       #include <arpa/inet.h>

namespace CBR {
RaknetNetwork::RaknetNetwork (ServerIDMap*idmap):Network(idmap),mListener(RakNetworkFactory::GetRakPeerInterface()){
}

void RaknetNetwork::listen(const ServerID& as_server) {
    Address4* addr = mServerIDMap->lookup(as_server);
    assert(addr != NULL);
    SocketDescriptor socketDescriptor(addr->getPort(),0);
    bool starting=mListener->Startup(16383,30,&socketDescriptor,1);
       mListener->SetMaximumIncomingConnections(16383);
    assert(starting);
    mListener->SetOccasionalPing(true);
}
Address4 MakeAddress(const SystemAddress &address) {
    return Address4(address.binaryAddress,address.port);
}
SystemAddress MakeAddress(const Address4 &address) {
    SystemAddress sa;
    sa.binaryAddress=address.ip;
    sa.port=address.port;
    return sa;
}
void ConnectTo(RakPeerInterface*ri,const SystemAddress &sa) {
    struct in_addr temp;
    temp.s_addr=sa.binaryAddress;
    std::string hostname(inet_ntoa(temp));
    bool evenTried=ri->Connect(hostname.c_str(),
                                      sa.port,
                                      0,
                                      0);
    assert(evenTried);

}
bool RaknetNetwork::sendTo(const Address4&addy, const Sirikata::Network::Chunk& toSend, bool reliable, bool ordered, int priority) {
    if (toSend.size()==0)
        return false;
    SystemAddress sa=MakeAddress(addy);
    if (mConnectingSockets.find(addy)==mConnectingSockets.end()&&!mListener->IsConnected(sa,true,false)) {
        ConnectTo(mListener,sa);
    }
    PacketReliability rel;
    if (ordered==true&&reliable==true) {
        rel=RELIABLE_ORDERED;
    }
    if (ordered==false&&reliable==true){
        rel=RELIABLE;
    }
    if (ordered==false&&reliable==false){
        rel=UNRELIABLE;
    }
    if (ordered==true&&reliable==false){
        rel=UNRELIABLE_SEQUENCED;
    }
    PacketPriority pri=(priority==0?HIGH_PRIORITY:(priority==1?MEDIUM_PRIORITY:LOW_PRIORITY));
    Sirikata::Network::Chunk paddedToSend(1);
    paddedToSend[0]=ID_USER_PACKET_ENUM;
    paddedToSend.insert(paddedToSend.end(),toSend.begin(),toSend.end());
    if (!mListener->IsConnected(sa,false,false)) {
        SentQueue::SendQueue* tosend=&mConnectingSockets[addy].mToBeSent;
        tosend->resize(tosend->size()+1);
        tosend->back().first=paddedToSend;
        tosend->back().second.first=pri;
        tosend->back().second.second=rel;
        return true;
    }else {
        assert(MakeAddress(sa)==addy);
        sendRemainingItems(sa);
        return mListener->Send((const char*)&*paddedToSend.begin(),toSend.size()+1,pri,rel,0,sa,false);
    }
}
Sirikata::Network::Chunk*RaknetNetwork::makeChunk(RakPeerInterface*i,Packet*p) {

    Sirikata::Network::Chunk*retval=new Sirikata::Network::Chunk(0);
    retval->insert(retval->end(),p->data+1,p->data+p->length);
    i->DeallocatePacket(p);
    return retval;
}
bool RaknetNetwork::sendRemainingItems(SystemAddress address) {
    Address4 a=MakeAddress(address);
    ConnectingMap::iterator where=mConnectingSockets.find(a);
    if (where!=mConnectingSockets.end()) {
        SentQueue::SendQueue v;
        v.swap(where->second.mToBeSent);
        mConnectingSockets.erase(where);
        for (SentQueue::SendQueue::iterator i=v.begin(),ie=v.end();i!=ie;++i) {
            bool retval=mListener->Send((const char*)&*i->first.begin(),i->first.size(),i->second.first,i->second.second,0,address,false);
            assert(retval);
        }
        return true;
    }
    return false;
}

Sirikata::Network::Chunk*RaknetNetwork::receiveOne() {
    Packet*p;
    while ((p=mListener->Receive())) {
        unsigned char packetIdentifier = p->data[0];
        //fprintf (stderr,"GOT SOMETHING!!!%d\n",packetIdentifier);
        switch (packetIdentifier) {
          case ID_ALREADY_CONNECTED:
            sendRemainingItems(p->systemAddress);
            break;
          case ID_NEW_INCOMING_CONNECTION:
          case ID_REMOTE_NEW_INCOMING_CONNECTION:
            sendRemainingItems(p->systemAddress);
            break;
          case ID_NO_FREE_INCOMING_CONNECTIONS:
            fprintf (stderr,"NO FREE INCOMING CONNECTIONS:Retrying\n");
          case ID_DISCONNECTION_NOTIFICATION:
          case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server tel
          case ID_REMOTE_CONNECTION_LOST:
          case ID_CONNECTION_BANNED:
          case ID_CONNECTION_ATTEMPT_FAILED:
          case ID_MODIFIED_PACKET:
            // Cheater!

          case ID_INVALID_PASSWORD:
          case ID_CONNECTION_LOST:
            if (mConnectingSockets.find(MakeAddress(p->systemAddress))!=mConnectingSockets.end()) {
                ConnectTo(mListener,p->systemAddress);
            }
            break;
          case ID_CONNECTION_REQUEST_ACCEPTED:
            sendRemainingItems(p->systemAddress);
            break;
          case ID_USER_PACKET_ENUM:
            return makeChunk(mListener,p);
          default:
            fprintf(stderr,"New unknown error message: %d\n",ID_USER_PACKET_ENUM);
            break;
        }
    }
    return  NULL;
}


}
