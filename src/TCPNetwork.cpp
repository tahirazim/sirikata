#include "TCPNetwork.hpp"
#include "sirikata/network/IOServiceFactory.hpp"
#include "sirikata/network/IOService.hpp"
#include "sirikata/network/StreamFactory.hpp"
#include "sirikata/network/StreamListenerFactory.hpp"
#include "sirikata/network/StreamListener.hpp"
#include "Options.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"

using namespace Sirikata::Network;
using namespace Sirikata;

#define TCPNET_LOG(level,msg) SILOG(tcpnetwork,level,"[TCPNET] " << msg)

namespace CBR {

TCPNetwork::RemoteStream::RemoteStream(TCPNetwork* parent, Sirikata::Network::Stream*strm, ServerID remote_id, Address4 remote_net, Initiator init)
        : stream(strm),
          network_endpoint(remote_net),
          logical_endpoint(remote_id),
          initiator(init),
          connected(false),
          shutting_down(false),
          receive_queue( CountResourceMonitor(16) ),
          paused(false)
{
}

TCPNetwork::RemoteStream::~RemoteStream() {
    delete stream;
}

bool TCPNetwork::RemoteStream::push(Chunk& data) {
    boost::lock_guard<boost::mutex> lck(mPushPopMutex);

    Chunk* tmp = new Chunk;
    tmp->swap(data);
    bool pushed = receive_queue.push(tmp, false);

    if (!pushed) {
        // Space is occupied, pause and ignore
        TCPNET_LOG(insane,"Pausing receive from " << logical_endpoint << ".");
        paused = true;
        data.swap(*tmp); // Put the data back
        delete tmp;
        return false;
    }
    else {
        // Otherwise, give it its own chunk and push it up
        TCPNET_LOG(insane,"Passing data up to next layer from " << logical_endpoint << ".");
        return true;
    }
}

Chunk* TCPNetwork::RemoteStream::pop(IOService* ios) {
    boost::lock_guard<boost::mutex> lck(mPushPopMutex);
    // NOTE: the ordering in this method is very important since calls to push()
    // and pop() are possibly concurrent.
    // 1. We can always just unset paused since we're going to unpause. The
    // ordering of unsetting pause isn't critical since we won't be getting
    // another call to push at this point if paused was set to true (since
    // reading will have been paused).
    // 2. However, we need to call readyRead() *after* setting front to NULL so
    // that a resulting call to push() will actually be able to push. Otherwise
    // the call to push might fail, but another pop would not occur in order to
    // generate another readyRead call.  Since readyRead is the last call, we
    // are guaranteed another pop will be possible and we're back to a safe
    // state.

    bool was_paused = paused;

    Chunk* result = NULL;
    bool popped = receive_queue.pop(result);

    paused = false; // we can always unset pause
    if (was_paused) {
        // Note: we're posting this to the network strand because
        // bytesReceivedCallback may not have returned yet, causing
        // pausing to appear to have occurred even though it may not
        // have gone through yet.  Posting guarantees that the
        // readyRead() will be processed after the stream has actually
        // been paused.
        ios->post(
            std::tr1::bind(&Sirikata::Network::Stream::readyRead, stream)
        );
    }
    return result;
}


TCPNetwork::RemoteSession::RemoteSession(ServerID sid)
 : logical_endpoint(sid)
{
}

TCPNetwork::RemoteSession::~RemoteSession() {
    remote_stream.reset();
    closing_stream.reset();
    pending_out.reset();
}



TCPNetwork::TCPSendStream::TCPSendStream(ServerID sid, RemoteSessionPtr s)
 : logical_endpoint(sid),
   session(s)
{
}

TCPNetwork::TCPSendStream::~TCPSendStream() {
    session.reset();
}

ServerID TCPNetwork::TCPSendStream::id() const {
    return logical_endpoint;
}

bool TCPNetwork::TCPSendStream::send(const Chunk& data) {
    if (!session)
        return false;

    RemoteStreamPtr remote_stream = session->remote_stream;
    if (!remote_stream)
        return false;

    bool success = (
        remote_stream->connected &&
        !remote_stream->shutting_down &&
        remote_stream->stream->send(data, ReliableOrdered));

    if (!success)
        remote_stream->stream->requestReadySendCallback();

    return success;
}


TCPNetwork::TCPReceiveStream::TCPReceiveStream(ServerID sid, RemoteSessionPtr s, IOService* _ios)
 : logical_endpoint(sid),
   session(s),
   front_stream(),
   front_elem(NULL),
   ios(_ios)
{
}

TCPNetwork::TCPReceiveStream::~TCPReceiveStream()
{
    session.reset();
    front_stream.reset();
}

ServerID TCPNetwork::TCPReceiveStream::id() const {
    return logical_endpoint;
}

Chunk* TCPNetwork::TCPReceiveStream::front() {
    if (!session)
        return NULL;

    if (front_elem != NULL)
        return front_elem;

    // Need to get a new front_elem
    RemoteStreamPtr stream(getCurrentRemoteStream());
    if (!stream)
        return NULL;

    Chunk* result = stream->pop(ios);
    front_elem = result;
    return result;
}

Chunk* TCPNetwork::TCPReceiveStream::pop() {
    // Use front() to get the next one.
    Chunk* result = front();

    // If front fails we can bail out now
    if (result == NULL)
        return NULL;

    // Otherwise, just do cleanup before returning the front item
    RemoteStreamPtr stream(front_stream);
    assert(stream);
    assert(result == front_elem);
    // We've already popped in front, we just clear out the front element and
    // front queue
    front_elem = NULL;
    front_stream.reset();

    return result;
}

bool TCPNetwork::TCPReceiveStream::canReadFrom(RemoteStreamPtr strm) {
    return (
        strm &&
        ( (front_elem != NULL && strm == front_stream) || !strm->receive_queue.probablyEmpty()) &&
        (strm->connected || strm->shutting_down)
    );
}

TCPNetwork::RemoteStreamPtr TCPNetwork::TCPReceiveStream::getCurrentRemoteStream() {
    // Consider
    //  1) an already marked front stream
    //  2) closing streams
    //  3) active streams.
    // In order to return a stream it must
    //  a) exist
    //  b) have data in its receive queue
    //  c) be either connected or shutting_down

    if (!session)
        return RemoteStreamPtr();

    // At least for closing streams, results may no longer be valid, need to
    // double check.
    if (canReadFrom(front_stream))
        return front_stream;
    else// It got invalidated, clean out and continue search
        front_stream.reset();

    RemoteStreamPtr closing_stream(session->closing_stream);
    if (canReadFrom(closing_stream)) {
        front_stream = closing_stream;
        return closing_stream;
    }

    RemoteStreamPtr remote_stream(session->remote_stream);
    if (canReadFrom(remote_stream)) {
        front_stream = remote_stream;
        return remote_stream;
    }

    return RemoteStreamPtr();
}





TCPNetwork::TCPNetwork(SpaceContext* ctx)
 : Network(ctx),
   mSendListener(NULL),
   mReceiveListener(NULL)
{
    mStreamPlugin = GetOption("spacestreamlib")->as<String>();
    mPluginManager.load(Sirikata::DynamicLibrary::filename(mStreamPlugin));

    mListenOptions = StreamListenerFactory::getSingleton().getOptionParser(mStreamPlugin)(GetOption("spacestreamoptions")->as<String>());
    mSendOptions = StreamFactory::getSingleton().getOptionParser(mStreamPlugin)(GetOption("spacestreamoptions")->as<String>());

    mIOService = IOServiceFactory::makeIOService();
    mIOWork = new IOWork(mIOService, "TCPNetwork Work");
    mThread = new Thread(std::tr1::bind(&IOService::run,mIOService));

    mListener = StreamListenerFactory::getSingleton().getConstructor(mStreamPlugin)(mIOService,mListenOptions);
}

TCPNetwork::~TCPNetwork() {
    delete mListener;

    // Cancel close timers
    for(TimerSet::iterator it = mClosingStreamTimers.begin(); it != mClosingStreamTimers.end(); it++) {
        Sirikata::Network::IOTimerPtr timer = *it;
        timer->cancel();
    }
    mClosingStreamTimers.clear();

    // Clear out all the remote sessions.
    mRemoteData.clear();


    delete mIOWork;
    mIOWork = NULL;

    mIOService->stop(); // FIXME forceful shutdown...

    mThread->join();
    delete mThread;

    IOServiceFactory::destroyIOService(mIOService);
    mIOService = NULL;
}


TCPNetwork::RemoteData* TCPNetwork::getRemoteData(ServerID sid) {
    boost::lock_guard<boost::recursive_mutex> lck(mRemoteDataMutex);

    RemoteDataMap::iterator it = mRemoteData.find(sid);
    if (it != mRemoteData.end())
        return it->second;

    RemoteSessionPtr remote_session(new RemoteSession(sid));
    RemoteData* result = new RemoteData( remote_session );
    mRemoteData[sid] = result;
    return result;
}

TCPNetwork::RemoteSessionPtr TCPNetwork::getRemoteSession(ServerID sid) {
    boost::lock_guard<boost::recursive_mutex> lck(mRemoteDataMutex);
    TCPNetwork::RemoteData* data = getRemoteData(sid);
    return data->session;
}

TCPNetwork::TCPSendStream* TCPNetwork::getNewSendStream(ServerID sid) {
    TCPSendStream* result = NULL;
    bool notify = false;
    {
        boost::lock_guard<boost::recursive_mutex> lck(mRemoteDataMutex);

        TCPNetwork::RemoteData* data = getRemoteData(sid);
        if (data->send == NULL) {
            notify = true;
            data->send = new TCPSendStream(sid, data->session);
        }
        result = data->send;
    }
    if (notify) {
        // And notify listeners.  We *must* do this here because the initial
        // send may have failed due to not having a connection so the normal
        // readySend callback will not occur since we couldn't register for
        // it yet.
        mSendListener->networkReadyToSend(sid);
    }
    return result;
}

TCPNetwork::TCPReceiveStream* TCPNetwork::getNewReceiveStream(ServerID sid) {
    TCPReceiveStream* result = NULL;
    bool notify = false;
    {
        boost::lock_guard<boost::recursive_mutex> lck(mRemoteDataMutex);

        TCPNetwork::RemoteData* data = getRemoteData(sid);
        if (data->receive == NULL) {
            notify = true;
            data->receive = new TCPReceiveStream(sid, data->session, mIOService);
        }
        result = data->receive;
    }
    if (notify) {
        // Notifications only happen here so we don't notify them twice about
        // what should look like the same connection to them.
        mReceiveListener->networkReceivedConnection(result);
    }
    return result;
}

TCPNetwork::RemoteStreamPtr TCPNetwork::getNewOutgoingStream(ServerID sid, Address4 remote_net, RemoteStream::Initiator init) {
    boost::lock_guard<boost::recursive_mutex> lck(mRemoteDataMutex);

    RemoteSessionPtr session = getRemoteSession(sid);
    RemoteStreamPtr pending = session->pending_out;
    if (pending != RemoteStreamPtr())
        return RemoteStreamPtr();

    Sirikata::Network::Stream* strm = StreamFactory::getSingleton().getConstructor(mStreamPlugin)(mIOService,mSendOptions);

    pending = RemoteStreamPtr(
        new RemoteStream(
            this,
            strm,
            sid,
            remote_net,
            init
        )
    );
    session->pending_out = pending;
    return pending;
}

TCPNetwork::RemoteStreamPtr TCPNetwork::getNewIncomingStream(Address4 remote_net, RemoteStream::Initiator init, Sirikata::Network::Stream* strm) {
    assert(strm != NULL);

    RemoteStreamPtr pending(
        new RemoteStream(
            this,
            strm,
            NullServerID,
            remote_net,
            init
        )
    );
    return pending;
}




// IO Thread Methods

void TCPNetwork::newStreamCallback(Sirikata::Network::Stream* newStream,
				   Sirikata::Network::Stream::SetCallbacks& setCallbacks)
{
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    if (newStream == NULL) {
        // Indicates underlying connection is being deleted.  Annoying, but we
        // just ignore it.
        return;
    }

    TCPNET_LOG(info,"New stream accepted.");
    // A new stream was initiated by a remote endpoint.  We need to
    // set up our basic storage and then wait for the specification of
    // the remote endpoint ID before allowing use of the connection

    Address4 source_address(newStream->getRemoteEndpoint());

    RemoteNetStreamMap::iterator it = mPendingStreams.find(source_address);
    if (it != mPendingStreams.end()) {
        TCPNET_LOG(info,"Immediately closing incoming stream, pending connection already present.");
        delete newStream;
        return;
    }

    RemoteStreamPtr remote_stream( getNewIncomingStream(source_address, RemoteStream::Them, newStream) );
    assert(remote_stream);
    RemoteStreamWPtr weak_remote_stream( remote_stream );

    IndirectTCPReceiveStream ind_recv_stream( new TCPReceiveStream*(NULL) );

    setCallbacks(
        std::tr1::bind(&TCPNetwork::connectionCallback, this, weak_remote_stream, _1, _2),
        std::tr1::bind(&TCPNetwork::bytesReceivedCallback, this, weak_remote_stream, ind_recv_stream, _1),
        std::tr1::bind(&TCPNetwork::readySendCallback, this, weak_remote_stream)
    );

    mPendingStreams[source_address] = remote_stream;
}

void TCPNetwork::connectionCallback(RemoteStreamWPtr wstream, const Sirikata::Network::Stream::ConnectionStatus status, const std::string& reason) {
    RemoteStreamPtr remote_stream(wstream);

    if (!remote_stream)
        TCPNET_LOG(warning,"Got connection callback on stream that is no longer referenced.");

    if (status == Sirikata::Network::Stream::Disconnected ||
        status == Sirikata::Network::Stream::ConnectionFailed) {
        TCPNET_LOG(info,"Got disconnection event from " << remote_stream->logical_endpoint);

        remote_stream->connected = false;
        remote_stream->shutting_down = true;
        handleDisconnectedStream(remote_stream);
    }
    else if (status == Sirikata::Network::Stream::Connected) {
        // Note: We should only get Connected for connections we initiated.
        TCPNET_LOG(info,"Sending intro and marking stream as connected to " << remote_stream->logical_endpoint);
        // The first message needs to be our introduction
        sendServerIntro(remote_stream);
        // And after we're sure its sent, open things up for everybody else
        remote_stream->connected = true;
        // Add to list of connections and notify listeners of connection
        handleConnectedStream(remote_stream);
    }
    else {
        TCPNET_LOG(error,"Unhandled send stream connection status: " << status << " -- " << reason);
    }
}

Sirikata::Network::Stream::ReceivedResponse TCPNetwork::bytesReceivedCallback(RemoteStreamWPtr wstream, IndirectTCPReceiveStream ind_recv_strm, Chunk&data) {
    RemoteStreamPtr remote_stream(wstream);

    // If we've lost all references to the RemoteStream just ignore
    if (!remote_stream) {
        TCPNET_LOG(error,"Ignoring received data; invalid weak reference to remote stream.");
        return Sirikata::Network::Stream::AcceptedData;
    }

    // If the stream hasn't been marked as connected, this *should* be
    // the initial header
    if (remote_stream->connected == false) {
        assert( *ind_recv_strm == NULL );
        // The stream might already be closing.  If so, just ignore this data.
        if (remote_stream->shutting_down)
            return Sirikata::Network::Stream::AcceptedData;

        // Remove from the list of pending connections
        TCPNET_LOG(info,"Parsing endpoint information for incomplete remote-initiated remote stream.");
        Address4 source_addr = remote_stream->network_endpoint;
        RemoteNetStreamMap::iterator it = mPendingStreams.find(source_addr);
        if (it == mPendingStreams.end()) {
            TCPNET_LOG(error,"Address for connection not found in pending receive buffers"); // FIXME print addr
        }
        else {
            mPendingStreams.erase(it);
        }

        // Parse the header indicating the remote ID
        CBR::Protocol::Server::ServerIntro intro;
        bool parsed = parsePBJMessage(&intro, data);
        assert(parsed);

        TCPNET_LOG(info,"Parsed remote endpoint information from " << intro.id());
        remote_stream->logical_endpoint = intro.id();
        remote_stream->connected = true;
        TCPReceiveStream* recv_strm = handleConnectedStream(remote_stream);
        *ind_recv_strm = recv_strm;
    }
    else {
        TCPNET_LOG(insane,"Handling regular received data from " << remote_stream->logical_endpoint << ".");

        TCPReceiveStream* recv_strm = *ind_recv_strm;
        assert( recv_strm != NULL );

        // Normal case, we can just handle the message
        bool pushed_success = remote_stream->push(data);
        if (!pushed_success)
            return Sirikata::Network::Stream::PauseReceive;
        else
            mReceiveListener->networkReceivedData( recv_strm );
    }

    return Sirikata::Network::Stream::AcceptedData;
}

void TCPNetwork::readySendCallback(RemoteStreamWPtr wstream) {
    RemoteStreamPtr remote_stream(wstream);
    if (!remote_stream)
        return;

    assert(remote_stream->logical_endpoint != NullServerID);

    mSendListener->networkReadyToSend(remote_stream->logical_endpoint);
}

// Main Thread/Strand Methods

namespace {

char toHex(unsigned char u) {
    if (u<=9) return '0'+u;
    return 'A'+(u-10);
}

void hexPrint(const char *name, const Chunk&data) {
    std::string str;
    str.resize(data.size()*2);
    for (size_t i=0;i<data.size();++i) {
        str[i*2]=toHex(data[i]%16);
        str[i*2+1]=toHex(data[i]/16);
    }
    std::cout<< name<<' '<<str<<'\n';
}

} // namespace

void TCPNetwork::sendServerIntro(const RemoteStreamPtr& out_stream) {
    CBR::Protocol::Server::ServerIntro intro;
    intro.set_id(mContext->id());
    std::string serializedIntro;
    serializePBJMessage(&serializedIntro, intro);

    out_stream->stream->send(
        MemoryReference(serializedIntro),
        ReliableOrdered
    );
}


// Connection Strand

TCPNetwork::TCPSendStream* TCPNetwork::openConnection(const ServerID& dest) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    Address4* addr = mServerIDMap->lookupInternal(dest);
    assert(addr != NULL);

