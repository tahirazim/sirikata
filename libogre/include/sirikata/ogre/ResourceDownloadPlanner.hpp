/*  Meru
 *  ResourceDownloadPlanner.hpp
 *
 *  Copyright (c) 2009, Stanford University
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

#ifndef _RESOURCE_DOWNLOAD_PLANNER_HPP
#define _RESOURCE_DOWNLOAD_PLANNER_HPP

#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/ogre/Camera.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>

namespace Sirikata {
namespace Graphics {

class Entity;

class SIRIKATA_OGRE_EXPORT ResourceDownloadPlanner : public PollingService
{
public:
    ResourceDownloadPlanner(Context* c);
    ~ResourceDownloadPlanner();

    virtual void addNewObject(ProxyObjectPtr p, Graphics::Entity *mesh);
    virtual void updateObject(ProxyObjectPtr p);
    virtual void removeObject(ProxyObjectPtr p) = 0;
    virtual void setCamera(Graphics::Camera *entity);

    //PollingService interface
    virtual void poll();
    virtual void stop();

    virtual void setMaxObjects(int32 new_max);
protected:
    Graphics::Camera *camera;
    int32 mMaxLoaded;
};

} // namespace Graphics
} // namespace Sirikata

#endif
