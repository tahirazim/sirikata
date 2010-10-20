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

#include "ResourceDownloadPlanner.hpp"
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include "../MeshEntity.hpp"
#include <stdlib.h>
#include <algorithm>

using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;
using namespace Sirikata::Graphics;

#define frequency 0.1

namespace Sirikata {

ResourceDownloadPlanner::ResourceDownloadPlanner(Provider<ProxyCreationListener*> *proxyManager, Context *c)
 : PollingService(c->mainStrand, Duration::seconds(frequency), c, "Resource Download Planner Poll")
{
    c->add(this);
    proxyManager->addListener(this);
    camera = NULL;
}

ResourceDownloadPlanner::~ResourceDownloadPlanner()
{

}

void ResourceDownloadPlanner::onCreateProxy(ProxyObjectPtr p)
{

}

void ResourceDownloadPlanner::onDestroyProxy(ProxyObjectPtr p)
{

}

void ResourceDownloadPlanner::addNewObject(ProxyObjectPtr p, MeshEntity *mesh)
{

}

void ResourceDownloadPlanner::setCamera(CameraEntity *entity)
{
    camera = entity;
}

void ResourceDownloadPlanner::onSetMesh(ProxyObjectPtr proxy, URI const &meshFile)
{

}

void ResourceDownloadPlanner::onSetScale (ProxyObjectPtr proxy, Vector3f const &scale)
{

}

void ResourceDownloadPlanner::onSetPhysical (ProxyObjectPtr proxy, PhysicalParameters const& pp)
{

}

void ResourceDownloadPlanner::poll()
{

}

void ResourceDownloadPlanner::stop()
{

}
}