    TCPNET_LOG(info,"Initiating new connection to " << dest);
    TCPSendStream* send_stream = getNewSendStream(dest);

    RemoteStreamPtr remote = getNewOutgoingStream(dest, *addr, RemoteStream::Us);
    if (remote == RemoteStreamPtr()) {
        TCPNET_LOG(info,"Skipped connecting to " << dest << ", connection already present.");
        return send_stream;
    }

    RemoteStreamWPtr weak_remote(remote);

    TCPReceiveStream* recv_strm = getNewReceiveStream(dest);
    IndirectTCPReceiveStream ind_recv_stream( new TCPReceiveStream*(recv_strm) );

    Sirikata::Network::Stream::SubstreamCallback sscb(&Sirikata::Network::Stream::ignoreSubstreamCallback);
    Sirikata::Network::Stream::ConnectionCallback connCallback(std::tr1::bind(&TCPNetwork::connectionCallback, this, weak_remote, _1, _2));
    Sirikata::Network::Stream::ReceivedCallback br(std::tr1::bind(&TCPNetwork::bytesReceivedCallback, this, weak_remote, ind_recv_stream, _1));
    Sirikata::Network::Stream::ReadySendCallback readySendCallback(std::tr1::bind(&TCPNetwork::readySendCallback, this, weak_remote));
    remote->stream->connect(convertAddress4ToSirikata(*addr),
                            sscb,
                            connCallback,
                            br,
                            readySendCallback);

