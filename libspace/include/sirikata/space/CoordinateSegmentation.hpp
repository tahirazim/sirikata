/*  Sirikata
 *  CoordinateSegmentation.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_COORDINATE_SEGMENTATION_HPP_
#define _SIRIKATA_COORDINATE_SEGMENTATION_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/SpaceContext.hpp>
#include <sirikata/space/LoadMonitor.hpp>
#include <sirikata/space/ServerMessage.hpp>
#include <sirikata/core/service/PollingService.hpp>

namespace Sirikata {

/** Handles the segmentation of the space into regions handled by servers.
 *  Answers queries of the type
 *   position -> ServerID
 *   ServerID -> region
 */
class SIRIKATA_SPACE_EXPORT CoordinateSegmentation : public MessageRecipient, public PollingService {
public:
    /** Listens for updates about the coordinate segmentation. */
    class Listener {
    public:
        virtual ~Listener() {}

        virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation) = 0;
    }; // class Listener

    CoordinateSegmentation(SpaceContext* ctx);
    virtual ~CoordinateSegmentation();

    virtual ServerID lookup(const Vector3f& pos) = 0;
    virtual BoundingBoxList serverRegion(const ServerID& server)  = 0;
    virtual BoundingBox3f region()  = 0;
    virtual uint32 numServers()  = 0;
    virtual std::vector<ServerID> lookupBoundingBox(const BoundingBox3f& bbox) = 0;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Callback from MessageDispatcher
    virtual void receiveMessage(Message* msg) = 0;

    virtual void reportLoad(ServerID sid, const BoundingBox3f& bbox, uint32 load) {  }

    virtual void migrationHint( std::vector<ServerLoadInfo>& svrLoadInfo ) {  }

    // FIXME this should be private but vis needs it for now
    virtual void service() = 0;

protected:
    virtual void poll();

    void notifyListeners(const std::vector<SegmentationInfo>& new_segmentation);

    SpaceContext* mContext;
private:
    CoordinateSegmentation();

    TimeProfiler::Stage* mServiceStage;
    std::set<Listener*> mListeners;
}; // class CoordinateSegmentation

} // namespace Sirikata

#endif
