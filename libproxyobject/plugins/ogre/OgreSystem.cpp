/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  OgreSystem.cpp
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
#include <sirikata/proxyobject/Platform.hpp>

#include <sirikata/core/util/Thread.hpp>

#include <sirikata/core/options/Options.hpp>
#include "OgreSystem.hpp"
#include "OgreSystemMouseHandler.hpp"
#include "OgrePlugin.hpp"
#include "task/Event.hpp"
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include "Camera.hpp"
#include "Entity.hpp"
#include "Lights.hpp"
#include <Ogre.h>
#include "CubeMap.hpp"
#include "input/SDLInputManager.hpp"
#include "input/InputDevice.hpp"
#include "input/InputEvents.hpp"
#include "OgreMeshRaytrace.hpp"
#include "resourceManager/CDNArchivePlugin.hpp"
#include "resourceManager/ResourceDownloadTask.hpp"

#include <sirikata/core/util/DynamicLibrary.hpp>

#include <boost/filesystem.hpp>
#include <stdio.h>

#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>

using namespace std;

//#include </Developer/SDKs/MacOSX10.4u.sdk/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/HIView.h>
#include "WebViewManager.hpp"

//volatile char assert_thread_support_is_gequal_2[OGRE_THREAD_SUPPORT*2-3]={0};
//volatile char assert_thread_support_is_lequal_2[5-OGRE_THREAD_SUPPORT*2]={0};
//enable the below when NEDMALLOC is turned off, so we can verify that NEDMALLOC is off
//volatile char assert_malloc_is_gequal_1[OGRE_MEMORY_ALLOCATOR*2-1]={0};
//volatile char assert_malloc_is_lequal_1[3-OGRE_MEMORY_ALLOCATOR*2]={0};