    // Note: Initial connection header is sent upon successful connection

    return send_stream;
}

TCPNetwork::TCPReceiveStream* TCPNetwork::handleConnectedStream(RemoteStreamPtr remote_stream) {
    ServerID remote_id = remote_stream->logical_endpoint;
    RemoteSessionPtr session = getRemoteSession(remote_id);

    if (!session->remote_stream || session->remote_stream == remote_stream) {
        session->remote_stream = remote_stream;
        if (session->pending_out == remote_stream)
            session->pending_out.reset();
    } else {
        // If there's already a stream in the map, this incoming stream conflicts
        // with one we started in the outgoing direction.  We need to cancel one and
        // save the other in our map.

        TCPNET_LOG(info,"Resolving multiple conflicting connections to " << remote_stream->logical_endpoint);

        RemoteStreamPtr new_remote_stream = remote_stream;
        RemoteStreamPtr existing_remote_stream = session->remote_stream;

        assert (new_remote_stream->initiator != existing_remote_stream->initiator);
        assert(new_remote_stream->logical_endpoint == existing_remote_stream->logical_endpoint);
        ServerID remote_ep = new_remote_stream->logical_endpoint;
        assert(remote_ep != mContext->id());

        // The choice of which connection to maintain is arbitrary,
        // but needs to be symmetric. If we have a smaller ID, prefer
        // the stream that we initiated. Otherwise, prefer the stream
        // that they initiated
        RemoteStreamPtr stream_to_save;
        RemoteStreamPtr stream_to_close;
        if ((mContext->id() < remote_ep && new_remote_stream->initiator == RemoteStream::Us) ||
            (mContext->id() > remote_ep && new_remote_stream->initiator == RemoteStream::Them)
        ) {
            stream_to_save = new_remote_stream;
            stream_to_close = existing_remote_stream;
        } else {
            stream_to_save = existing_remote_stream;
            stream_to_close = new_remote_stream;
        }

        // We should never have 2 closing streams at the same time or else the
        // remote end is misbehaving
        if (session->closing_stream) {
            TCPNET_LOG(fatal,"Multiple closing streams for single remote server, server " << stream_to_close->logical_endpoint);
        }

        session->closing_stream = stream_to_close;
        session->remote_stream = stream_to_save;
        stream_to_close->shutting_down = true;

        Sirikata::Network::IOTimerPtr timer = Sirikata::Network::IOTimer::create(mIOService);
        timer->wait(
            Duration::seconds(5),
            std::tr1::bind(&TCPNetwork::handleClosingStreamTimeout, this, timer, stream_to_close)
                   );

        mClosingStreamTimers.insert(timer);
    }

    // With everything setup, simply try to get the send and receive streams,
    // which will also cause any necessary notifications
    TCPReceiveStream* receive_stream = getNewReceiveStream(remote_id);
    TCPSendStream* send_stream = getNewSendStream(remote_id);

    // And poke the stream in both directions -- since we're now ready to send
    // and just in case the receive stream got paused
    mSendListener->networkReadyToSend(remote_id);
    remote_stream->stream->readyRead();
    return receive_stream;
}

