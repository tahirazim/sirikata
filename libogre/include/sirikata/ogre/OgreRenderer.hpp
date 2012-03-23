/*  Sirikata
 *  OgreRenderer.hpp
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

#ifndef _SIRIKATA_OGRE_OGRE_RENDERER_HPP_
#define _SIRIKATA_OGRE_OGRE_RENDERER_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/OgreHeaders.hpp>
#include "OgreResource.h"
#include <sirikata/mesh/Visual.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/oh/TimeSteppedSimulation.hpp>
#include <OgreWindowEventUtilities.h>
#include <sirikata/core/util/Liveness.hpp>

namespace Sirikata {

namespace Input {
class SDLInputManager;
}

namespace Mesh {
class Filter;
}
class ModelsSystem;

namespace Graphics {

class Camera;
class Entity;

class ResourceLoader;
class CDNArchivePlugin;
class ResourceDownloadPlanner;

using Input::SDLInputManager;

class ParseMeshTaskInfo {
public:
    ParseMeshTaskInfo()
     : mProcess(true)
    {}

    void cancel() {
        mProcess = false;
    }

    bool process() const { return mProcess; }
private:
    bool mProcess;
};
typedef std::tr1::shared_ptr<ParseMeshTaskInfo> ParseMeshTaskHandle;


/** Represents a SQLite database connection. */
class SIRIKATA_OGRE_EXPORT OgreRenderer :
        public TimeSteppedSimulation,
        public Ogre::WindowEventListener,
        public virtual Liveness
{
public:
    OgreRenderer(Context* ctx,Network::IOStrandPtr sStrand);
    virtual ~OgreRenderer();

    virtual bool initialize(const String& options, bool with_berkelium = true);

    Context* context() const { return mContext; }

    void toggleSuspend();
    void suspend();
    void resume();

    // Initiate quiting by indicating to the main loop that we want to shut down
    void quit();

    // Event injection for SDL created windows.
    void injectWindowResized(uint32 w, uint32 h);

    const Vector3d& getOffset() const { return mFloatingPointOffset; }

    virtual Transfer::TransferPoolPtr transferPool();

    ResourceDownloadPlanner* downloadPlanner() { return mDownloadPlanner; }

    virtual Ogre::RenderTarget* createRenderTarget(const String &name, uint32 width, uint32 height, bool automipmap, int pixelFmt);
    virtual Ogre::RenderTarget* createRenderTarget(String name,uint32 width=0, uint32 height=0);
    virtual void destroyRenderTarget(Ogre::ResourcePtr& name);
    virtual void destroyRenderTarget(const String &name);

    Network::IOStrandPtr renderStrand()
    {
        return simStrand;
    }


    Ogre::SceneManager* getSceneManager() {
        return mSceneManager;
    }

    Ogre::SceneManager* getOverlaySceneManager() {
        return mOverlaySceneManager;
    }

    Ogre::RenderTarget* getRenderTarget() {
        return mRenderTarget;
    }

    SDLInputManager *getInputManager() {
        return mInputManager;
    }

    ResourceLoader* getResourceLoader() const {
        return mResourceLoader;
    }

    // TimeSteppedSimulation Interface
    virtual void poll();
    virtual void stop();

    // Invokable Interface
    //should only invoke from within simStrand
    virtual boost::any invoke(std::vector<boost::any>& params);


    // Ogre::WindowEventListener Interface overloads
    virtual void windowResized(Ogre::RenderWindow *rw);
    virtual void windowFocusChange(Ogre::RenderWindow *rw);

    // Options values
    virtual float32 nearPlane();
    virtual float32 farPlane();
    virtual float32 parallaxSteps();
    virtual int32 parallaxShadowSteps();

    ///adds the camera to the list of attached cameras, making it the primary camera if it is first to be added
    virtual void attachCamera(const String& renderTargetName, Camera*);
    ///removes the camera from the list of attached cameras.
    virtual void detachCamera(Camera*);

    void addObject(Entity* ent, const Transfer::URI& mesh);
    void removeObject(Entity* ent);

    typedef std::tr1::function<void(Mesh::VisualPtr)> ParseMeshCallback;

    /** Tries to parse a mesh. Can handle different types of meshes and tries to
     *  find the right parser using magic numbers.  If it is unable to find the
     *  right parser, returns NULL.  Otherwise, returns the parsed mesh as a
     *  Visual object.
     *  \param metadata RemoteFileMetadata describing the remote resource
     *  \param fp the fingerprint of the data, used for unique naming and passed
     *            through to the resulting mesh data
     *  \param data the contents of the
     *  \param cb callback to invoke when parsing is complete
     *
     *  \returns A handle you can use to cancel the task. You aren't required to
     *  hold onto it if you don't need to be able to cancel the request.
     */
    ParseMeshTaskHandle parseMesh(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, bool isAggregate, ParseMeshCallback cb);

    /** Get the default mesh to present if a model fails to load. This may
     *  return an empty VisualPtr if no default mesh is specified.
     */
    virtual Mesh::VisualPtr defaultMesh() const { return Mesh::VisualPtr(); }

    void screenshot(const String& filename);
    void screenshotNextFrame(const String& filename);
  protected:
    Network::IOStrandPtr simStrand;
    static Ogre::Root *getRoot();

    bool loadBuiltinPlugins();
    // Loads system lights if they are being used.
    void loadSystemLights();
    // Helper for loadSystemLights.
    void constructSystemLight(const String& name, const Vector3f& direction, float brightness);

    bool useModelLights() const;

    virtual bool renderOneFrame(Task::LocalTime, Duration frameTime);
    ///all the things that should happen just before the frame
    virtual void preFrame(Task::LocalTime, Duration);
    ///all the things that should happen once the frame finishes
    virtual void postFrame(Task::LocalTime, Duration);

    void iStop(Liveness::Token rendererAlive);

    void parseMeshWork(
        Liveness::Token rendererAlive,
        ParseMeshTaskHandle handle,
        const Transfer::RemoteFileMetadata& metadata,
        const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data,
        bool isAggregate, ParseMeshCallback cb);

    Mesh::VisualPtr parseMeshWorkSync(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, bool isAggregate);


    // Invokable helpers

    // Set handler to be called on each tick, i.e. before each frame
    boost::any setOnTick(std::vector<boost::any>& params);
    boost::any setMaxObjects(std::vector<boost::any>& params);

    static Ogre::Root* sRoot;
    static Ogre::Plugin* sCDNArchivePlugin;
    static CDNArchivePlugin* mCDNArchivePlugin;
    static std::list<OgreRenderer*> sActiveOgreScenes;
    static uint32 sNumOgreSystems;
    static Ogre::RenderTarget *sRenderTarget; // FIXME why a static render target?

    Context* mContext;

    bool mQuitRequested;
    bool mQuitRequestHandled;

    bool mSuspended;

    // FIXME because we don't have proper multithreaded support in cppoh, we
    // need to allocate our own thread dedicated to parsing
    Network::IOService* mParsingIOService;
    Network::IOWork* mParsingWork;
    Thread* mParsingThread;

    SDLInputManager *mInputManager;
    Ogre::SceneManager *mSceneManager;
    Ogre::SceneManager *mOverlaySceneManager;
    bool mOgreOwnedRenderWindow;

    Ogre::RenderTarget *mRenderTarget;
    Ogre::RenderWindow *mRenderWindow; // Should be the same as mRenderTarget,
                                       // but we need the RenderWindow form to
                                       // deal with window events.


    OptionValue*mWindowWidth;
    OptionValue*mWindowHeight;
    OptionValue*mWindowDepth;
    OptionValue*mFullScreen;
    OptionValue* mOgreRootDir;
    ///How many seconds we aim to spend in each frame
    OptionValue*mFrameDuration;
    OptionValue *mParallaxSteps;
    OptionValue *mParallaxShadowSteps;
    OptionValue* mModelLights; // Use model or basic lights
    OptionSet* mOptions;
    std::vector<String> mSearchPaths;
    Vector4f mBackgroundColor;
    Vector3d mFloatingPointOffset;

    Task::LocalTime mLastFrameTime;
    Invokable* mOnTickCallback;


    String mResourcesDir;

    TimeProfiler::Stage* mParserProfiler;
    ModelsSystem* mModelParser;
    Mesh::Filter* mModelFilter;
    Mesh::Filter* mCenteringFilter;

    Transfer::TransferPoolPtr mTransferPool;

    ResourceLoader* mResourceLoader;
    ResourceDownloadPlanner* mDownloadPlanner;

    typedef std::tr1::unordered_map<String,Entity*> SceneEntitiesMap;
    SceneEntitiesMap mSceneEntities;
    std::list<Entity*> mMovingEntities; // FIXME used to call extrapolate
                                        // location. register by Entity, but
                                        // only ProxyEntities use it

    typedef std::tr1::unordered_set<Camera*> CameraSet;
    CameraSet mAttachedCameras;

    friend class Entity; //Entity will insert/delete itself from these arrays.
    friend class Camera; //CameraEntity will insert/delete itself from the scene
                         //cameras array.

    // To simplify taking screenshots after a specific event has occurred, we
    // allow them to be taken on the next frame.
    String mNextFrameScreenshotFile;
    bool initialized;
    bool stopped;

};

} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_OGRE_RENDERER_HPP_
