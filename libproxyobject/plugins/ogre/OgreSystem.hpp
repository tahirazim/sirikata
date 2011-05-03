/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  OgreSystem.hpp
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

#ifndef _SIRIKATA_OGRE_GRAPHICS_
#define _SIRIKATA_OGRE_GRAPHICS_
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/proxyobject/TimeSteppedQueryableSimulation.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include "OgreHeaders.hpp"
#include <OgreResourceManager.h>
#include <OgrePixelFormat.h>
#include <OgreWindowEventUtilities.h>
#include "resourceManager/ResourceDownloadPlanner.hpp"
#include "resourceManager/DistanceDownloadPlanner.hpp"
#include "resourceManager/SAngleDownloadPlanner.hpp"

#include "task/EventManager.hpp"
#include <sirikata/core/task/WorkQueue.hpp>

#include <sirikata/mesh/ModelsSystemFactory.hpp>

//Thank you Apple:
// /System/Library/Frameworks/CoreServices.framework/Headers/../Frameworks/CarbonCore.framework/Headers/MacTypes.h
#ifdef nil
#undef nil
#endif

namespace Ogre {
struct RaySceneQueryResultEntry;
class SubEntity;
}

namespace Sirikata {
class ProxyObject;
namespace Input {
class SDLInputManager;
}
/** Namespace for the OGRE Graphics Plugin: see class OgreSystem. */
namespace Graphics {
class Entity;
using Input::SDLInputManager;
class Camera;
class CubeMap;
struct IntersectResult;
class CDNArchivePlugin;
class OgreSystemMouseHandler;

/** Represents one OGRE SceneManager, a single environment. */
class OgreSystem: public TimeSteppedQueryableSimulation, protected SessionEventListener, public Ogre::WindowEventListener
{
    Context* mContext;
    VWObjectPtr mViewer;
    SpaceObjectReference mPresenceID;

    Vector4f mBackgroundColor;

    // FIXME because we don't have proper multithreaded support in cppoh, we
    // need to allocate our own thread dedicated to parsing
    Network::IOService* mParsingIOService;
    Network::IOWork* mParsingWork;
    Thread* mParsingThread;

    friend class OgreSystemMouseHandler;
    OgreSystemMouseHandler *mMouseHandler;
    void allocMouseHandler();
    void destroyMouseHandler();
    void tickInputHandler(const Task::LocalTime& t) const;

    SDLInputManager *mInputManager;
    Ogre::SceneManager *mSceneManager;
    bool mOgreOwnedRenderWindow;
    static Ogre::RenderTarget *sRenderTarget;
    Ogre::RenderTarget *mRenderTarget;
    Ogre::RenderWindow *mRenderWindow; // Should be the same as mRenderTarget,
                                       // but we need the RenderWindow form to
                                       // deal with window events.

    typedef std::tr1::unordered_map<SpaceObjectReference,Entity*,SpaceObjectReference::Hasher> SceneEntitiesMap;
    SceneEntitiesMap mSceneEntities;
    std::list<Entity*> mMovingEntities;

    friend class Entity; //Entity will insert/delete itself from these arrays.
    friend class Camera; //CameraEntity will insert/delete itself from the scene cameras array.
    OptionValue*mWindowWidth;
    OptionValue*mWindowHeight;
    OptionValue*mWindowDepth;
    OptionValue*mFullScreen;
    OptionValue* mOgreRootDir;
    ///How many seconds we aim to spend in each frame
    OptionValue*mFrameDuration;
    OptionSet*mOptions;
    Task::LocalTime mLastFrameTime;
    static Ogre::Plugin* sCDNArchivePlugin;
    static Ogre::Root* sRoot;
    static CDNArchivePlugin* mCDNArchivePlugin;

    String mResourcesDir;

    // FIXME need to support multiple parsers, see #124
    ModelsSystem* mModelParser;

    Transfer::TransferPoolPtr mTransferPool;

    OptionValue* mModelLights; // Use model or basic lights

    bool loadBuiltinPlugins();
    OgreSystem(Context* ctx);
    bool initialize(VWObjectPtr viewer, const SpaceObjectReference& presenceid, const String&options);
    // Loads system lights if they are being used.
    void loadSystemLights();
    // Helper for loadSystemLights.
    void constructSystemLight(const String& name, const Vector3f& direction);

    bool renderOneFrame(Task::LocalTime, Duration frameTime);
    ///all the things that should happen just before the frame
    void preFrame(Task::LocalTime, Duration);
    ///all the things that should happen once the frame finishes
    void postFrame(Task::LocalTime, Duration);
    void destroyRenderTarget(Ogre::ResourcePtr &name);

    void screenshot(const String& filename);

    void toggleSuspend();
    void suspend();
    void resume();

    // Initiate quiting by indicating to the main loop that we want to shut down
    void quit();

    bool mQuitRequested;
    bool mQuitRequestHandled;

    bool mSuspended;

    Ogre::RenderTarget* createRenderTarget(const String &name, uint32 width, uint32 height, bool automipmap, Ogre::PixelFormat pf);
    Vector3d mFloatingPointOffset;
    Ogre::RaySceneQuery* mRayQuery;
    CubeMap *mCubeMap;
    Entity* internalRayTrace(const Ogre::Ray &traceFrom,
                     bool aabbOnly,
                     int&resultCount,
                     double &returnResult,
                     Vector3f &returnNormal,
                     int& returnSubMesh,
                     IntersectResult *returnIntersectResult, bool texcoord,
                     int which=0) const;
public:
    OptionValue *mParallaxSteps;
    OptionValue *mParallaxShadowSteps;
    static std::list<OgreSystem*> sActiveOgreScenes;
    static uint32 sNumOgreSystems;
    std::tr1::unordered_set<Camera*> mAttachedCameras;
    Camera *mPrimaryCamera;

