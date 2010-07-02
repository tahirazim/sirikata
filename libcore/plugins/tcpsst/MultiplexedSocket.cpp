/*  Sirikata Network Utilities
 *  MultiplexedSocket.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/network/Stream.hpp>
#include "TcpsstUtil.hpp"
#include "TCPStream.hpp"
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include "ASIOSocketWrapper.hpp"
#include "ASIOReadBuffer.hpp"
#include "MultiplexedSocket.hpp"
#include "ASIOConnectAndHandshake.hpp"
#include "TCPSetCallbacks.hpp"

#include <boost/thread.hpp>

#define ASIO_SEND_BUFFER_SIZE 1500


namespace Sirikata {
namespace Network {

boost::mutex MultiplexedSocket::sConnectingMutex;
void MultiplexedSocket::unpauseSendStreams(const std::vector<Stream::StreamID>&toUnpause) {
    if (toUnpause.size()) {
        std::deque<StreamIDCallbackPair> registrations;
        CommitCallbacks(registrations,MultiplexedSocket::CONNECTED,false);
        for (std::vector<Stream::StreamID>::const_iterator i=toUnpause.begin(),ie=toUnpause.end();i!=ie;++i) {
            CallbackMap::iterator where=mCallbacks.find(*i);
            if (where!=mCallbacks.end()) {
                where->second->mReadySendCallback();
            }else {
                SILOG(tcpsst,debug,"Stream  unpaused but no callback registered for it\n");//Call commit callbacks
            }
        }
    }
}

void triggerMultiplexedConnectionError(MultiplexedSocket*socket,ASIOSocketWrapper*wrapper,const boost::system::error_code &error){
    socket->hostDisconnectedCallback(wrapper,error);
}

void MultiplexedSocket::ioReactorThreadCommitCallback(StreamIDCallbackPair& newcallback){
    if (newcallback.mCallback==NULL) {
        //make sure that a new substream callback won't be sent for outstanding closing streams
        mOneSidedClosingStreams.insert(newcallback.mID);
        CallbackMap::iterator where=mCallbacks.find(newcallback.mID);
        if (where!=mCallbacks.end()) {
            delete where->second;
            mCallbacks.erase(where);
        }else {
            SILOG(tcpsst,error,"ERROR in finding callback to erase for stream ID "<<newcallback.mID.read());
            //assert("ERROR in finding callback to erase for stream ID"&&false);
        }
    }else {
        mCallbacks.insert(newcallback.pair());
    }
}

bool MultiplexedSocket::CommitCallbacks(std::deque<StreamIDCallbackPair> &registration, SocketConnectionPhase status, bool setConnectedStatus) {
    SerializationCheck::Scoped ss(this);

    bool statusChanged=false;
    if (setConnectedStatus||!mCallbackRegistration.empty()) {
        if (status==CONNECTED) {
            //do a little house cleaning and empty as many new requests as possible
            std::deque<RawRequest> newRequests;
            {
                boost::lock_guard<boost::mutex> connecting_mutex(sConnectingMutex);
                if (mNewRequests) {
                    mNewRequests->popAll(&newRequests);
                    delete mNewRequests;
                    mNewRequests=NULL;
                }
            }
            for (std::deque<RawRequest>::iterator i=newRequests.begin(),ie=newRequests.end();i!=ie;++i) {
                sendBytesNow(getSharedPtr(),*i, true/*force since we promised to send them out*/);
            }

        }
        boost::lock_guard<boost::mutex> connecting_mutex(sConnectingMutex);
        statusChanged=(status!=mSocketConnectionPhase);
        if (setConnectedStatus) {
            if (status!=CONNECTED) {
                mSocketConnectionPhase=status;
            } else {
                mSocketConnectionPhase=WAITCONNECTING;
                std::deque<RawRequest> newRequests;
                {
                    if (mNewRequests) {
                        mNewRequests->popAll(&newRequests);
                        delete mNewRequests;
                        mNewRequests=NULL;
                    }
                }
                for (std::deque<RawRequest>::iterator i=newRequests.begin(),ie=newRequests.end();i!=ie;++i) {
                    sendBytesNow(getSharedPtr(),*i, true/*force since we promised to send them out*/);
                }
                delete mNewRequests;
                mNewRequests=NULL;
                mSocketConnectionPhase=CONNECTED;
            }
        }
        bool other_registrations=registration.empty();
        mCallbackRegistration.swap(registration);
    }
    while (!registration.empty()) {
        ioReactorThreadCommitCallback(registration.front());
        registration.pop_front();
    }

    return statusChanged;
}

