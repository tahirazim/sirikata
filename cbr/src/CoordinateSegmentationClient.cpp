/*  Sirikata
 *  CoordinateSegmentationClient.cpp
 *
 *  Copyright (c) 2009, Tahir Azim
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

#include "CoordinateSegmentationClient.hpp"
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>

#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <sirikata/cbrcore/Options.hpp>
#include <sirikata/cbrcore/Message.hpp>
#include <sirikata/cbrcore/ServerIDMap.hpp>

namespace Sirikata {

template<typename T>
T clamp(T val, T minval, T maxval) {
    if (val < minval) return minval;
    if (val > maxval) return maxval;
    return val;
}

void memdump1(uint8* buffer, int len) {
  for (int i=0; i<len; i++) {
    int val = buffer[i];
    printf("%x ",  val);
  }
  printf("\n");
  fflush(stdout);
}

using Sirikata::Network::TCPResolver;
using Sirikata::Network::TCPSocket;
using Sirikata::Network::TCPListener;

CoordinateSegmentationClient::CoordinateSegmentationClient(SpaceContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim,
							   ServerIDMap* sidmap)
                               : CoordinateSegmentation(ctx),  mRegion(region), mBSPTreeValid(false), mAvailableServersCount(0), mIOService(IOServiceFactory::makeIOService()), mSidMap(sidmap)
{
  mTopLevelRegion.mBoundingBox = BoundingBox3f( Vector3f(0,0,0), Vector3f(0,0,0));

  Address4* addy = mSidMap->lookupInternal(mContext->id());

  mAcceptor = boost::shared_ptr<TCPListener>(new TCPListener(*mIOService,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), addy->port+10000)));

  startAccepting();

  sendSegmentationListenMessage();
}

void CoordinateSegmentationClient::startAccepting() {
  mSocket = boost::shared_ptr<TCPSocket>(new TCPSocket(*mIOService));
  mAcceptor->async_accept(*mSocket, boost::bind(&CoordinateSegmentationClient::accept_handler,this));
}

void CoordinateSegmentationClient::accept_handler() {
  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  for (;;)
    {
      boost::array<uint8, 1024> buf;
      boost::system::error_code error;

      size_t len = mSocket->read_some(boost::asio::buffer(buf), error);

      if (dataReceived == NULL) {
	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;

      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error) {
	std::cout << "Error reading request from " << mSocket->remote_endpoint().address().to_string()
		    <<" in accept_handler\n";
        throw boost::system::system_error(error); // Some other error.
      }
    }

  if ( dataReceived != NULL) {
    SegmentationChangeMessage* segChangeMessage = (SegmentationChangeMessage*) dataReceived;

    std::cout << "\n\nReceived segmentation change message with numEntries=" << segChangeMessage->numEntries << " at time " << mContext->simTime().raw() << "\n";

    mTopLevelRegion.destroy();
    mServerRegionCache.clear();

    int offset = 2;
    std::vector<Listener::SegmentationInfo> segInfoVector;
    for (int i=0; i<segChangeMessage->numEntries; i++) {
      BoundingBoxList bbList;
      Listener::SegmentationInfo segInfo;
      SerializedSegmentChange* segChange = (SerializedSegmentChange*) (dataReceived+offset);

      for (unsigned int j=0; j < segChange->listLength; j++) {
	BoundingBox3f bbox;
	SerializedBBox sbbox= segChange->bboxList[j];
	sbbox.deserialize(bbox);
	bbList.push_back(bbox);
      }

      segInfo.server = segChange->serverID;
      segInfo.region = bbList;

      printf("segInfo.server=%d, segInfo.region.size=%d\n", segInfo.server, (int)segInfo.region.size());
      fflush(stdout);

      segInfoVector.push_back(segInfo);
      offset += (4 + 4 + sizeof(SerializedBBox)*segChange->listLength);
    }

    notifyListeners(segInfoVector);

    free(dataReceived);
  }

  mSocket->close();
  startAccepting();
}

CoordinateSegmentationClient::~CoordinateSegmentationClient() {

}

void CoordinateSegmentationClient::sendSegmentationListenMessage() {
  SegmentationListenMessage requestMessage;
  Address4* addy = mSidMap->lookupInternal(mContext->id());

  gethostname(requestMessage.host, 255);
  printf("host=%s\n", requestMessage.host);
  requestMessage.port = addy->port+10000;

  struct in_addr ip_addr;
  ip_addr.s_addr = addy->ip;
  char* addr = inet_ntoa(ip_addr);

  IOService* io_service = IOServiceFactory::makeIOService();

  TCPResolver resolver(*io_service);

  TCPResolver::query query(boost::asio::ip::tcp::v4(), GetOption("cseg-service-host")->as<String>(),
			               GetOption("cseg-service-tcp-port")->as<String>());

  TCPResolver::iterator endpoint_iterator = resolver.resolve(query);

  TCPResolver::iterator end;

  //std::cout << "Calling CSEG server for serverRegion!\n";
  TCPSocket socket(*io_service);
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
  if (error) {
    std::cout << "Error connecting to  CSEG server for segmentation listen subscription\n";
  }

  boost::asio::write(socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );

  socket.close();

  IOServiceFactory::destroyIOService(io_service);
}


ServerID CoordinateSegmentationClient::lookup(const Vector3f& pos)  {
  LookupRequestMessage lookupMessage;
  lookupMessage.x = pos.x;
  lookupMessage.y = pos.y;
  lookupMessage.z = pos.z;

  IOService* io_service = IOServiceFactory::makeIOService();

  TCPResolver resolver(*io_service);

  TCPResolver::query query(boost::asio::ip::tcp::v4(), GetOption("cseg-service-host")->as<String>(),
			               GetOption("cseg-service-tcp-port")->as<String>());

  TCPResolver::iterator endpoint_iterator = resolver.resolve(query);

  TCPResolver::iterator end;


  TCPSocket socket(*io_service);
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
  if (error) {
    std::cout << "Error connecting to  CSEG server for lookup\n";
    return 0;
  }

  boost::asio::write(socket,
                     boost::asio::buffer((const void*)&lookupMessage,sizeof (lookupMessage)),
                     boost::asio::transfer_all() );



  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  bool failedOnce=false;
  for (;;)
  {
      boost::system::error_code error;
      boost::array<uint8, 1048576> buf;

      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (dataReceived == NULL) {
	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error) {
	std::cout << "Error reading response from " << socket.remote_endpoint().address().to_string()
		  <<" in lookup\n\n\n";
      }
  }

  LookupResponseMessage* response = (LookupResponseMessage*)dataReceived;
  assert(response->type == LOOKUP_RESPONSE);

  ServerID retval = response->serverID;

  if (dataReceived != NULL) {
    free(dataReceived);
  }

  IOServiceFactory::destroyIOService(io_service);

  return retval;
}

BoundingBoxList CoordinateSegmentationClient::serverRegion(const ServerID& server)
{
  BoundingBoxList boundingBoxList;

  if (mServerRegionCache.find(server) != mServerRegionCache.end()) {
    //printf("Returning cached serverRegion\n");
    return mServerRegionCache[server];
  }

  //printf("Going to server for serverRegion for svr_id=%d\n", server);

  ServerRegionRequestMessage requestMessage;
  requestMessage.serverID = server;

  IOService* io_service = IOServiceFactory::makeIOService();

  TCPResolver resolver(*io_service);

  TCPResolver::query query(boost::asio::ip::tcp::v4(), GetOption("cseg-service-host")->as<String>(),
			               GetOption("cseg-service-tcp-port")->as<String>());

  TCPResolver::iterator endpoint_iterator = resolver.resolve(query);

  TCPResolver::iterator end;

  //std::cout << "Calling CSEG server for serverRegion!\n";
  TCPSocket socket(*io_service);
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
  if (error) {
    //std::cout << "Error connecting to  CSEG server for server region\n";
    return boundingBoxList;
  }

  boost::asio::write(socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );

  //std::cout << "Sent over serverRegion request to cseg server\n";


  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  for (;;)
  {
      boost::system::error_code error;
      boost::array<uint8, 1048576> buf;

      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (dataReceived == NULL) {
	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error) {
	std::cout << "Error reading response from " << socket.remote_endpoint().address().to_string() <<" in serverRegion\n";
       //throw boost::system::system_error(error); // Some other error.

      }
  }

  if (dataReceived == NULL) {
    boundingBoxList.push_back(BoundingBox3f(Vector3f(0,0,0), Vector3f(0,0,0)));
    return boundingBoxList;
  }

  //std::cout << "Received reply from cseg server for server region"  << "\n";

  ServerRegionResponseMessage* response = (ServerRegionResponseMessage*) malloc(bytesReceived);
  memcpy(response, dataReceived, bytesReceived);

  assert(response->type == SERVER_REGION_RESPONSE);

  for (uint32 i=0; i < response->listLength; i++) {
    BoundingBox3f bbox;
    SerializedBBox* sbbox = &(response->bboxList[i]);
    sbbox->deserialize(bbox);

    boundingBoxList.push_back(bbox);
  }

  free(dataReceived);
  free(response);
  //printf("serverRegion for %d returned %d boxes\n", server, boundingBoxList.size());
  /*for (int i=0 ; i<boundingBoxList.size(); i++) {
    //std::cout << server << " has bounding box : " << boundingBoxList[i] << "\n";
  }*/

  //memdump1((uint8*) response, bytesReceived);

  mServerRegionCache[server] = boundingBoxList;

  IOServiceFactory::destroyIOService(io_service);

  return boundingBoxList;
}