    Context* context() const { return mContext; }

    // For classes that only have access to OgreSystem and not a Context
    Time simTime();

    VWObjectPtr getViewer() const { return mViewer; }
    SpaceObjectReference getViewerPresence() const { return mPresenceID; }

    Transfer::TransferPoolPtr transferPool();

    String getResourcesDir() const { return mResourcesDir; }

    bool useModelLights() const;

    ///adds the camera to the list of attached cameras, making it the primary camera if it is first to be added
    void  attachCamera(const String&renderTargetName,Camera*);
    ///removes the camera from the list of attached cameras.
    void detachCamera(Camera*);
    Camera*getPrimaryCamera() {
        return mPrimaryCamera;
    }
    SDLInputManager *getInputManager() {
        return mInputManager;
    }
    OptionSet*getOptions(){
        return mOptions;
    }
    const OptionSet*getOptions()const{
        return mOptions;
    }

    const Vector3d& getOffset()const {return mFloatingPointOffset;}
    void destroyRenderTarget(const String &name);
    ///creates or restores a render target. if name is 0 length it will return the render target associated with this OgreSystem
    Ogre::RenderTarget* createRenderTarget(String name,uint32 width=0, uint32 height=0);

    static TimeSteppedQueryableSimulation* create(
        Context* ctx,
        VWObjectPtr obj,
        const SpaceObjectReference& presenceid,
        const String& options
    )
    {
        OgreSystem*os= new OgreSystem(ctx);
        if (os->initialize(obj, presenceid, options))
            return os;
        delete os;
        return NULL;
    }
    Entity* getEntity(const SpaceObjectReference &proxyId) const {
        SceneEntitiesMap::const_iterator iter = mSceneEntities.find(proxyId);
        if (iter != mSceneEntities.end()) {
            return (*iter).second;
        } else {
            return NULL;
        }
    }
    Entity* getEntity(const ProxyObjectPtr &proxy) const {
        return getEntity(proxy->getObjectReference());
    }

    typedef std::tr1::function<void(Mesh::MeshdataPtr)> ParseMeshCallback;
    /** Tries to parse a mesh. Can handle different types of meshes and tries to
     *  find the right parser using magic numbers.  If it is unable to find the
     *  right parser, returns NULL.  Otherwise, returns the parsed mesh as a
     *  Meshdata object.
     *  \param orig_uri original URI, used to construct correct relative paths
     *                  for dependent resources
     *  \param fp the fingerprint of the data, used for unique naming and passed
     *            through to the resulting mesh data
     *  \param data the contents of the
     *  \param cb callback to invoke when parsing is complete
     */
    void parseMesh(const Transfer::URI& orig_uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, ParseMeshCallback cb);
private:
    void parseMeshWork(const Transfer::URI& orig_uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, ParseMeshCallback cb);
public:

    bool queryRay(const Vector3d&position,
                  const Vector3f&direction,
                  const double maxDistance,
                  ProxyObjectPtr ignore,
                  double &returnDistance,
                  Vector3f &returnNormal,
                  SpaceObjectReference &returnName);

    Entity* rayTrace(const Vector3d &position,
                     const Vector3f &direction,
                     int&resultCount,
                     double &returnResult,
                     Vector3f&returnNormal,
                     int&subent,
                     int which=0) const;
    Entity* rayTraceAABB(const Vector3d &position,
                     const Vector3f &direction,
                     int&resultCount,
                     double &returnResult,
                     int&subent,
                     int which=0) const;
    virtual Duration desiredTickRate()const;
    ///returns if rendering should continue
    virtual void poll();
    virtual void stop();
    Ogre::RenderTarget *getRenderTarget();
    static Ogre::Root *getRoot();
    Ogre::SceneManager* getSceneManager();
    virtual void onCreateProxy(ProxyObjectPtr p); // MCB: interface from ProxyCreationListener
    virtual void onDestroyProxy(ProxyObjectPtr p); // MCB: interface from
                                                   // ProxyCreationListener


    // Event injection for SDL created windows.
    void injectWindowResized(uint32 w, uint32 h);

    // Ogre::WindowEventListener Interface overloads
    virtual void windowResized(Ogre::RenderWindow *rw);

    // ConnectionEventListener Interface
    virtual void onConnected(const Network::Address& addr);
    virtual void onDisconnected(const Network::Address& addr, bool requested, const String& reason);

    // SessionEventListener Interface
    virtual void onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name);

    // Methods for handling Invokable actions
    virtual boost::any invoke(std::vector<boost::any>& params);

    // Helper which creates a WebView window, either
    boost::any createWindow(const String& name, bool is_html, bool is_file, String content, uint32 width, uint32 height);
    // Create a window using a URL
    boost::any createWindow(std::vector<boost::any>& params);
    // Create a window using a file
    boost::any createWindowFile(std::vector<boost::any>& params);
    // Create a window using HTML
    boost::any createWindowHTML(std::vector<boost::any>& params);
    // Dynamically load a javscript UI module
    boost::any addModuleToUI(std::vector<boost::any>& params);

    // Set an input handler function which will be invoked for input
    // events, e.g. mouse and keyboard
    boost::any setInputHandler(std::vector<boost::any>& params);

    boost::any pick(std::vector<boost::any>& params);
    boost::any bbox(std::vector<boost::any>& params);

    boost::any initScript(std::vector<boost::any>& params);

    boost::any getCamera(std::vector<boost::any>& params);


    ~OgreSystem();

private:
    ResourceDownloadPlanner *dlPlanner;
    void instantiateAllObjects(ProxyManagerPtr pop);
};


} }
#endif