size_t MultiplexedSocket::leastBusyStream(size_t favored) {


    size_t retval=rand()%mSockets.size();
    if(0)if (favored==retval||mSockets[favored].getResourceMonitor().filledSize()<mSockets[retval].getResourceMonitor().filledSize()) {
        return favored;
    }
    return retval;
}

float MultiplexedSocket::dropChance(const Chunk*data,size_t whichStream) {
    return .25;
}

bool MultiplexedSocket::sendBytesNow(const MultiplexedSocketPtr& thus,const RawRequest&data, bool force) {
    TCPSSTLOG(this,"sendnow",&*data.data->begin(),data.data->size(),false);
    TCPSSTLOG(this,"sendnow","\n",1,false);
    static Stream::StreamID::Hasher hasher;
    if (data.originStream==Stream::StreamID()) {
        unsigned int socket_size=(unsigned int)thus->mSockets.size();
        for(unsigned int i=1;i<socket_size;++i) {
            thus->mSockets[i].rawSend(thus,new Chunk(*data.data),true);
        }
        thus->mSockets[0].rawSend(thus,data.data,true);
        return true;
    }else {
        size_t whichStream=hasher(data.originStream)%thus->mSockets.size();
        if (data.unordered) {
            whichStream=thus->leastBusyStream(whichStream);
        }
        if (data.unreliable==false||rand()/(float)RAND_MAX>thus->dropChance(data.data,whichStream)) {
            return thus->mSockets[whichStream].rawSend(thus,data.data,force);
        }else {
            return true;
        }
    }
}


void MultiplexedSocket::closeStream(const MultiplexedSocketPtr& thus,const Stream::StreamID&sid,TCPStream::TCPStreamControlCodes code) {
    RawRequest closeRequest;
    closeRequest.originStream=Stream::StreamID();//control packet
    closeRequest.unordered=false;
    closeRequest.unreliable=false;
    closeRequest.data=ASIOSocketWrapper::constructControlPacket(thus, code,sid);

    sendBytes(thus,closeRequest);
}

bool MultiplexedSocket::canSendBytes(Stream::StreamID originStream,size_t dataSize)const{
    if (mSocketConnectionPhase==CONNECTED) {
        static Stream::StreamID::Hasher hasher;
        size_t whichStream=hasher(originStream)%mSockets.size();
        return mSockets[whichStream].canSend(dataSize);
    }else {
        //FIXME should we give a blank check to unconnected streams or should we tell them false--cus it won't get sent until later
        return false;
    }
}

bool MultiplexedSocket::sendBytes(const MultiplexedSocketPtr& thus,const RawRequest&data, unsigned int maxQueueSize) {
    bool retval=false;
    if (thus->mSocketConnectionPhase==CONNECTED) {
        retval=sendBytesNow(thus,data,false);
    }else {
        bool lockCheckConnected=false;
        {
            boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
            if (thus->mSocketConnectionPhase==CONNECTED) {
                lockCheckConnected=true;
            }else if(thus->mSocketConnectionPhase==DISCONNECTED) {
                //retval=false;
                //FIXME is this the correct thing to do?
                TCPSSTLOG(this,"sendnvr",&*data.data->begin(),data.data->size(),false);
                TCPSSTLOG(this,"sendnvr","\n",1,false);
            }else {
                //with the connectionMutex acquired, no socket is allowed to be in the mSocketConnectionPhase
                assert(thus->mSocketConnectionPhase==PRECONNECTION);
                TCPSSTLOG(this,"sendl8r",&*data.data->begin(),data.data->size(),false);
                TCPSSTLOG(this,"sendl8r","\n",1,false);
                if (thus->mNewRequests==NULL) {
                    thus->mNewRequests=new SizedThreadSafeQueue<RawRequest>(SizedResourceMonitor(maxQueueSize));
                }
                retval=thus->mNewRequests->push(data,false);
            }
        }
        if (lockCheckConnected) {
            retval=sendBytesNow(thus,data,false);
        }
    }
    return retval;
}

MultiplexedSocket::SocketConnectionPhase MultiplexedSocket::addCallbacks(const Stream::StreamID&sid,
                                                                         TCPStream::Callbacks* cb) {
    boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
    mCallbackRegistration.push_back(StreamIDCallbackPair(sid,cb));
    return mSocketConnectionPhase;
}


