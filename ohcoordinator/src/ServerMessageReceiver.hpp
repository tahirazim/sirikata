/*  Sirikata
 *  ServerMessageReceiver.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_SERVER_MESSAGE_RECEIVER_HPP_
#define _SIRIKATA_SERVER_MESSAGE_RECEIVER_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/ohcoordinator/SpaceContext.hpp>
#include <sirikata/core/service/TimeProfiler.hpp>
#include <sirikata/ohcoordinator/SpaceNetwork.hpp>
#include <sirikata/ohcoordinator/CoordinateSegmentation.hpp>
#include "RateEstimator.hpp"

namespace Sirikata{

class SpaceContext;
class Message;

/** ServerMessageReceiver handles receiving ServerMessages from the Network.  It
 *  pulls messages from the network as its rate limiting permits and handles
 *  distributing available bandwidth fairly between connections.  As messages
 *  are received from the network it pushes them up to a listener which can
 *  handle them -- no queueing is performed internally.
 */
class ServerMessageReceiver : public SpaceNetwork::ReceiveListener, CoordinateSegmentation::Listener {
public:
    class Listener {
      public:
        virtual ~Listener() {}

        virtual void serverConnectionReceived(ServerID sid) = 0;

        virtual void serverMessageReceived(Message* msg) = 0;
    };

    ServerMessageReceiver(SpaceContext* ctx, SpaceNetwork* net, Listener* listener);
    virtual ~ServerMessageReceiver();

    // Invoked by Forwarder when it needs to update the weight for a given
    // server.  Implementations shouldn't override, instead they should
    // implement the protected handleUpdateSenderStats which will occur on
    // receiver strand.
    void updateSenderStats(ServerID sid, double total_weight, double used_weight);

    // Get the total weight (real total, not just used) feeding into this queue.
    double totalUsedWeight();
    // Get the capacity of this receiver in bytes per second.
    double capacity();
    bool isBlocked() const{
        return mBlocked;
    }
protected:
    // SpaceNetwork::ReceiveListener Interface
    virtual void networkReceivedConnection(SpaceNetwork::ReceiveStream* strm) = 0;
    virtual void networkReceivedData(SpaceNetwork::ReceiveStream* strm) = 0;
    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation);
    // ServerMessageReceiver Protected (Implementation) Interface
    virtual void handleUpdateSenderStats(ServerID sid, double total_weight, double used_weight) = 0;

    SpaceContext* mContext;
    Network::IOStrand* mReceiverStrand;
    SpaceNetwork* mNetwork;
    TimeProfiler::Stage* mProfiler;
    Listener* mListener;

    // Total weights are handled by the main strand since that's the only place
    // they are needed. Handling of used weights is implementation dependent and
    // goes to the receiver strand.
    typedef std::tr1::unordered_map<ServerID, double> WeightMap;
    WeightMap mUsedWeights;
    double mUsedWeightSum;

    bool mBlocked;
    SimpleRateEstimator mCapacityEstimator;
    double mCapacityOverestimate;
};

} // namespace Sirikata

#endif //_SIRIKATA_SERVER_MESSAGE_RECEIVER_HPP_
