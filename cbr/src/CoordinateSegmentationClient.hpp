/*  Sirikata
 *  CoordinateSegmentationClient.hpp
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

#ifndef _SIRIKATA_COORDINATE_SEGMENTATION_CLIENT_HPP_
#define _SIRIKATA_COORDINATE_SEGMENTATION_CLIENT_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/cbrcore/CoordinateSegmentation.hpp>
#include <sirikata/cbrcore/SegmentedRegion.hpp>

#include "CBR_CSeg.pbj.hpp"

namespace Sirikata {

class ServerIDMap;

/** Distributed BSP-tree based implementation of CoordinateSegmentation. */
class CoordinateSegmentationClient : public CoordinateSegmentation {
public:
  CoordinateSegmentationClient(SpaceContext* ctx, const BoundingBox3f& region, const Vector3ui32& perdim,
				ServerIDMap* sidmap );
    virtual ~CoordinateSegmentationClient();

    virtual ServerID lookup(const Vector3f& pos) ;
    virtual BoundingBoxList serverRegion(const ServerID& server) ;
    virtual BoundingBox3f region() ;
    virtual uint32 numServers() ;

    // From MessageRecipient
    virtual void receiveMessage(Message* msg);

    virtual void migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo );

private:
    virtual void service();

    BoundingBox3f mRegion;
    void csegChangeMessage(Sirikata::Protocol::CSeg::ChangeMessage* ccMsg);

    void downloadUpdatedBSPTree();

    SegmentedRegion mTopLevelRegion;
    bool mBSPTreeValid;

    Trace::Trace* mTrace;

    std::map<ServerID, BoundingBoxList> mServerRegionCache;

    uint16 mAvailableServersCount;

    Network::IOService* mIOService;  //creates an io service
    boost::shared_ptr<Network::TCPListener> mAcceptor;
    boost::shared_ptr<Network::TCPSocket> mSocket;

    ServerIDMap *  mSidMap;

    void startAccepting();
    void accept_handler();

    void sendSegmentationListenMessage();

}; // class CoordinateSegmentation

} // namespace Sirikata

#endif