Stream::StreamID MultiplexedSocket::getNewID() {
    if (!mFreeStreamIDs.probablyEmpty()) {
        Stream::StreamID retval;
        if (mFreeStreamIDs.pop(retval))
            return retval;
    }
    unsigned int retval=mHighestStreamID+=2;
    assert(retval>1);
    return Stream::StreamID(retval);
}
MultiplexedSocket::MultiplexedSocket(IOService*io, const Stream::SubstreamCallback&substreamCallback, bool zeroDelim)
 : SerializationCheck(),
   mIO(io),
   mNewSubstreamCallback(substreamCallback),
   mHighestStreamID(1)
{
    mZeroDelim=zeroDelim;
    mNewRequests=NULL;
    mSocketConnectionPhase=PRECONNECTION;
}
MultiplexedSocket::MultiplexedSocket(IOService*io,const UUID&uuid,const Stream::SubstreamCallback &substreamCallback, bool zeroDelimited)
 :SerializationCheck(),
  mIO(io),
     mNewSubstreamCallback(substreamCallback),
     mHighestStreamID(0) {
    mZeroDelim=zeroDelimited;
    mNewRequests=NULL;
    mSocketConnectionPhase=PRECONNECTION;
}

void MultiplexedSocket::initFromSockets(const std::vector<TCPSocket*>&sockets, size_t max_send_buffer_size) {
    for (unsigned int i=0;i<(unsigned int)sockets.size();++i) {
        mSockets.push_back(ASIOSocketWrapper(sockets[i],max_send_buffer_size,max_send_buffer_size>ASIO_SEND_BUFFER_SIZE?max_send_buffer_size:ASIO_SEND_BUFFER_SIZE,getSharedPtr()));
        mSockets.back().bindFunctions(getSharedPtr());
    }
}
void MultiplexedSocket::sendAllProtocolHeaders(const MultiplexedSocketPtr& thus, const std::string&origin, const std::string&host, const std::string&port, const std::string&resource_name, const std::string&subprotocol){
    unsigned int numSockets=(unsigned int)thus->mSockets.size();
    for (std::vector<ASIOSocketWrapper>::iterator i=thus->mSockets.begin(),ie=thus->mSockets.end();i!=ie;++i) {
        i->sendServerProtocolHeader(thus,origin,host,port,resource_name,subprotocol);
    }
    boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
    thus->mSocketConnectionPhase=CONNECTED;
    for (unsigned int i=0,ie=thus->mSockets.size();i!=ie;++i) {
        MakeASIOReadBuffer(thus,i,MemoryReference(NULL,0));
    }
    assert (thus->mNewRequests==NULL||thus->mNewRequests->probablyEmpty());//would otherwise need to empty out new requests--but no one should have a reference to us here
}
///erase all sockets and callbacks since the refcount is now zero;
MultiplexedSocket::~MultiplexedSocket() {
    Stream::SubstreamCallback callbackToBeDeleted=mNewSubstreamCallback;
    mNewSubstreamCallback=&Stream::ignoreSubstreamCallback;
    TCPSetCallbacks setCallbackFunctor(this,NULL);
    callbackToBeDeleted(NULL,setCallbackFunctor);
    for (unsigned int i=0;i<(unsigned int)mSockets.size();++i){
        mSockets[i].shutdownAndClose();
    }
    boost::lock_guard<boost::mutex> connecting_mutex(sConnectingMutex);
    for (unsigned int i=0;i<(unsigned int)mSockets.size();++i){
        mSockets[i].destroySocket();
    }
    mSockets.clear();

    while (!mCallbackRegistration.empty()){
        delete mCallbackRegistration.front().mCallback;
        mCallbackRegistration.pop_front();
    }
    if (mNewRequests) {
        std::deque<RawRequest> newRequests;
        mNewRequests->popAll(&newRequests);
        for (std::deque<RawRequest>::iterator i=newRequests.begin(),ie=newRequests.end();i!=ie;++i) {
            delete i->data;
        }
        delete mNewRequests;
        mNewRequests=NULL;
    }
    while(!mCallbacks.empty()) {
        delete mCallbacks.begin()->second;
        mCallbacks.erase(mCallbacks.begin());
    }
}

