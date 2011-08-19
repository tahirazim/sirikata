/*  Sirikata
 *  AssetDownloadTask.hpp
 *
 *  Copyright (c) 2011, Stanford University
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

#include <sirikata/ogre/OgreRenderer.hpp>
#include <sirikata/ogre/resourceManager/AssetDownloadTask.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/mesh/Billboard.hpp>
#include <sirikata/core/transfer/URL.hpp>

using namespace std::tr1::placeholders;
using namespace Sirikata::Transfer;
using namespace Sirikata::Mesh;

namespace Sirikata {
std::tr1::shared_ptr<AssetDownloadTask> AssetDownloadTask::construct(const Transfer::URI& uri, Graphics::OgreRenderer* const scene, double priority, FinishedCallback cb){
    std::tr1::shared_ptr<AssetDownloadTask> retval(SelfWeakPtr<AssetDownloadTask>::internalConstruct(new AssetDownloadTask(uri,scene,priority,cb)));
    retval->downloadAssetFile();
    return retval;
}
AssetDownloadTask::AssetDownloadTask(const Transfer::URI& uri, Graphics::OgreRenderer* const scene, double priority, FinishedCallback cb)
 : mScene(scene),
   mAssetURI(uri),
   mPriority(priority),
   mCB(cb),
   mAsset()
{
}

AssetDownloadTask::~AssetDownloadTask() {
    cancel();
}

Transfer::URI AssetDownloadTask::getURL(const Transfer::URI& orig, const String& given_url) {
    // We need to handle relative and absolute URIs:

    // If we can decode a URL, then we may need to handle relative URLs.
    Transfer::URL orig_url(orig);
    if (!orig_url.empty()) {
        // The URL constructor will figure out absolute/relative, or just fail
        // if it can't construct a valid URL.
        Transfer::URL deriv_url(orig_url.context(), given_url);
        if (!deriv_url.empty()) return Transfer::URI(deriv_url.toString());
    }

    // Otherwise, try to decode as a plain URI, ignoring the original URI. This
    // will either succeed with a full URI or fail and return an empty URI
    return Transfer::URI(given_url);
}

void AssetDownloadTask::cancel() {
    for(ActiveDownloadMap::iterator it = mActiveDownloads.begin(); it != mActiveDownloads.end(); it++)
        it->second->cancel();
    mActiveDownloads.clear();
}

void AssetDownloadTask::downloadAssetFile() {
    assert( !mAssetURI.empty() );

    ResourceDownloadTaskPtr dl = ResourceDownloadTask::construct(
        mAssetURI, mScene->transferPool(),
        mPriority,
        std::tr1::bind(&AssetDownloadTask::weakAssetFileDownloaded, getWeakPtr(), _1, _2)
    );
    mActiveDownloads[mAssetURI] = dl;
    dl->start();
}

void AssetDownloadTask::weakAssetFileDownloaded(std::tr1::weak_ptr<AssetDownloadTask> thus, std::tr1::shared_ptr<ChunkRequest> request, std::tr1::shared_ptr<const DenseData> response) {
    std::tr1::shared_ptr<AssetDownloadTask> locked(thus.lock());
    if (locked){
        locked->assetFileDownloaded(request,response);
    }
}

void AssetDownloadTask::assetFileDownloaded(std::tr1::shared_ptr<ChunkRequest> request, std::tr1::shared_ptr<const DenseData> response) {
    // Clear from the active download list
    assert(mActiveDownloads.size() == 1);
    mActiveDownloads.erase(mAssetURI);

    // Lack of response data means failure of some sort
    if (!response) {
        failDownload();
        return;
    }

    // FIXME here we could have another callback which lets them get
    // at the hash to try to use an existing copy. Even with the
    // eventual centralized loading we want, this may still be
    // beneficial since Ogre may have a copy even if we don't have a
    // copy of the raw data any more.

    mScene->parseMesh(
        mAssetURI, request->getMetadata().getFingerprint(), response,
        std::tr1::bind(&AssetDownloadTask::weakHandleAssetParsed, getWeakPtr(), _1)
    );
}

void AssetDownloadTask::weakHandleAssetParsed(std::tr1::weak_ptr<AssetDownloadTask> thus, Mesh::VisualPtr md){
    std::tr1::shared_ptr<AssetDownloadTask> locked(thus.lock());
    if (locked){
        locked->handleAssetParsed(md);
    }
}

void AssetDownloadTask::handleAssetParsed(Mesh::VisualPtr vis) {
    mAsset = vis;

    if (!vis) {
        SILOG(ogre,error,"Failed to parse mesh " << mAssetURI.toString());
        mCB();
        return;
    }

    // Now we need to handle downloads for each type of Visual.
    MeshdataPtr md( std::tr1::dynamic_pointer_cast<Meshdata>(vis) );
    if (md) {
        // This is a sanity check. There's no way Ogre can reasonably handle meshes
        // that require a ton of draw calls. Estimate them here and if its too high,
        // destroy the data and invoke the callback to make it look like failure.
        {
            // Draw calls =
            //   Number of instances * number of primitives in instance
            uint32 draw_calls = 0;
            Meshdata::GeometryInstanceIterator geoinst_it = md->getGeometryInstanceIterator();
            uint32 geoinst_idx;
            Matrix4x4f pos_xform;
            while( geoinst_it.next(&geoinst_idx, &pos_xform) )
                draw_calls += md->geometry[ md->instances[geoinst_idx].geometryIndex ].primitives.size();

            // Arbitrary number, but probably more than we should even allow given
            // that there are probably hundreds or thousands of other objects
            if (draw_calls > 500) {
                SILOG(ogre,error,"Excessively complicated mesh: " << mAssetURI.toString() << " has " << draw_calls << " draw calls. Ignoring this mesh.");
                mAsset = Mesh::VisualPtr();
                mCB();
                return;
            }
        }
        // Another sanity check: if we have an excessive number of textures,
        // we're probably going to hit some memory constraints
        {
            // Complete arbitrary number
            if (md->textures.size() > 50) {
                SILOG(ogre,error, "Mesh with excessive number of textures: " << mAssetURI.toString() << " has " << md->textures.size() << " textures. Ignoring this mesh.");
                mAsset = Mesh::VisualPtr();
                mCB();
                return;
            }
        }



        // Special case for no dependent downloads
        if (md->textures.size() == 0) {
            mCB();
            return;
        }

        for(TextureList::const_iterator it = md->textures.begin(); it != md->textures.end(); it++) {
            Transfer::URI texURI( getURL(mAssetURI, *it) );
            addDependentDownload(texURI);
        }
        startDependentDownloads();

        return;
    }

    BillboardPtr bboard( std::tr1::dynamic_pointer_cast<Billboard>(vis) );
    if (bboard) {
        // For billboards, we have to download at least the image to display on
        // it
        Transfer::URI texURI( getURL(mAssetURI, bboard->image) );
        addDependentDownload(texURI);
        startDependentDownloads();
        return;
    }

    // If we've gotten here, then we haven't handled the specific type of visual
    // and we need to issue a warning and callback.
    SILOG(ogre, error, "Tried to use AssetDownloadTask for a visual type it doesn't handle (" << vis->type() << "). Not downloading dependent resources.");
    mCB();
}

void AssetDownloadTask::addDependentDownload(const Transfer::URI& depUrl) {
    // Sometimes we get duplicate references, so make sure we're not already
    // working on this one.
    if (mActiveDownloads.find(depUrl) != mActiveDownloads.end()) return;

    ResourceDownloadTaskPtr dl = ResourceDownloadTask::construct(
        depUrl, mScene->transferPool(),
        mPriority,
        std::tr1::bind(&AssetDownloadTask::weakTextureDownloaded, getWeakPtr(), _1, _2)
    );
    mActiveDownloads[depUrl] = dl;
}

void AssetDownloadTask::startDependentDownloads() {
    // Copy since we could get callbacks as soon as we start the downloads
    ActiveDownloadMap downloads_copy(mActiveDownloads);
    for(ActiveDownloadMap::iterator it = downloads_copy.begin(); it != downloads_copy.end(); it++)
        it->second->start();
}

void AssetDownloadTask::weakTextureDownloaded(const std::tr1::weak_ptr<AssetDownloadTask>&thus, std::tr1::shared_ptr<ChunkRequest> request, std::tr1::shared_ptr<const DenseData> response) {
    std::tr1::shared_ptr<AssetDownloadTask>locked(thus.lock());
    if (locked) {
        locked->textureDownloaded(request,response);
    }
}

void AssetDownloadTask::textureDownloaded(std::tr1::shared_ptr<ChunkRequest> request, std::tr1::shared_ptr<const DenseData> response) {
    // This could be triggered by any CDN thread, protect access
    // (mActiveDownloads, mDependencies)
    boost::mutex::scoped_lock lok(mDependentDownloadMutex);

    if (!request) {
        failDownload();
        return;
    }

    // Clear the download task
    mActiveDownloads.erase(request->getURI());

    // Lack of response data means failure of some sort
    if (!response) {
        failDownload();
        return;
    }

    // Store data for later use
    mDependencies[request->getURI()].request = request;
    mDependencies[request->getURI()].response = response;

    if (mActiveDownloads.size() == 0)
        mCB();
}

void AssetDownloadTask::failDownload() {
    // Cancel will stop the current download process.
    cancel();

    // In this case, since it wasn't user requested, we also clear any parsed
    // data (e.g. if we failed on a texture download) and trigger a callback to
    // let them know about the failure.
    mAsset.reset();
    mCB();
}

} // namespace Sirikata