namespace Sirikata {
namespace Graphics {

namespace {

// FIXME we really need a better way to figure out where our data is
// This is a generic search method. It searches upwards from the current
// directory for any of the specified files and returns the first path it finds
// that contains one of them.
std::string findResource(boost::filesystem::path* search_paths, uint32 nsearch_paths, bool want_dir = true, const std::string& start_path = ".", boost::filesystem::path default_ = boost::filesystem::complete(boost::filesystem::path("."))) {
    using namespace boost::filesystem;

    path start_path_path = boost::filesystem::complete(start_path);

    // FIXME there probably need to be more of these, including
    // some absolute paths of expected installation locations.
    // It should also be possible to add some from the options.
    path search_offsets[] = {
        path("."),
        path(".."),
        path("../.."),
        path("../../.."),
        path("../../../.."),
        path("../../../../.."),
        path("../../../../../.."),
        path("../../../../../../..")
    };
    uint32 nsearch_offsets = sizeof(search_offsets)/sizeof(*search_offsets);

    for(uint32 offset = 0; offset < nsearch_offsets; offset++) {
        for(uint32 spath = 0; spath < nsearch_paths; spath++) {
            path full = start_path_path / search_offsets[offset] / search_paths[spath];
            if (exists(full) && (!want_dir || is_directory(full)))
                return full.string();
        }
    }

    // If we can't find it anywhere else, just let it try to use the current directory
    return default_.string();
}

// FIXME we really need a better way to figure out where our data is
std::string getOgreResourcesDir() {
    using namespace boost::filesystem;

    // FIXME there probably need to be more of these
    // The current two reflect what we'd expect for installed
    // and what's in the source tree.
    path search_paths[] = {
        path("ogre/data"),
        path("share/ogre/data"),
        path("libproxyobject/plugins/ogre/data")
    };
    uint32 nsearch_paths = sizeof(search_paths)/sizeof(*search_paths);

    return findResource(search_paths, nsearch_paths);
}

// FIXME we really need a better way to figure out where our data is
std::string getBerkeliumBinaryDir() {
    using namespace boost::filesystem;

    // FIXME there probably need to be more of these
    // The current two reflect what we'd expect for installed
    // and what's in the source tree.
    path search_paths[] = {
#if SIRIKATA_PLATFORM == PLATFORM_MAC
        // On mac we must be in a .app/Contents
        // It needs to be there so that running the .app from the Finder works
        // and therefore the resources for berkelium are setup to be found from
        // that location. However, the binaries are in .app/Contents/MacOS, so
        // that needs to be the search path
        path("MacOS")
#else
        path("chrome"),
        path("build/cmake/chrome")
#endif
    };
    uint32 nsearch_paths = sizeof(search_paths)/sizeof(*search_paths);

    return findResource(search_paths, nsearch_paths);
}

std::string getChromeResourcesDir() {
    using namespace boost::filesystem;

    return (path(getOgreResourcesDir()) / "chrome").string();
}

}


Ogre::Root *OgreSystem::sRoot;
CDNArchivePlugin *OgreSystem::mCDNArchivePlugin=NULL;
Ogre::RenderTarget* OgreSystem::sRenderTarget=NULL;
Ogre::Plugin*OgreSystem::sCDNArchivePlugin=NULL;
std::list<OgreSystem*> OgreSystem::sActiveOgreScenes;
uint32 OgreSystem::sNumOgreSystems=0;
OgreSystem::OgreSystem(Context* ctx)
 : TimeSteppedQueryableSimulation(ctx, Duration::seconds(1.f/60.f), "Ogre Graphics"),
   mContext(ctx),
   mLastFrameTime(Task::LocalTime::now()),
   mResourcesDir(getOgreResourcesDir()),
   mModelParser( ModelsSystemFactory::getSingleton ().getConstructor ( "any" ) ( "" ) ),
     mQuitRequested(false),
     mQuitRequestHandled(false),
     mFloatingPointOffset(0,0,0),
     mSuspended(false),
     mPrimaryCamera(NULL)
{
    increfcount();
    mCubeMap=NULL;
    mInputManager=NULL;
    mRenderTarget=NULL;
    mSceneManager=NULL;
    mRenderTarget=NULL;
    mMouseHandler=NULL;
    mRayQuery=NULL;
}
namespace {
class FrequencyType{public:
    static Any lexical_cast(const std::string&value) {
        double val=60;
        std::istringstream ss(value);
        ss>>val;
        if (val==0.)
            val=60.;
        return Duration::seconds(1./val);
    }
};
class ShadowType{public:
        static bool caseq(const std::string&a, const std::string&b){
            if (a.length()!=b.length())
                return false;
            for (std::string::const_iterator ia=a.begin(),iae=a.end(),ib=b.begin();
                 ia!=iae;
                 ++ia,++ib) {
                using namespace std;
                if (toupper(*ia)!=toupper(*ib))
                    return false;
            }
            return true;
        }
        static Any lexical_cast(const std::string&value) {
            Ogre::ShadowTechnique st=Ogre::SHADOWTYPE_NONE;
            if (caseq(value,"textureadditive"))
                return st=Ogre::SHADOWTYPE_TEXTURE_ADDITIVE;
            if (caseq(value,"texturemodulative"))
                return st=Ogre::SHADOWTYPE_TEXTURE_MODULATIVE;
            if (caseq(value,"textureadditiveintegrated"))
                return st=Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED;
            if (caseq(value,"texturemodulativeintegrated"))
                return st=Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED;
            if (caseq(value,"stenciladditive"))
                return st=Ogre::SHADOWTYPE_TEXTURE_ADDITIVE;
            if (caseq(value,"stencilmodulative"))
                return st=Ogre::SHADOWTYPE_TEXTURE_MODULATIVE;
            if (caseq(value,"none"))
                return st=Ogre::SHADOWTYPE_NONE;
            return st=Ogre::SHADOWTYPE_NONE;
        }
};
class OgrePixelFormatParser{public:
        static Any lexical_cast(const std::string&value) {
            Ogre::PixelFormat fmt=Ogre::PF_A8B8G8R8;
            if (value=="16")
                return fmt=Ogre::PF_FLOAT16_RGBA;
            if (value=="32")
                return fmt=Ogre::PF_FLOAT32_RGBA;
            if (value=="dxt1"||value=="DXT1")
                return fmt=Ogre::PF_DXT1;
            if (value=="dxt3"||value=="DXT3")
                return fmt=Ogre::PF_DXT3;
            if (value=="dxt5"||value=="DXT5")
                return fmt=Ogre::PF_DXT5;
            if (value=="8")
                return fmt=Ogre::PF_R8G8B8;
            if (value=="8a")
                return fmt=Ogre::PF_A8R8G8B8;
            return fmt;
        }
};
class BugfixRenderTexture:public Ogre::RenderTexture{
    Ogre::HardwarePixelBufferSharedPtr mHardwarePixelBuffer;
public:
    BugfixRenderTexture(Ogre::HardwarePixelBufferSharedPtr hpbp):Ogre::RenderTexture(&*hpbp,0),mHardwarePixelBuffer(hpbp) {

    }
    virtual bool requiresTextureFlipping() const{
        return false;
    }
};
}
void OgreSystem::destroyRenderTarget(const String&name) {
    if (mRenderTarget->getName()==name) {

    }else {
        Ogre::ResourcePtr renderTargetTexture=Ogre::TextureManager::getSingleton().getByName(name);
        if (!renderTargetTexture.isNull())
            destroyRenderTarget(renderTargetTexture);
    }
}

Time OgreSystem::simTime() {
    return mContext->simTime();
}

Transfer::TransferPoolPtr OgreSystem::transferPool() {
    return mTransferPool;
}

void OgreSystem::suspend() {
  mSuspended = !mSuspended;
}
void OgreSystem::destroyRenderTarget(Ogre::ResourcePtr&name) {
    Ogre::TextureManager::getSingleton().remove(name);
}
void OgreSystem::quit() {
    mQuitRequested = true;
}
Ogre::RenderTarget*OgreSystem::createRenderTarget(String name, uint32 width, uint32 height) {
    if (name.length()==0&&mRenderTarget)
        name=mRenderTarget->getName();
    if (width==0) width=mWindowWidth->as<uint32>();
    if (height==0) height=mWindowHeight->as<uint32>();
    return createRenderTarget(name,width,height,true,mWindowDepth->as<Ogre::PixelFormat>());
}
Ogre::RenderTarget*OgreSystem::createRenderTarget(const String&name, uint32 width, uint32 height, bool automipmap, Ogre::PixelFormat pf) {
    if (mRenderTarget&&mRenderTarget->getName()==name) {
        return mRenderTarget;
    }else if (sRenderTarget&&sRenderTarget->getName()==name) {
        return sRenderTarget;
    }else {
        Ogre::TexturePtr texptr=Ogre::TextureManager::getSingleton().getByName(name);
        if (texptr.isNull()) {
            texptr=Ogre::TextureManager::getSingleton().createManual(name,
                                                                     "Sirikata",
                                                                     Ogre::TEX_TYPE_2D,
                                                                     width,
                                                                     height,
                                                                     1,
                                                                     automipmap?Ogre::MIP_DEFAULT:1,
                                                                     pf,
                                                                     automipmap?(Ogre::TU_RENDERTARGET|Ogre::TU_AUTOMIPMAP|Ogre::TU_DYNAMIC|Ogre::TU_WRITE_ONLY):(Ogre::TU_RENDERTARGET|Ogre::TU_DYNAMIC|Ogre::TU_WRITE_ONLY));
        }
        try {
            return texptr->getBuffer()->getRenderTarget();
        }catch (Ogre::Exception &e) {
            if (e.getNumber()==Ogre::Exception::ERR_RENDERINGAPI_ERROR) {
                width=texptr->getWidth();
                height=texptr->getHeight();
                uint32 nummipmaps=texptr->getNumMipmaps();
                pf=texptr->getFormat();
                Ogre::ResourcePtr resourceTexPtr=texptr;
                Ogre::TextureManager::getSingleton().remove(resourceTexPtr);
                texptr=Ogre::TextureManager::getSingleton().createManual(name,
                                                                         "Sirikata",
                                                                         Ogre::TEX_TYPE_2D,
                                                                         width,
                                                                         height,
                                                                         1,
                                                                         automipmap?Ogre::MIP_DEFAULT:1,
                                                                         pf,
                                                                         automipmap?(Ogre::TU_RENDERTARGET|Ogre::TU_AUTOMIPMAP|Ogre::TU_DYNAMIC|Ogre::TU_WRITE_ONLY):(Ogre::TU_RENDERTARGET|Ogre::TU_DYNAMIC|Ogre::TU_WRITE_ONLY));
                return texptr->getBuffer()->getRenderTarget();
            }else throw;
        }
    }
}
void    setupResources(const String &filename){
    using namespace Ogre;
    ConfigFile cf;
    cf.load(filename);    // Go through all sections & settings in the file
    ConfigFile::SectionIterator seci = cf.getSectionIterator();

    String secName, typeName, archName;
    while (seci.hasMoreElements()) {
        secName = seci.peekNextKey();
        ConfigFile::SettingsMultiMap *settings = seci.getNext();
        ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;

            ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
        }
    }
    ResourceGroupManager::getSingleton().addResourceLocation(".", typeName, secName);