void MultiplexedSocket::shutDownClosedStream(unsigned int controlCode,const Stream::StreamID &id) {
    if (controlCode==TCPStream::TCPStreamCloseStream){
        std::deque<StreamIDCallbackPair> registrations;
        CommitCallbacks(registrations,CONNECTED,false);
        CallbackMap::iterator where=mCallbacks.find(id);
        if (where!=mCallbacks.end()) {
            std::tr1::shared_ptr<AtomicValue<int> > sendStatus=where->second->mSendStatus.lock();
            if (sendStatus) {
                TCPStream::closeSendStatus(*sendStatus);
            }
            where->second->mConnectionCallback(Stream::Disconnected,"Remote Host Disconnected");

            CommitCallbacks(registrations,CONNECTED,false);//just in case stream committed new callbacks during callback
            where=mCallbacks.find(id);
			if (where!=mCallbacks.end()) {//may have gotten erased in CommitCallback by a concurrent close request
                delete where->second;
                mCallbacks.erase(where);
			}
        }
    }
    std::tr1::unordered_set<Stream::StreamID>::iterator where=mOneSidedClosingStreams.find(id);
    if (where!=mOneSidedClosingStreams.end()) {
        mOneSidedClosingStreams.erase(where);
    }
    if (id.odd()==((mHighestStreamID.read()&1)?true:false)) {
        mFreeStreamIDs.push(id);
    }
}
void MultiplexedSocket::receiveFullChunk(unsigned int whichSocket, Stream::StreamID id, Chunk&newChunk, const Stream::PauseReceiveCallback& pauseReceive){
    if (id==Stream::StreamID()) {//control packet
        if(newChunk.size()) {
            unsigned int controlCode=*newChunk.begin();
            switch (controlCode) {
              case TCPStream::TCPStreamCloseStream:
              case TCPStream::TCPStreamAckCloseStream:
                if (newChunk.size()>1) {
                    unsigned int avail_len=newChunk.size()-1;
                    id.unserialize((const uint8*)&(newChunk[1]),avail_len);
                    if (avail_len+1>newChunk.size()) {
                        SILOG(tcpsst,warning,"Control Chunk too short");
                    }
                }
                if (id!=Stream::StreamID()) {
                    std::tr1::unordered_map<Stream::StreamID,unsigned int>::iterator where=mAckedClosingStreams.find(id);
                    if (where!=mAckedClosingStreams.end()){
                        where->second++;
                        int how_much=where->second;
                        if (where->second==mSockets.size()) {
                            mAckedClosingStreams.erase(where);
                            shutDownClosedStream(controlCode,id);
                            if (controlCode==TCPStream::TCPStreamCloseStream) {
                                closeStream(getSharedPtr(),id,TCPStream::TCPStreamAckCloseStream);
                            }
                        }
                    }else{
                        if (mSockets.size()==1) {
                            shutDownClosedStream(controlCode,id);
                            if (controlCode==TCPStream::TCPStreamCloseStream) {
                                closeStream(getSharedPtr(),id,TCPStream::TCPStreamAckCloseStream);
                            }
                        }else {
                            mAckedClosingStreams[id]=1;
                        }
                    }
                }
                break;
              default:
                break;
            }
        }
    }else {
        std::deque<StreamIDCallbackPair> registrations;
        CommitCallbacks(registrations,CONNECTED,false);
        CallbackMap::iterator where=mCallbacks.find(id);
        if (where!=mCallbacks.end()) {
            where->second->mBytesReceivedCallback(newChunk, pauseReceive);
        }else if (mOneSidedClosingStreams.find(id)==mOneSidedClosingStreams.end()) {
            //new substream
            TCPStream*newStream=new TCPStream(getSharedPtr(),id);
            TCPSetCallbacks setCallbackFunctor(this,newStream);
            mNewSubstreamCallback(newStream,setCallbackFunctor);
            if (setCallbackFunctor.mCallbacks != NULL) {
                CommitCallbacks(registrations,CONNECTED,false);//make sure bytes are received
                setCallbackFunctor.mCallbacks->mBytesReceivedCallback(newChunk, pauseReceive);
            }else {
                closeStream(getSharedPtr(),id);
            }
        }else {
            //IGNORED MESSAGE
        }
    }
}
void MultiplexedSocket::connectionFailureOrSuccessCallback(SocketConnectionPhase status, Stream::ConnectionStatus reportedProblem, const std::string&errorMessage) {
    Stream::ConnectionStatus stat=reportedProblem;
    std::deque<StreamIDCallbackPair> registrations;
    bool actuallyDoSend=CommitCallbacks(registrations,status,true);
    if (actuallyDoSend) {
        for (CallbackMap::iterator i=mCallbacks.begin(),ie=mCallbacks.end();i!=ie;++i) {
            i->second->mConnectionCallback(stat,errorMessage);
            if (reportedProblem!=Stream::Connected) {
                i->second->mConnectionCallback=&Stream::ignoreConnectionCallback;
            }

        }
    }else {
        //SILOG(tcpsst,debug,"Did not call callbacks because callback message already sent for "<<errorMessage);
    }
}
void MultiplexedSocket::connectionFailedCallback(const std::string& error) {

    connectionFailureOrSuccessCallback(DISCONNECTED,Stream::ConnectionFailed,error);
}
void MultiplexedSocket::connectionFailedCallback(unsigned int whichSocket, const std::string& error) {
    connectionFailedCallback(error);
    //FIXME do something with the socket specifically that failed.
}