void TCPNetwork::handleDisconnectedStream(const RemoteStreamPtr& closed_stream) {
    // When we get a disconnection signal, there's nothing we can even do
    // anymore -- the other side has closed the connection for some reason and
    // isn't going to accept any more data.  The connection has already been
    // marked as disconnected and shutting down, we just have to do the removal
    // from the main thread.

    RemoteSessionPtr session = getRemoteSession(closed_stream->logical_endpoint);

    assert(closed_stream &&
           !closed_stream->connected &&
           closed_stream->shutting_down);

    // If this matches either of our streams, remove them immediately
    if (session->remote_stream == closed_stream)
        session->remote_stream.reset();
    if (session->closing_stream == closed_stream)
        session->closing_stream.reset();
}

void TCPNetwork::handleClosingStreamTimeout(Sirikata::Network::IOTimerPtr timer, RemoteStreamPtr& stream) {
    mClosingStreamTimers.erase(timer);

    if (!stream) {
        TCPNET_LOG(error,"Got close stream timeout for stream that is no longer in closing stream set.");
        return;
    }

    TCPNET_LOG(info,"Closing stream due to timeout, server " << stream->logical_endpoint);

    stream->connected = false;
    stream->stream->close();
}



void TCPNetwork::setSendListener(SendListener* sl) {
    mSendListener = sl;
}

void TCPNetwork::listen(const ServerID& as_server, ReceiveListener* receive_listener) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    mReceiveListener = receive_listener;

    TCPNET_LOG(info,"Listening for remote space servers.");

    Address4* listen_addr = mServerIDMap->lookupInternal(as_server);
    assert(listen_addr != NULL);
    mListenAddress = *listen_addr;

    Address listenAddress(convertAddress4ToSirikata(mListenAddress));
    mListener->listen(
        Address(listenAddress.getHostName(),listenAddress.getService()),
        std::tr1::bind(&TCPNetwork::newStreamCallback,this,_1,_2)
                      );
}

Network::SendStream* TCPNetwork::connect(const ServerID& addr) {
    return openConnection(addr);
}

} // namespace Sirikata