    ResourceGroupManager::getSingleton().initialiseAllResourceGroups(); /// Although the override is optional, this is mandatory
}

void OgreSystem::attachCamera(const String &renderTargetName, Camera* entity) {
    mAttachedCameras.insert(entity);
    if (renderTargetName.empty()) {
        dlPlanner->setCamera(entity);
        std::vector<String> cubeMapNames;

        std::vector<Vector3f> cubeMapOffsets;
        std::vector<float> cubeMapNearPlanes;
        cubeMapNames.push_back("ExteriorCubeMap");
        cubeMapOffsets.push_back(Vector3f(0,0,0));
        cubeMapNearPlanes.push_back(10);
        cubeMapNames.push_back("InteriorCubeMap");
        cubeMapOffsets.push_back(Vector3f(0,0,0));
        cubeMapNearPlanes.push_back(0.1);
        try {
            mCubeMap=new CubeMap(this,cubeMapNames,512,cubeMapOffsets, cubeMapNearPlanes);
        }catch (std::bad_alloc&) {
            mCubeMap=NULL;
        }

    }
}

void OgreSystem::detachCamera(Camera* entity) {
    if (mAttachedCameras.find(entity) == mAttachedCameras.end()) return;

    if (mPrimaryCamera == entity) {
        mPrimaryCamera = NULL;
        delete mCubeMap;
        mCubeMap = NULL;
    }
    mAttachedCameras.erase(entity);
}

void OgreSystem::instantiateAllObjects(ProxyManagerPtr pman)
{
    std::vector<SpaceObjectReference> allORefs;
    pman->getAllObjectReferences(allORefs);

    for (std::vector<SpaceObjectReference>::iterator iter = allORefs.begin(); iter != allORefs.end(); ++iter)
    {
        //instantiate each object in graphics system separately.
        ProxyObjectPtr toAdd = pman->getProxyObject(*iter);
        onCreateProxy(toAdd);
    }
}

void OgreSystem::constructSystemLight(const String& name, const Vector3f& direction) {
    LightInfo li;

    Color lcol(1.f, 1.f, 1.f);
    li.setLightDiffuseColor(lcol);
    li.setLightSpecularColor(lcol);
    li.setLightFalloff(1.f, 0.f, 0.f);
    li.setLightType(LightInfo::DIRECTIONAL);
    li.setLightRange(10000.f);

    Ogre::Light* light = constructOgreLight(getSceneManager(), String("____system_") + name, li);
    light->setDirection(toOgre(direction));

    getSceneManager()->getRootSceneNode()->attachObject(light);
}

void OgreSystem::loadSystemLights() {
    if (useModelLights()) return;

    constructSystemLight("forward", Vector3f(0.f, 0.f, 1.f));
    constructSystemLight("back", Vector3f(0.f, 0.f, -1.f));
    constructSystemLight("left", Vector3f(-1.f, 0.f, 0.f));
    constructSystemLight("right", Vector3f(1.f, 0.f, 0.f));
    constructSystemLight("up", Vector3f(0.f, 1.f, 0.f));
    constructSystemLight("down", Vector3f(0.f, -1.f, 0.f));
}

bool OgreSystem::initialize(VWObjectPtr viewer, const SpaceObjectReference& presenceid, const String& options) {
    mViewer = viewer;
    mPresenceID = presenceid;

    mParsingIOService = Network::IOServiceFactory::makeIOService();
    mParsingWork = new Network::IOWork(*mParsingIOService, "Ogre Mesh Parsing");
    mParsingThread = new Sirikata::Thread(std::tr1::bind(&Network::IOService::runNoReturn, mParsingIOService));

    ProxyManagerPtr proxyManager = mViewer->presence(presenceid);
    mViewer->addListener((SessionEventListener*)this);

    ++sNumOgreSystems;
    proxyManager->addListener(this);

    //initialize the Resource Download Planner
    dlPlanner = new SAngleDownloadPlanner(mContext);

    //add ogre system options here
    OptionValue*pluginFile;
    OptionValue*configFile;
    OptionValue*ogreLogFile;
    OptionValue*purgeConfig;
    OptionValue* keybindingFile;
    OptionValue*createWindow;
    OptionValue*ogreSceneManager;
    OptionValue*windowTitle;
    OptionValue*shadowTechnique;
    OptionValue*shadowFarDistance;
    OptionValue*renderBufferAutoMipmap;
    OptionValue*grabCursor;
    OptionValue* backColor;

    InitializeClassOptions("ogregraphics",this,
                           pluginFile=new OptionValue("pluginfile","plugins.cfg",OptionValueType<String>(),"sets the file ogre should read options from."),
                           configFile=new OptionValue("configfile","ogre.cfg",OptionValueType<String>(),"sets the ogre config file for config options"),
                           ogreLogFile=new OptionValue("logfile","Ogre.log",OptionValueType<String>(),"sets the ogre log file"),
                           purgeConfig=new OptionValue("purgeconfig","false",OptionValueType<bool>(),"Pops up the dialog asking for the screen resolution no matter what"),
                           keybindingFile=new OptionValue("keybinding","keybinding.default",OptionValueType<String>(),"File to load key bindings from"),
                           createWindow=new OptionValue("window","true",OptionValueType<bool>(),"Render to a onscreen window"),
                           grabCursor=new OptionValue("grabcursor","false",OptionValueType<bool>(),"Grab cursor"),
                           windowTitle=new OptionValue("windowtitle","Sirikata",OptionValueType<String>(),"Window title name"),
                           mOgreRootDir=new OptionValue("ogretoplevel","",OptionValueType<String>(),"Directory with ogre plugins"),
                           ogreSceneManager=new OptionValue("scenemanager","OctreeSceneManager",OptionValueType<String>(),"Which scene manager to use to arrange objects"),
                           mWindowWidth=new OptionValue("windowwidth","1024",OptionValueType<uint32>(),"Window width"),
                           mFullScreen=new OptionValue("fullscreen","false",OptionValueType<bool>(),"Fullscreen"),
                           mWindowHeight=new OptionValue("windowheight","768",OptionValueType<uint32>(),"Window height"),
                           mWindowDepth=new OptionValue("colordepth","8",OgrePixelFormatParser(),"Pixel color depth"),
                           renderBufferAutoMipmap=new OptionValue("rendertargetautomipmap","false",OptionValueType<bool>(),"If the render target needs auto mipmaps generated"),
                           mFrameDuration=new OptionValue("fps","30",FrequencyType(),"Target framerate"),
                           shadowTechnique=new OptionValue("shadows","none",ShadowType(),"Shadow Style=[none,texture_additive,texture_modulative,stencil_additive,stencil_modulaive]"),
                           shadowFarDistance=new OptionValue("shadowfar","1000",OptionValueType<float32>(),"The distance away a shadowcaster may hide the light"),
                           mParallaxSteps=new OptionValue("parallax-steps","1.0",OptionValueType<float>(),"Multiplies the per-material parallax steps by this constant (default 1.0)"),
                           mParallaxShadowSteps=new OptionValue("parallax-shadow-steps","10",OptionValueType<int>(),"Total number of steps for shadow parallax mapping (default 10)"),
                           new OptionValue("nearplane",".125",OptionValueType<float32>(),"The min distance away you can see"),
                           new OptionValue("farplane","5000",OptionValueType<float32>(),"The max distance away you can see"),
                           mModelLights = new OptionValue("model-lights","false",OptionValueType<bool>(),"Whether to use a base set of lights or load lights dynamically from loaded models."),
                           backColor = new OptionValue("back-color","<.71,.785,.91,1>",OptionValueType<Vector4f>(),"Background color to clear render viewport to."),

                           NULL);
    bool userAccepted=true;

    (mOptions=OptionSet::getOptions("ogregraphics",this))->parse(options);

    mBackgroundColor = backColor->as<Vector4f>();

    // Initialize this first so we can get it to not spit out to stderr
    Ogre::LogManager * lm = OGRE_NEW Ogre::LogManager();
    lm->createLog(ogreLogFile->as<String>(), true, false, false);

    static bool success=((sRoot=OGRE_NEW Ogre::Root(pluginFile->as<String>(),configFile->as<String>(),ogreLogFile->as<String>()))!=NULL
                         &&loadBuiltinPlugins()
                         &&((purgeConfig->as<bool>()==false&&getRoot()->restoreConfig())
                            || (userAccepted=getRoot()->showConfigDialog())));
    if (userAccepted&&success) {
        if (!getRoot()->isInitialised()) {
            bool doAutoWindow=
#if defined(_WIN32)
                true
#else
                false
#endif
                ;

            sRoot->initialise(doAutoWindow,windowTitle->as<String>());
            Ogre::RenderWindow *rw=(doAutoWindow?sRoot->getAutoCreatedWindow():NULL);
            mTransferPool = Transfer::TransferMediator::getSingleton().registerClient("OgreGraphics");

            mCDNArchivePlugin = new CDNArchivePlugin;
            sRoot->installPlugin(&*mCDNArchivePlugin);
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation("", "CDN", "General");
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(getOgreResourcesDir(), "FileSystem", "General");
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(".", "FileSystem", "General");//FIXME get rid of this line of code: we don't want to load resources from $PWD

            Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups(); /// Although t    //just to test if the cam is setup ok ==>
                                                                                      /// setupResources("/home/daniel/clipmapterrain/trunk/resources.cfg");
            bool ogreCreatedWindow=
#if defined(_WIN32) || defined(__APPLE__)
                true
#else
                doAutoWindow
#endif
                ;
#ifdef __APPLE__x
            if (system("/usr/bin/sw_vers|/usr/bin/grep ProductVersion:.10.4.")==0)
                ogreCreatedWindow=false;
#endif
            void* hWnd=NULL;
            if (ogreCreatedWindow) {
                if (!rw) {
                    Ogre::NameValuePairList misc;
#ifdef __APPLE__
                    if (mFullScreen->as<bool>()==false) {//does not work in fullscreen
                        misc["macAPI"] = String("cocoa");
                    }
#endif
                    sRenderTarget=mRenderTarget=static_cast<Ogre::RenderTarget*>(rw=getRoot()->createRenderWindow(windowTitle->as<String>(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>(),&misc));
                    rw->setVisible(true);
                }
                printf("RW: %s\n", typeid(*rw).name());
                rw->getCustomAttribute("WINDOW",&hWnd);
#ifdef _WIN32
                {
                    char tmp[64];
                    sprintf(tmp, "SDL_WINDOWID=%u", (unsigned long)hWnd);
                    _putenv(tmp);
                }
#endif
                SILOG(ogre,warning,"Setting window width from "<<mWindowWidth->as<uint32>()<< " to "<<rw->getWidth()<<'\n'<<"Setting window height from "<<mWindowHeight->as<uint32>()<< " to "<<rw->getHeight()<<'\n');
                *mWindowWidth->get()=Any(rw->getWidth());
                *mWindowHeight->get()=Any(rw->getHeight());
                mInputManager=new SDLInputManager(rw->getWidth(),
                                                  rw->getHeight(),
                                                  mFullScreen->as<bool>(),
                                                  mWindowDepth->as<Ogre::PixelFormat>(),
                                                  grabCursor->as<bool>(),
                          						  hWnd);
            }else {
                mInputManager=new SDLInputManager(mWindowWidth->as<uint32>(),
                                                  mWindowHeight->as<uint32>(),
                                                  mFullScreen->as<bool>(),
                                                  mWindowDepth->as<Ogre::PixelFormat>(),
                                                  grabCursor->as<bool>(),
                                                  hWnd);
                Ogre::NameValuePairList misc;
#ifdef __APPLE__
                {
                    if (mFullScreen->as<bool>()==false) {//does not work in fullscreen
                        misc["macAPI"] = String("cocoa");
                        //misc["macAPICocoaUseNSView"] = String("true");
//                        misc["externalWindowHandle"] = Ogre::StringConverter::toString((size_t)hWnd);
                        misc["currentGLContext"] = String("True");
                    }
                }
#else
                misc["currentGLContext"] = String("True");
#endif
                sRenderTarget=mRenderTarget=static_cast<Ogre::RenderTarget*>(rw=getRoot()->createRenderWindow(windowTitle->as<String>(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>(),&misc));
                SILOG(ogre,warning,"Setting window width from "<<mWindowWidth->as<uint32>()<< " to "<<rw->getWidth()<<'\n'<<"Setting window height from "<<mWindowHeight->as<uint32>()<< " to "<<rw->getHeight()<<'\n');
                *mWindowWidth->get()=Any(rw->getWidth());
                *mWindowHeight->get()=Any(rw->getHeight());
                rw->setVisible(true);

            }
            sRenderTarget=mRenderTarget=rw;

        } else if (createWindow->as<bool>()) {
                Ogre::RenderWindow *rw;
                //mRenderTarget=rw=sRoot->createRenderWindow(UUID::random().rawHexData(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>());
                mRenderTarget=rw=sRoot->createRenderWindow(windowTitle->as<String>(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>());
                rw->setVisible(true);
                if (sRenderTarget==NULL)
                    sRenderTarget=mRenderTarget;
        }else {
            mRenderTarget=createRenderTarget(windowTitle->as<String>(),
                                             mWindowWidth->as<uint32>(),
                                             mWindowHeight->as<uint32>(),
                                             renderBufferAutoMipmap->as<bool>(),
                                             mWindowDepth->as<Ogre::PixelFormat>());
        }
    }
    if (!getRoot()->isInitialised()) {
        return false;
    }
    try {
        mSceneManager=getRoot()->createSceneManager(ogreSceneManager->as<String>());
    }catch (Ogre::Exception &e) {
        if (e.getNumber()==Ogre::Exception::ERR_ITEM_NOT_FOUND) {
            SILOG(ogre,warning,"Cannot find ogre scene manager: "<<ogreSceneManager->as<String>());
            getRoot()->createSceneManager(0);
        } else
            throw e;
    }
    mSceneManager->setShadowTechnique(shadowTechnique->as<Ogre::ShadowTechnique>());
    mSceneManager->setShadowFarDistance(shadowFarDistance->as<float32>());
    mSceneManager->setAmbientLight(Ogre::ColourValue(0.0,0.0,0.0,0));
    sActiveOgreScenes.push_back(this);

    allocMouseHandler(keybindingFile->as<String>());
    new WebViewManager(0, mInputManager, getBerkeliumBinaryDir(), getOgreResourcesDir());

    loadSystemLights();

  // Test web view
/*
    WebView* view = WebViewManager::getSingleton().createWebView(UUID::random().rawHexData(), UUID::random().rawHexData(), 400, 300, OverlayPosition());
    //view->setProxyObject(webviewpxy);
    view->loadURL("http://www.google.com");
*/
    //finish instantiation here
    instantiateAllObjects(proxyManager);

    return true;
}
namespace {
bool ogreLoadPlugin(const String& _filename, const String& root = "") {
    using namespace boost::filesystem;

    String filename = _filename;
#if SIRIKATA_PLATFORM == PLATFORM_MAC
    filename += ".dylib";
#endif

    // FIXME there probably need to be more of these
    // The current two reflect what we'd expect for installed
    // and what's in the source tree.
    path search_paths[] = {
        path(".") / filename,
        path("dependencies/ogre-1.6.1/lib") / filename,
        path("dependencies/ogre-1.6.x/lib") / filename,
        path("dependencies/ogre-1.6.1/lib/OGRE") / filename,
        path("dependencies/ogre-1.6.x/lib/OGRE") / filename,
        path("dependencies/lib/OGRE") / filename,
        path("dependencies/installed-ogre/OgrePlugins") / filename, // Mac
        path("lib/OGRE") / filename,
        path("OGRE") / filename,
        path("Debug") / filename,
        path("Release") / filename,
        path("MinSizeRel") / filename,
        path("RelWithDebInfo") / filename,
        path("/usr/local/lib/OGRE") / filename,
        path("/usr/lib/OGRE") / filename
    };
    uint32 nsearch_paths = sizeof(search_paths)/sizeof(*search_paths);

    path not_found;
    path plugin_path = path(findResource(search_paths, nsearch_paths, false, root, not_found));
    if (plugin_path == not_found)
        return false;

    String plugin_str = plugin_path.string();

    FILE *fp=fopen(plugin_str.c_str(),"rb");
    if (fp) {
        fclose(fp);
        Ogre::Root::getSingleton().loadPlugin(plugin_str);
        return true;
    }
    return false;
}
}
bool OgreSystem::loadBuiltinPlugins () {
    bool retval = true;
    String exeDir = mOgreRootDir->as<String>();
    if (exeDir.empty())
        exeDir = DynamicLibrary::GetExecutablePath();

#ifdef __APPLE__
    retval = ogreLoadPlugin("RenderSystem_GL", exeDir);
    //retval = ogreLoadPlugin("Plugin_CgProgramManager", exeDir) && retval;
    retval = ogreLoadPlugin("Plugin_ParticleFX", exeDir) && retval;
    retval = ogreLoadPlugin("Plugin_OctreeSceneManager", exeDir) && retval;
	if (!retval) {
		SILOG(ogre,error,"The required ogre plugins failed to load. Check that all .dylib files named RenderSystem_* and Plugin_* are copied to the current directory.");
	}
#else
#ifdef _WIN32
#ifdef NDEBUG
   #define OGRE_DEBUG_MACRO ".dll"
#else
   #define OGRE_DEBUG_MACRO "_d.dll"
#endif
#else
#ifdef NDEBUG
   #define OGRE_DEBUG_MACRO ".so"
#else
   #define OGRE_DEBUG_MACRO ".so"
#endif
#endif
        retval = ogreLoadPlugin("RenderSystem_GL" OGRE_DEBUG_MACRO, exeDir);
#ifdef _WIN32
	try {
	    retval=ogreLoadPlugin("RenderSystem_Direct3D9" OGRE_DEBUG_MACRO, exeDir) || retval;
	} catch (Ogre::InternalErrorException) {
		SILOG(ogre,warn,"Received an Internal Error when loading the Direct3D9 plugin, falling back to OpenGL. Check that you have the latest version of DirectX installed from microsoft.com/directx");
	}
#endif
    retval=ogreLoadPlugin("Plugin_CgProgramManager" OGRE_DEBUG_MACRO, exeDir) && retval;
    retval=ogreLoadPlugin("Plugin_ParticleFX" OGRE_DEBUG_MACRO, exeDir) && retval;
    retval=ogreLoadPlugin("Plugin_OctreeSceneManager" OGRE_DEBUG_MACRO, exeDir) && retval;
	if (!retval) {
		SILOG(ogre,error,"The required ogre plugins failed to load. Check that all DLLs named RenderSystem_* and Plugin_* are copied to the current directory.");
	}
#undef OGRE_DEBUG_MACRO
#endif
    return retval;
    /*
    sCDNArchivePlugin=new CDNArchivePlugin;
    getRoot()->installPlugin(&*sCDNArchivePlugin);
    */
}
Ogre::SceneManager* OgreSystem::getSceneManager(){
    return mSceneManager;
}
Ogre::Root*OgreSystem::getRoot() {
    return &Ogre::Root::getSingleton();
}
Ogre::RenderTarget*OgreSystem::getRenderTarget() {
    return mRenderTarget;
}
OgreSystem::~OgreSystem() {
    mParsingThread->join();
    delete mParsingThread;
    Network::IOServiceFactory::destroyIOService(mParsingIOService);

    {
        SceneEntitiesMap toDelete;
        toDelete.swap(mSceneEntities);
        SceneEntitiesMap::iterator iter;
        for (iter = toDelete.begin(); iter != toDelete.end(); ++iter) {
            Entity* current = (*iter).second;
            delete current;
        }
    }
    if (mSceneManager) {
        Ogre::Root::getSingleton().destroySceneManager(mSceneManager);
    }
    decrefcount();
    for (std::list<OgreSystem*>::iterator iter=sActiveOgreScenes.begin()
             ;iter!=sActiveOgreScenes.end();) {
        if (*iter==this) {
            sActiveOgreScenes.erase(iter++);
            break;
        }else ++iter;
        assert(iter!=sActiveOgreScenes.end());
    }
    --sNumOgreSystems;
    if (sNumOgreSystems==0) {
        OGRE_DELETE sCDNArchivePlugin;
        sCDNArchivePlugin=NULL;
        OGRE_DELETE sRoot;
        sRoot=NULL;
    }
    destroyMouseHandler();
    delete mInputManager;

    delete mModelParser;
}

static void KillWebView(OgreSystem*ogreSystem,ProxyObjectPtr p) {
    SILOG(ogre,detailed,"Killing WebView!");
    p->getProxyManager()->destroyObject(p);
}

bool OgreSystem::useModelLights() const {
    return mModelLights->as<bool>();
}

void OgreSystem::onCreateProxy(ProxyObjectPtr p)
{
    bool created = false;

    Entity* mesh = new Entity(this,p);
    dlPlanner->addNewObject(p,mesh);

    bool is_viewer = (p->getObjectReference() == mPresenceID);
    if (is_viewer)
    {
        assert(mPrimaryCamera == NULL);
        mPrimaryCamera = new Camera(this, mesh);
        mPrimaryCamera->attach("", 0, 0, mBackgroundColor);
        attachCamera("", mPrimaryCamera);
        // for now, always hide the original entity. In the future, this should
        // be controlled based on the type of view we have (1st vs 3rd person).
        mesh->setVisible(false);
    }
}

void OgreSystem::onDestroyProxy(ProxyObjectPtr p)
{
    dlPlanner->removeObject(p);
}

void OgreSystem::parseMesh(const Transfer::URI& orig_uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, ParseMeshCallback cb) {
    mParsingIOService->post(
        std::tr1::bind(&OgreSystem::parseMeshWork, this, orig_uri, fp, data, cb)
    );
}
void OgreSystem::parseMeshWork(const Transfer::URI& orig_uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, ParseMeshCallback cb) {
    Mesh::MeshdataPtr parsed = mModelParser->load(orig_uri, fp, data);
    mContext->mainStrand->post(std::tr1::bind(cb, parsed));
}

struct RayTraceResult {
    Ogre::Real mDistance;
    Ogre::MovableObject *mMovableObject;
    IntersectResult mResult;
    int mSubMesh;
    RayTraceResult() { mDistance=3.0e38f;mMovableObject=NULL;mSubMesh=-1;}
    RayTraceResult(Ogre::Real distance,
                   Ogre::MovableObject *moveableObject) {
        mDistance=distance;
        mMovableObject=moveableObject;
    }
    bool operator<(const RayTraceResult&other)const {
        if (mDistance==other.mDistance) {
            return mMovableObject<other.mMovableObject;
        }
        return mDistance<other.mDistance;
    }
    bool operator==(const RayTraceResult&other)const {
        return mDistance==other.mDistance&&mMovableObject==other.mMovableObject;
    }
};

Entity* OgreSystem::rayTrace(const Vector3d &position,
                             const Vector3f &direction,
                             int&resultCount,
                             double &returnResult,
                             Vector3f&returnNormal,
                             int&subent,
                             int which) const{
    Ogre::Ray traceFrom(toOgre(position, getOffset()), toOgre(direction));
    return internalRayTrace(traceFrom,false,resultCount,returnResult,returnNormal, subent,NULL,false,which);
}
Entity* OgreSystem::rayTraceAABB(const Vector3d &position,
                     const Vector3f &direction,
                     int&resultCount,
                     double &returnResult,
                     int&subent,
                     int which) const{
    Vector3f normal;
    Ogre::Ray traceFrom(toOgre(position, getOffset()), toOgre(direction));
    return internalRayTrace(traceFrom,true,resultCount,returnResult,normal,subent,NULL,false,which);
}
bool OgreSystem::queryRay(const Vector3d&position,
                          const Vector3f&direction,
                          const double maxDistance,
                          ProxyObjectPtr ignore,
                          double &returnDistance,
                          Vector3f &returnNormal,
                          SpaceObjectReference &returnName) {
    int resultCount=0;
    int subent;
    Ogre::Ray traceFrom(toOgre(position, getOffset()), toOgre(direction));
    Entity * retval=internalRayTrace(traceFrom,false,resultCount,returnDistance,returnNormal,subent,NULL,false,0);
    if (retval != NULL) {
        returnName= retval->getProxy().getObjectReference();
        return true;
    }
    return false;
}
Entity *OgreSystem::internalRayTrace(const Ogre::Ray &traceFrom, bool aabbOnly,int&resultCount,double &returnresult, Vector3f&returnNormal, int& returnSubMesh, IntersectResult *intersectResult, bool texcoord, int which) const {
    Ogre::RaySceneQuery* mRayQuery;
    mRayQuery = mSceneManager->createRayQuery(Ogre::Ray(), Ogre::SceneManager::WORLD_GEOMETRY_TYPE_MASK);
    mRayQuery->setRay(traceFrom);
    mRayQuery->setSortByDistance(aabbOnly);
    const Ogre::RaySceneQueryResult& resultList = mRayQuery->execute();

    Entity *toReturn = NULL;
    returnresult = 0;
    int count = 0;
    std::vector<RayTraceResult> fineGrainedResults;
    for (Ogre::RaySceneQueryResult::const_iterator iter  = resultList.begin();
         iter != resultList.end(); ++iter) {
        const Ogre::RaySceneQueryResultEntry &result = (*iter);
        Ogre::Entity *foundEntity = dynamic_cast<Ogre::Entity*>(result.movable);
        if (!foundEntity) continue;
        Entity *ourEntity = Entity::fromMovableObject(result.movable);
        if (!ourEntity) continue;
        if (ourEntity->id() == mPresenceID) continue;

        RayTraceResult rtr(result.distance,result.movable);
        bool passed=aabbOnly&&result.distance > 0;
        if (aabbOnly==false) {
            rtr.mDistance=3.0e38f;
            Ogre::Ray meshRay = OgreMesh::transformRay(ourEntity->getSceneNode(), traceFrom);
            Ogre::Mesh *mesh = foundEntity->getMesh().get();
            uint16 numSubMeshes = mesh->getNumSubMeshes();
            std::vector<TriVertex> sharedVertices;
            for (uint16 ndx = 0; ndx < numSubMeshes; ndx++) {
                Ogre::SubMesh *submesh = mesh->getSubMesh(ndx);
                OgreMesh ogreMesh(submesh, texcoord, sharedVertices);
                IntersectResult intRes;
                ogreMesh.intersect(ourEntity->getSceneNode(), meshRay, intRes);
                if (intRes.intersected && intRes.distance < rtr.mDistance && intRes.distance > 0 ) {
                    rtr.mResult = intRes;
                    rtr.mMovableObject = result.movable;
                    rtr.mDistance=intRes.distance;
                    rtr.mSubMesh=ndx;
                    passed=true;
                }
            }
        }
        if (passed) {
            fineGrainedResults.push_back(rtr);
            ++count;
        }
    }
    if (!aabbOnly) {
        std::sort(fineGrainedResults.begin(),fineGrainedResults.end());
    }
    if (count > 0) {
        which %= count;
        if (which < 0) {
            which += count;
        }
        for (std::vector<RayTraceResult>::const_iterator iter  = fineGrainedResults.begin()+which,iterEnd=fineGrainedResults.end();
             iter != iterEnd; ++iter) {
            const RayTraceResult &result = (*iter);
            Entity *foundEntity = Entity::fromMovableObject(result.mMovableObject);
            if (foundEntity) {
                toReturn = foundEntity;
                returnresult = result.mDistance;
                returnNormal=result.mResult.normal;
                returnSubMesh=result.mSubMesh;
                if(intersectResult)*intersectResult=result.mResult;
                break;
            }
        }
    }
    mRayQuery->clearResults();
    if (mRayQuery) {
        mSceneManager->destroyQuery(mRayQuery);
    }
    resultCount=count;
    return toReturn;
}

Duration OgreSystem::desiredTickRate()const{
    return mFrameDuration->as<Duration>();
}

bool OgreSystem::renderOneFrame(Task::LocalTime curFrameTime, Duration deltaTime) {
    for (std::list<OgreSystem*>::iterator iter=sActiveOgreScenes.begin();iter!=sActiveOgreScenes.end();) {
        (*iter++)->preFrame(curFrameTime, deltaTime);
    }
    Ogre::WindowEventUtilities::messagePump();
//    if (mPrimaryCamera) {
    if (!mSuspended) {
        Ogre::Root::getSingleton().renderOneFrame();
    }
//    }
    Task::LocalTime postFrameTime = Task::LocalTime::now();
    Duration postFrameDelta = postFrameTime-mLastFrameTime;
    bool continueRendering=mInputManager->tick(postFrameTime,postFrameDelta);
    for (std::list<OgreSystem*>::iterator iter=sActiveOgreScenes.begin();iter!=sActiveOgreScenes.end();) {
        (*iter++)->postFrame(postFrameTime, postFrameDelta);
    }

    static int counter=0;
    counter++;

    if(WebViewManager::getSingletonPtr())
    {
        // HACK: WebViewManager is static, but points to a RenderTarget! If OgreSystem dies, we will crash.
        static bool webViewInitialized = false;
        if(!webViewInitialized) {
            if (mPrimaryCamera) {
                WebViewManager::getSingleton().setDefaultViewport(mPrimaryCamera->getViewport());
                webViewInitialized = true;
            }
            // else, keep waiting for a camera to appear (may require connecting to a space).
        }
        if (webViewInitialized) {
            WebViewManager::getSingleton().Update();
        }
    }

    return continueRendering;
}
//static Task::LocalTime debugStartTime = Task::LocalTime::now();
void OgreSystem::poll(){
    Task::LocalTime curFrameTime(Task::LocalTime::now());
    Task::LocalTime finishTime(curFrameTime + desiredTickRate()); // arbitrary

    tickInputHandler(curFrameTime);

    Duration frameTime=curFrameTime-mLastFrameTime;
    if (mRenderTarget==sRenderTarget) {
        if (!renderOneFrame(curFrameTime, frameTime))
            quit();
    }
    else if (sRenderTarget==NULL) {
        SILOG(ogre,warning,"No window set to render: skipping render phase");
    }
    mLastFrameTime=curFrameTime;//reevaluate Time::now()?

    if (mQuitRequested && !mQuitRequestHandled) {
        mContext->shutdown();
        mQuitRequestHandled = true;
    }
}
void OgreSystem::stop() {
    delete mParsingWork;
    TimeSteppedQueryableSimulation::stop();
}
void OgreSystem::preFrame(Task::LocalTime currentTime, Duration frameTime) {
    std::list<Entity*>::iterator iter;
    for (iter = mMovingEntities.begin(); iter != mMovingEntities.end();) {
        Entity *current = *iter;
        ++iter;
        SpaceID space(current->getProxy().getObjectReference().space());
        Time cur_time = simTime();
        current->extrapolateLocation(cur_time);
    }
    for(std::tr1::unordered_set<Camera*>::iterator cam_it = mAttachedCameras.begin(); cam_it != mAttachedCameras.end(); cam_it++) {
        Camera* cam = *cam_it;
        cam->tick();
    }
}

void OgreSystem::postFrame(Task::LocalTime current, Duration frameTime) {
    Ogre::FrameEvent evt;
    evt.timeSinceLastEvent=frameTime.toMicroseconds()*1000000.;
    evt.timeSinceLastFrame=frameTime.toMicroseconds()*1000000.;
    if (mCubeMap) {
        mCubeMap->frameEnded(evt);
    }

}

void OgreSystem::screenshot(const String& filename) {
    if (mRenderTarget != NULL)
        mRenderTarget->writeContentsToFile(filename);
}

// ConnectionEventListener Interface
void OgreSystem::onConnected(const Network::Address& addr)
{
}

void OgreSystem::onDisconnected(const Network::Address& addr, bool requested, const String& reason) {
    if (!requested) {
        SILOG(ogre,fatal,"Got disconnected from space server: " << reason);
        quit(); // FIXME
    }
    else
        SILOG(ogre,warn,"Disconnected from space server.");
}

void OgreSystem::onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name) {
    mViewer->removeListener((SessionEventListener*)this);
    SILOG(ogre,info,"Got disconnected from space server.");
    mMouseHandler->alert("Disconnected", "Lost connection to space server...");
}


void OgreSystem::allocMouseHandler(const String& keybindings_file) {
    mMouseHandler = new OgreSystemMouseHandler(this, keybindings_file);
}
void OgreSystem::destroyMouseHandler() {
    if (mMouseHandler) {
        delete mMouseHandler;
    }
}

void OgreSystem::selectObject(Entity *obj, bool replace) {
    if (replace) {
        mMouseHandler->setParentGroupAndClear(obj->getProxy().getParentProxy()->getObjectReference());
    }
    if (mMouseHandler->getParentGroup() == obj->getProxy().getParentProxy()->getObjectReference()) {
        mMouseHandler->addToSelection(obj->getProxyPtr());
        obj->setSelected(true);
    }
}

void OgreSystem::tickInputHandler(const Task::LocalTime& t) const {
    if (mMouseHandler != NULL)
        mMouseHandler->tick(t);
}


namespace {

bool anyIsString(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(String));
}

String anyAsString(const boost::any& a) {
    return boost::any_cast<String>(a);
}


bool anyIsInvokable(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(Invokable*));
}

Invokable* anyAsInvokable(const boost::any& a) {
    return boost::any_cast<Invokable*>(a);
}

} // namespace

boost::any OgreSystem::invoke(vector<boost::any>& params)
{
    // Decode the command. First argument is the "function name"
    if (params.empty() || !anyIsString(params[0]))
        return NULL;

    string name = anyAsString(params[0]);
    SILOG(ogre,detailed,"Invoking the function " << name);

    if(name == "createWindow")
        return createWindow(params);
    else if(name == "createWindowHTML")
        return createWindowHTML(params);
    else if(name == "setInputHandler")
        return setInputHandler(params);

    return NULL;
}

boost::any OgreSystem::createWindow(const String& window_name, bool is_html, String content) {
    WebViewManager* wvManager = WebViewManager::getSingletonPtr();
    WebView* ui_wv = wvManager->getWebView(window_name);
    if(!ui_wv)
    {
        ui_wv = wvManager->createWebView(window_name, window_name, 300, 300, OverlayPosition(RP_TOPLEFT));
        if (is_html)
            ui_wv->loadHTML(content);
        else
            ui_wv->loadFile(content);
    }
    Invokable* inn = ui_wv;
    boost::any result(inn);
    return result;
}

boost::any OgreSystem::createWindow(vector<boost::any>& params) {
    // Create a window using the specified url
    if (params.size() < 3) return NULL;
    if (!anyIsString(params[1]) || !anyIsString(params[2])) return NULL;

    String window_name = anyAsString(params[1]);
    String html_url = anyAsString(params[2]);

    return createWindow(window_name, false, html_url);
}

boost::any OgreSystem::createWindowHTML(vector<boost::any>& params) {
    // Create a window using the specified HTML content
    if (params.size() < 3) return NULL;
    if (!anyIsString(params[1]) || !anyIsString(params[2])) return NULL;

    String window_name = anyAsString(params[1]);
    String html_script = anyAsString(params[2]);

    return createWindow(window_name, true, html_script);
}

boost::any OgreSystem::setInputHandler(vector<boost::any>& params) {
    if (params.size() < 2) return NULL;
    if (!anyIsInvokable(params[1])) return NULL;

    Invokable* handler = anyAsInvokable(params[1]);

}

}
}
