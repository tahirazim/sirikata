/*  Meru
 *  ResourceDownloadTask.cpp
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

#ifndef _DISTANCE_DOWNLOAD_PLANNER_HPP
#define _DISTANCE_DOWNLOAD_PLANNER_HPP

#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/mesh/ModelsSystem.hpp>
#include <sirikata/proxyobject/MeshListener.hpp>
#include "ResourceDownloadPlanner.hpp"
#include "../CameraEntity.hpp"
#include <vector>

namespace Sirikata {
namespace Graphics{
class MeshEntity;
}

class DistanceDownloadPlanner : public ResourceDownloadPlanner
{
public:
    DistanceDownloadPlanner(Provider<ProxyCreationListener*> *proxyManager, Context *c);
    ~DistanceDownloadPlanner();

    virtual void addNewObject(ProxyObjectPtr p, Graphics::MeshEntity *mesh);

    //ProxyCreationListener interface
    virtual void onCreateProxy ( ProxyObjectPtr object );
    virtual void onDestroyProxy ( ProxyObjectPtr object );

    //MeshListener interface
    virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh);

    //PollingService interface
    virtual void poll();
    virtual void stop();

    struct Resource {
        Resource(Graphics::MeshEntity *m, ProxyObjectPtr p) : mesh(m), proxy(p) {
            ready = false;
            file = NULL;
        }        virtual ~Resource(){}

        Transfer::URI *file;
        Graphics::MeshEntity *mesh;
        ProxyObjectPtr proxy;
        bool ready;
    };

    std::vector<Resource>::iterator findResource(ProxyObjectPtr p);

protected:
    std::vector<Resource> resources;
    virtual double calculatePriority(ProxyObjectPtr proxy);

};
}

#endif