BoundingBox3f CoordinateSegmentationClient::region()  {
  //std::cout << "mTopLevelRegion.mBoundingBox= " << mTopLevelRegion.mBoundingBox << "\n";
  if ( mTopLevelRegion.mBoundingBox.min().x  != mTopLevelRegion.mBoundingBox.max().x ) {
    //printf("Returning cached region\n");
    return mTopLevelRegion.mBoundingBox;
  }

  //printf("Going to server for region\n");

  RegionRequestMessage requestMessage;

  IOService* io_service = IOServiceFactory::makeIOService();

  TCPResolver resolver(*io_service);

  TCPResolver::query query(boost::asio::ip::tcp::v4(), GetOption("cseg-service-host")->as<String>(),
			               GetOption("cseg-service-tcp-port")->as<String>());

  TCPResolver::iterator endpoint_iterator = resolver.resolve(query);

  TCPResolver::iterator end;

  //std::cout << "Calling CSEG server for region!\n";
  TCPSocket socket(*io_service);
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
  if (error) {
    //std::cout << "Error connecting to  CSEG server for region\n";
    return BoundingBox3f();
  }

  boost::asio::write(socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );

  //std::cout << "Sent over region request to cseg server\n";


  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  for (;;)
  {
      boost::system::error_code error;
      boost::array<uint8, 1048576> buf;

      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (dataReceived == NULL) {
	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error) {
	//std::cout << "Error reading response from " << socket.remote_endpoint().address().to_string()<<" in region\n";

        //throw boost::system::system_error(error); // Some other error.
	return mTopLevelRegion.mBoundingBox;
      }
  }

  //std::cout << "Received reply from cseg server for region"  << "\n";

  if (dataReceived == NULL) {
    return mTopLevelRegion.mBoundingBox;
  }

  RegionResponseMessage* response = (RegionResponseMessage*) malloc(bytesReceived);
  memcpy(response, dataReceived, bytesReceived);
  assert(response->type == REGION_RESPONSE);


  BoundingBox3f bbox;
  SerializedBBox* sbbox = &response->bbox;
  sbbox->deserialize(bbox);

  free(dataReceived);
  free(response);

  //std::cout << "Region returned " <<  bbox << "\n";

  mTopLevelRegion.mBoundingBox = bbox;

  IOServiceFactory::destroyIOService(io_service);

  return bbox;
}

