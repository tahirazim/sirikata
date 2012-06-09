/*  Sirikata Network Utilities
 *  TCPStreamListener.cpp
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
#include <sirikata/core/network/IOStrand.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/network/IOService.hpp>
#include "TCPStream.hpp"
#include "TCPStreamListener.hpp"
#include "ASIOStreamBuilder.hpp"
#include <sirikata/core/options/Options.hpp>

namespace Sirikata {
namespace Network {

using namespace boost::asio::ip;

TCPStreamListener::Data::Data(IOStrand* io,
                              uint8 maxSimultaneousSockets,
                              uint32 sendBufferSize,
                              bool noDelay,
                              uint32 kernelSendBufferSize,
                              uint32 kernelReceiveBufferSize)
 : strand(io),
   acceptor(NULL),
     socket(NULL),
     cb(0),
     mMaxSimultaneousSockets(maxSimultaneousSockets),
     mNoDelay(noDelay),
     mSendBufferSize(sendBufferSize),
     mKernelSendBufferSize(kernelSendBufferSize),
     mKernelReceiveBufferSize(kernelReceiveBufferSize)
{
}

TCPStreamListener::Data::~Data() {
    delete acceptor;
    delete socket;
}

// Start the listening process.
void TCPStreamListener::Data::start(DataPtr shared_this) {
    assert(shared_this.get() == this);
    strand->post(
        std::tr1::bind(&TCPStreamListener::Data::startAccept, shared_this),
        "TCPStreamListener::Data::startAccept"
        );
}

// All the real work happens here in these methods
void TCPStreamListener::Data::startAccept(DataPtr& data) {
    assert(data->socket == NULL);
    data->socket = new TCPSocket(data->strand->service());
    if (data->mKernelReceiveBufferSize) {
        boost::asio::socket_base::receive_buffer_size option(data->mKernelReceiveBufferSize);
        data->socket->set_option(option);
    }
    if (data->mKernelSendBufferSize) {
        boost::asio::socket_base::send_buffer_size optionS(data->mKernelSendBufferSize);
        data->socket->set_option(optionS);
    }

    data->acceptor->async_accept(
        *(data->socket),
        data->strand->wrap(std::tr1::bind(&TCPStreamListener::Data::handleAccept, data, _1))
    );
}

void TCPStreamListener::Data::handleAccept(DataPtr& data, const boost::system::error_code& error) {
    if (error) {
        if (error == boost::system::errc::operation_canceled) {
            SILOG(tcpsst, insane, "TCPStreamListener listening operation cancelled. Likely due to socket shutdown.");
        }
        else {
            boost::system::system_error se(error);
            SILOG(tcpsst, error, "Error listening for TCP stream:" << se.what() << std::endl);
        }
        //FIXME: attempt more?
        return;
    }

    TCPSocket* newSocket = data->socket;
    data->socket = NULL;

    // Hand off the new connection for sessions initiation
    ASIOStreamBuilder::beginNewStream(newSocket, data);

    // Continue listening
    startAccept(data);
}


TCPStreamListener::TCPStreamListener(IOStrand* io, OptionSet*options)
{
    OptionValue *maxSimultSockets=options->referenceOption("max-parallel-sockets");
    OptionValue *sendBufferSize=options->referenceOption("send-buffer-size");
    OptionValue *noDelay=options->referenceOption("no-delay");
    OptionValue *kernelSendBufferSize=options->referenceOption("ksend-buffer-size");
    OptionValue *kernelReceiveBufferSize=options->referenceOption("kreceive-buffer-size");
    OptionValue *fragmentPackets=options->referenceOption("test-fragment-packet-level");
    if (fragmentPackets->as<int>()!=-1) {
        TCPStream::sFragmentPackets = fragmentPackets->as<int>();
    }

    assert(maxSimultSockets&&sendBufferSize);
    DataPtr data (new Data(io,
                           (unsigned char)maxSimultSockets->as<unsigned int>(),
                           sendBufferSize->as<unsigned int>(),
                           noDelay->as<bool>(),
                           kernelSendBufferSize->as<unsigned int>(),
                           kernelReceiveBufferSize->as<unsigned int>()));
    mData=data;
}

TCPStreamListener::~TCPStreamListener() {
    close();
}

bool TCPStreamListener::listen (const Address&address,
                                const Stream::SubstreamCallback&newStreamCallback) {
    close();

    mData->acceptor = new TCPListener(mData->strand->service(),tcp::endpoint(tcp::v4(), atoi(address.getService().c_str())));
    mData->cb = newStreamCallback;
    mData->start(mData);

    return true;
}

String TCPStreamListener::listenAddressName() const {
    std::stringstream retval;
    retval << mData->acceptor->local_endpoint().address().to_string() << ':' << mData->acceptor->local_endpoint().port();
    return retval.str();
}

Address TCPStreamListener::listenAddress() const {
    std::stringstream port;
    port << mData->acceptor->local_endpoint().port();
    return Address(mData->acceptor->local_endpoint().address().to_string(),
                   port.str());
}

void TCPStreamListener::close(){
    if (mData->acceptor != NULL) {

		boost::system::error_code ec;
        mData->acceptor->cancel(ec);
		if (ec) {
			SILOG(tcpsst,error,"Error closing listener "<<ec);
		}
        mData->acceptor->close();

        delete mData->acceptor;
        mData->acceptor = NULL;
        mData->cb = 0;
    }
}

} // namespace Network
} // namespace Sirikata