void MultiplexedSocket::hostDisconnectedCallback(const std::string& error) {

    connectionFailureOrSuccessCallback(DISCONNECTED,Stream::Disconnected,error);
}
void MultiplexedSocket::hostDisconnectedCallback(unsigned int whichSocket, const std::string& error) {
    hostDisconnectedCallback(error);
    //FIXME do something with the socket specifically that failed.
}


void MultiplexedSocket::prepareConnect(unsigned int numSockets, size_t max_enqueued_send_size, bool noDelay, unsigned int kernelSendBufferSize, unsigned int kernelReceiveBufferSize) {
    mSocketConnectionPhase=PRECONNECTION;
    unsigned int oldSize=(unsigned int)mSockets.size();
    if (numSockets>mSockets.size()) {
        for (unsigned int i=oldSize;i<numSockets;++i) {
            mSockets.push_back(ASIOSocketWrapper(max_enqueued_send_size, max_enqueued_send_size>ASIO_SEND_BUFFER_SIZE?max_enqueued_send_size:ASIO_SEND_BUFFER_SIZE,getSharedPtr()));
            mSockets.back().bindFunctions(getSharedPtr());
            mSockets.back().createSocket(getASIOService(), kernelSendBufferSize, kernelReceiveBufferSize);
        }
    }
}
void MultiplexedSocket::connect(const Address&address, unsigned int numSockets, size_t max_enqueued_send_size, bool noDelay, unsigned int kernelSendBufferSize, unsigned int kernelReceiveBufferSize) {
    prepareConnect(numSockets,max_enqueued_send_size,noDelay,kernelSendBufferSize,kernelReceiveBufferSize);
    ASIOConnectAndHandshakePtr
        headerCheck(new ASIOConnectAndHandshake(getSharedPtr(),
                                                UUID::random()));
    //will notify connectionFailureOrSuccessCallback when resolved
    ASIOConnectAndHandshake::connect(headerCheck,address,noDelay);
}

void MultiplexedSocket::ioReactorThreadResumeRead(const MultiplexedSocketWPtr& weak_thus, Stream::StreamID sid){
    MultiplexedSocketPtr thus(weak_thus.lock());
    if (thus) {
        SerializationCheck::Scoped ss(thus.get());
        for (std::vector<ASIOSocketWrapper>::iterator i=thus->mSockets.begin(),ie=thus->mSockets.end();i!=ie;++i) {
            if (i->getReadBuffer()) {
                i->getReadBuffer()->ioReactorThreadResumeRead(thus);
            }
        }
    }
}
void MultiplexedSocket::ioReactorThreadPauseSend(const MultiplexedSocketWPtr& weak_thus, Stream::StreamID sid) {
    MultiplexedSocketPtr thus(weak_thus.lock());
    if (thus) {
        static Stream::StreamID::Hasher hasher;
        SerializationCheck::Scoped ss(thus.get());
        size_t whichStream=hasher(sid)%thus->mSockets.size();
        thus->mSockets[whichStream].ioReactorThreadPauseStream(thus, sid);
    }
}
Address MultiplexedSocket::getRemoteEndpoint(Stream::StreamID originStream)const {
    if (mSocketConnectionPhase==CONNECTED) {

        size_t whichStream=Stream::StreamID::Hasher()(originStream)%mSockets.size();
        return mSockets[whichStream].getRemoteEndpoint();
    }else return Address::null();
}
Address MultiplexedSocket::getLocalEndpoint(Stream::StreamID originStream)const {
    if (mSocketConnectionPhase==CONNECTED) {
        size_t whichStream=Stream::StreamID::Hasher()(originStream)%mSockets.size();
        return mSockets[whichStream].getLocalEndpoint();

    }else return Address::null();
}

Duration MultiplexedSocket::averageSendLatency() const {
    Duration avg(Duration::zero());

    uint32 nsockets = (uint32)mSockets.size();
    for(uint32 ii = 0; ii < nsockets; ++ii) {
        avg += mSockets[ii].averageSendLatency();
    }

    return avg / (float)nsockets;
}

Duration MultiplexedSocket::averageReceiveLatency() const {
    Duration avg(Duration::zero());

    uint32 nsockets = (uint32)mSockets.size();
    for(uint32 ii = 0; ii < nsockets; ++ii) {
        avg += mSockets[ii].averageReceiveLatency();
    }

    return avg / (float)nsockets;
}

} // namespace Network
} // namespace Sirikata