uint32 CoordinateSegmentationClient::numServers()  {
  if (mAvailableServersCount > 0) {
    //std::cout << "Returning cached numServers\n";
    return mAvailableServersCount;
  }

  //printf("Going to server for numServers\n");

  NumServersRequestMessage requestMessage;

  IOService* io_service = IOServiceFactory::makeIOService();

  TCPResolver resolver(*io_service);

  TCPResolver::query query(boost::asio::ip::tcp::v4(), GetOption("cseg-service-host")->as<String>(),
			               GetOption("cseg-service-tcp-port")->as<String>());

  TCPResolver::iterator endpoint_iterator = resolver.resolve(query);

  TCPResolver::iterator end;

  //std::cout << "Calling CSEG server for numservers!\n";
  TCPSocket socket(*io_service);
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
  if (error) {
    //std::cout << "Error connecting to  CSEG server for numservers\n";
    return  mAvailableServersCount;;
  }

  boost::asio::write(socket,
                     boost::asio::buffer((const void*)&requestMessage,sizeof (requestMessage)),
                     boost::asio::transfer_all() );

  //std::cout << "Sent over numservers request to cseg server\n";


  uint8* dataReceived = NULL;
  uint32 bytesReceived = 0;
  for (;;)
  {
      boost::system::error_code error;
      boost::array<uint8, 1048576> buf;

      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (dataReceived == NULL) {
	dataReceived = (uint8*) malloc (len);
      }
      else if (len > 0){
	dataReceived = (uint8*) realloc(dataReceived, bytesReceived+len);
      }
      memcpy(dataReceived+bytesReceived, buf.c_array(), len);

      bytesReceived += len;
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error) {
	//std::cout << "Error reading response from " << socket.remote_endpoint().address().to_string() <<" in numservers\n";
	//       throw boost::system::system_error(error); // Some other error.
	return mAvailableServersCount;
      }
  }

  //std::cout << "Received reply from cseg server for numservers"  << "\n";

  NumServersResponseMessage* response = (NumServersResponseMessage*)dataReceived;
  assert(response->type == NUM_SERVERS_RESPONSE);

  uint32 retval = response->numServers;
  mAvailableServersCount = retval;

  if (dataReceived != NULL) {
    free(dataReceived);
  }

  IOServiceFactory::destroyIOService(io_service);

  //std::cout << "numServers returned " <<  retval << "\n";
  return retval;
}

void CoordinateSegmentationClient::service() {
    mIOService->pollOne();
}

void CoordinateSegmentationClient::receiveMessage(Message* msg) {
}

void CoordinateSegmentationClient::csegChangeMessage(Sirikata::Protocol::CSeg::ChangeMessage* ccMsg) {

}

void CoordinateSegmentationClient::migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo ) {
}

void CoordinateSegmentationClient::downloadUpdatedBSPTree() {
}


} // namespace Sirikata
