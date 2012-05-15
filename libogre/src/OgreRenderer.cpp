/*  Sirikata
 *  Ogre.cpp
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
#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include <sirikata/mesh/Filter.hpp>
#include <sirikata/mesh/CompositeFilter.hpp>
#include <sirikata/mesh/Bounds.hpp>

#include <sirikata/ogre/Camera.hpp>
#include <sirikata/ogre/Entity.hpp>

#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/DynamicLibrary.hpp>
#include <sirikata/core/transfer/AggregatedTransferPool.hpp>

#include <sirikata/ogre/resourceManager/CDNArchivePlugin.hpp>

#include <sirikata/ogre/input/SDLInputManager.hpp>

#include <sirikata/core/util/Paths.hpp>
#include <boost/filesystem.hpp>

#include "OgreRoot.h"
#include "OgreHardwarePixelBuffer.h"

#include <sirikata/ogre/Lights.hpp>

//#include </Developer/SDKs/MacOSX10.4u.sdk/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/HIView.h>
#include <sirikata/ogre/WebViewManager.hpp>

#include "ResourceLoader.hpp"
#include "PriorityDownloadPlanner.hpp"


//volatile char assert_thread_support_is_gequal_2[OGRE_THREAD_SUPPORT*2-3]={0};
//volatile char assert_thread_support_is_lequal_2[5-OGRE_THREAD_SUPPORT*2]={0};
//enable the below when NEDMALLOC is turned off, so we can verify that NEDMALLOC is off
//volatile char assert_malloc_is_gequal_1[OGRE_MEMORY_ALLOCATOR*2-1]={0};
//volatile char assert_malloc_is_lequal_1[3-OGRE_MEMORY_ALLOCATOR*2]={0};

namespace Sirikata {
namespace Graphics {

Ogre::Root *OgreRenderer::sRoot;
CDNArchivePlugin *OgreRenderer::mCDNArchivePlugin=NULL;
Ogre::RenderTarget* OgreRenderer::sRenderTarget=NULL;
Ogre::Plugin*OgreRenderer::sCDNArchivePlugin=NULL;
std::list<OgreRenderer*> OgreRenderer::sActiveOgreScenes;
uint32 OgreRenderer::sNumOgreSystems=0;


namespace {

// This is a generic search method. It searches upwards from the
// directory containing the executable for any of the specified files
// and returns the first path it finds that contains one of them.
std::string findResource(boost::filesystem::path* search_paths, uint32 nsearch_paths, const std::vector<String>&searchPoints, bool want_dir = true, const std::string& start_path = ".", boost::filesystem::path default_ = boost::filesystem::complete(boost::filesystem::path("."))) {
    using namespace boost::filesystem;

    path start_path_path = boost::filesystem::complete(start_path);

    for(size_t offset = 0; offset < searchPoints.size(); offset++) {
        for(uint32 spath = 0; spath < nsearch_paths; spath++) {
            path full = start_path_path / searchPoints[offset] / search_paths[spath];
            if (exists(full) && (!want_dir || is_directory(full)))
                return full.string();
        }
    }

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

std::string getOgreResourcesDir(const std::vector<String>&searchPoints) {
    using namespace boost::filesystem;

    // FIXME there probably need to be more of these
    // The current two reflect what we'd expect for installed
    // and what's in the source tree.
    path search_paths[] = {
        path("ogre/data"),
        path("share/sirikata/ogre/data"),
        path("liboh/plugins/ogre/data")
    };
    uint32 nsearch_paths = sizeof(search_paths)/sizeof(*search_paths);

    String exe_dir = Path::Get(Path::DIR_EXE);
    return findResource(search_paths, nsearch_paths, searchPoints, true, exe_dir);
}

std::string getBerkeliumBinaryDir(const std::vector<String>&searchPoints) {
    using namespace boost::filesystem;

    // FIXME there probably need to be more of these
    // The current two reflect what we'd expect for installed
    // and what's in the source tree.
    path search_paths[] = {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
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

    String exe_dir = Path::Get(Path::DIR_EXE);
    return findResource(search_paths, nsearch_paths, searchPoints, true, exe_dir);
}

std::string getChromeResourcesDir(const std::vector<String>&searchPoints) {
    using namespace boost::filesystem;

    return (path(getOgreResourcesDir(searchPoints)) / "chrome").string();
}

} // namespace

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

} // namespace



OgreRenderer::OgreRenderer(Context* ctx,Network::IOStrandPtr sStrand)
 //note use of .get when passing iostrand pointer.  we need to insure that the
 //lifetime of sstrand is greater than the polling service.  from emerson, this
 //should be feasible if delete simulations before shut down an object.  Be very
 //careful here though.
 : TimeSteppedSimulation(ctx, Duration::seconds(1.f/60.f),
     "Ogre Graphics", sStrand.get(), "Ogre Graphics", true),
   simStrand(sStrand),
   mContext(ctx),
   mQuitRequested(false),
   mQuitRequestHandled(false),
   mSuspended(false),
   mFloatingPointOffset(0,0,0),
   mLastFrameTime(Task::LocalTime::now()),
   mOnTickCallback(NULL),
   mParserProfiler(ctx->profiler->addStage("Ogre Renderer Mesh Parsing")),
   mModelParser( ModelsSystemFactory::getSingleton ().getConstructor ( "any" ) ( "" ) ),
   mDownloadPlanner(NULL),
   mNextFrameScreenshotFile(""),
   initialized(false),
   stopped(false)
{
    try {
        // These have to be consistent with any other simulations -- e.g. the
        // space bullet plugin and scripting plugins that expose mesh data
        std::vector<String> names_and_args;
        names_and_args.push_back("triangulate"); names_and_args.push_back("all");
        names_and_args.push_back("compute-normals"); names_and_args.push_back("");
        names_and_args.push_back("reduce-draw-calls"); names_and_args.push_back("");
        mModelFilter = new Mesh::CompositeFilter(names_and_args);

        names_and_args.clear();
        names_and_args.push_back("center"); names_and_args.push_back("");
        mCenteringFilter = new Mesh::CompositeFilter(names_and_args);
    }
    catch(Mesh::CompositeFilter::Exception e) {
        SILOG(ogre,warning,"Couldn't allocate requested model load filter, will not apply filter to loaded models.");
        mModelFilter = NULL;
        mCenteringFilter = NULL;
    }
}

bool OgreRenderer::initialize(const String& options, bool with_berkelium) {
    ++sNumOgreSystems;

    mParsingIOService = new Network::IOService("Ogre Mesh Parsing");
    mParsingWork = new Network::IOWork(*mParsingIOService, "Ogre Mesh Parsing");
    mParsingThread = new Sirikata::Thread("OgreRenderer Model Parsing", std::tr1::bind(&Network::IOService::runNoReturn, mParsingIOService));

    //add ogre system options here
    OptionValue*pluginFile;
    OptionValue*configFile;
    OptionValue*ogreLogFile;
    OptionValue*purgeConfig;
    OptionValue*createWindow;
    OptionValue*ogreSceneManager;
    OptionValue*windowTitle;
    OptionValue* frameLoadDuration;
    OptionValue*shadowTechnique;
    OptionValue*shadowFarDistance;
    OptionValue*renderBufferAutoMipmap;
    OptionValue*grabCursor;
    OptionValue* backColor;
    OptionValue *searchPaths;
    InitializeClassOptions("ogregraphics",this,
                           pluginFile=new OptionValue("pluginfile","",OptionValueType<String>(),"sets the file ogre should read options from."),
                           configFile=new OptionValue("configfile","ogre.cfg",OptionValueType<String>(),"sets the ogre config file for config options"),
                           ogreLogFile=new OptionValue("logfile","Ogre.log",OptionValueType<String>(),"sets the ogre log file"),
                           purgeConfig=new OptionValue("purgeconfig","false",OptionValueType<bool>(),"Pops up the dialog asking for the screen resolution no matter what"),
                           createWindow=new OptionValue("window","true",OptionValueType<bool>(),"Render to a onscreen window"),
                           grabCursor=new OptionValue("grabcursor","false",OptionValueType<bool>(),"Grab cursor"),
                           windowTitle=new OptionValue("windowtitle","Sirikata",OptionValueType<String>(),"Window title name"),
                           mOgreRootDir=new OptionValue("ogretoplevel","",OptionValueType<String>(),"Directory with ogre plugins"),
                           ogreSceneManager=new OptionValue("scenemanager","OctreeSceneManager",OptionValueType<String>(),"Which scene manager to use to arrange objects"),
                           mWindowWidth=new OptionValue("windowwidth","1024",OptionValueType<uint32>(),"Window width"),
                           mFullScreen=new OptionValue("fullscreen","false",OptionValueType<bool>(),"Fullscreen"),
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
#define SIRIKATA_OGRE_DEFAULT_WINDOW_HEIGHT "600"
#else
#define SIRIKATA_OGRE_DEFAULT_WINDOW_HEIGHT "768"
#endif
                           mWindowHeight=new OptionValue("windowheight",SIRIKATA_OGRE_DEFAULT_WINDOW_HEIGHT,OptionValueType<uint32>(),"Window height"),
                           mWindowDepth=new OptionValue("colordepth","8a",OgrePixelFormatParser(),"Pixel color depth"),
                           renderBufferAutoMipmap=new OptionValue("rendertargetautomipmap","false",OptionValueType<bool>(),"If the render target needs auto mipmaps generated"),
                           frameLoadDuration=new OptionValue("load-duration","1ms",OptionValueType<Duration>(),"Amount of time to spend loading resources per frame. Keep low to maintain good frame rates."),
                           shadowTechnique=new OptionValue("shadows","none",ShadowType(),"Shadow Style=[none,texture_additive,texture_modulative,stencil_additive,stencil_modulaive]"),
                           shadowFarDistance=new OptionValue("shadowfar","1000",OptionValueType<float32>(),"The distance away a shadowcaster may hide the light"),
                           mParallaxSteps=new OptionValue("parallax-steps","1.0",OptionValueType<float>(),"Multiplies the per-material parallax steps by this constant (default 1.0)"),
                           mParallaxShadowSteps=new OptionValue("parallax-shadow-steps","10",OptionValueType<int>(),"Total number of steps for shadow parallax mapping (default 10)"),
                           new OptionValue("nearplane","0.9",OptionValueType<float32>(),"The min distance away you can see"),
                           new OptionValue("farplane","20000",OptionValueType<float32>(),"The max distance away you can see"),
                           searchPaths=new OptionValue("search_path","../..",OptionValueType<String>(),"Colon separated list of places to search for Ogre data (eg ogre/data/chrome/js or ogre/data/chrome/ui)"),
                           mModelLights = new OptionValue("model-lights","false",OptionValueType<bool>(),"Whether to use a base set of lights or load lights dynamically from loaded models."),
                           backColor = new OptionValue("back-color","<.71,.785,.91,1>",OptionValueType<Vector4f>(),"Background color to clear render viewport to."),

                           NULL);
    bool userAccepted=true;
    (mOptions=OptionSet::getOptions("ogregraphics",this))->parse(options);

    String search_path=searchPaths->as<String>();
    if (search_path.length()) {
        while (true) {
            String::size_type where=search_path.find(":");
            if (where==String::npos) {
                mSearchPaths.push_back(search_path);
                break;
            }else {
                mSearchPaths.push_back(search_path.substr(0,where));
                search_path=search_path.substr(where+1);
            }
        }
    }
    mResourcesDir=getOgreResourcesDir(mSearchPaths);

    mBackgroundColor = backColor->as<Vector4f>();

    // Initialize this first so we can get it to not spit out to stderr
    Ogre::LogManager * lm = OGRE_NEW Ogre::LogManager();
    // NOTE: we specifically keep the log file as specified instead of
    // relocating it so logs show up in the expected location and
    // different clients don't overwrite each other's logs if they are
    // run from different locations
    String temp_ogre_path = Path::Get(Path::DIR_TEMP, "ogre");
    // Make sure we have the directories
    boost::filesystem::create_directories( boost::filesystem::path(temp_ogre_path) );
    String ogreLogFilePath = (boost::filesystem::path(temp_ogre_path) / Path::GetTempFilename(ogreLogFile->as<String>() + "-")).string();
    lm->createLog(ogreLogFilePath, true, false, false);

    // NOTE: However, unlike above, we share the config file. This is
    // nice since you only have to configure once.
    std::string ogreConfigFile = Path::Get(Path::DIR_USER_HIDDEN, configFile->as<String>());
    static bool success=((sRoot=OGRE_NEW Ogre::Root(pluginFile->as<String>(),ogreConfigFile,ogreLogFilePath))!=NULL
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
            mRenderWindow = (doAutoWindow?sRoot->getAutoCreatedWindow():NULL);
	    mOgreOwnedRenderWindow = (mRenderWindow != NULL);
            mTransferPool = Transfer::TransferMediator::getSingleton().registerClient<Transfer::AggregatedTransferPool>("OgreGraphics");

            mCDNArchivePlugin = new CDNArchivePlugin;
            sRoot->installPlugin(&*mCDNArchivePlugin);
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation("", "CDN", "General");
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(getOgreResourcesDir(mSearchPaths), "FileSystem", "General");
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
                if (!mRenderWindow) {
                    Ogre::NameValuePairList misc;
#ifdef __APPLE__
                    if (mFullScreen->as<bool>()==false) {//does not work in fullscreen
                        misc["macAPI"] = String("cocoa");
                    }
#endif
                    sRenderTarget=mRenderTarget=static_cast<Ogre::RenderTarget*>(mRenderWindow=getRoot()->createRenderWindow(windowTitle->as<String>(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>(),&misc));
                    mRenderWindow->setVisible(true);
		    mOgreOwnedRenderWindow = (mRenderWindow != NULL);
                }
                printf("RW: %s\n", typeid(*mRenderWindow).name());
                mRenderWindow->getCustomAttribute("WINDOW",&hWnd);
#ifdef _WIN32
                {
                    char tmp[64];
                    sprintf(tmp, "SDL_WINDOWID=%u", (unsigned long)hWnd);
                    _putenv(tmp);
                }
#endif
                SILOG(ogre,warning,"Setting window width from "<<mWindowWidth->as<uint32>()<< " to "<<mRenderWindow->getWidth()<<'\n'<<"Setting window height from "<<mWindowHeight->as<uint32>()<< " to "<<mRenderWindow->getHeight()<<'\n');
                *mWindowWidth->get()=Any(mRenderWindow->getWidth());
                *mWindowHeight->get()=Any(mRenderWindow->getHeight());
                try {
                    mInputManager=new SDLInputManager(this,
                        mRenderWindow->getWidth(),
                        mRenderWindow->getHeight(),
                        mFullScreen->as<bool>(),
                        grabCursor->as<bool>(),
                        hWnd);
                } catch (SDLInputManager::InitializationException exc) {
                    SILOG(ogre,error,exc.what());
                    return false;
                }
            }else {
                try {
                    mInputManager=new SDLInputManager(this,
                        mWindowWidth->as<uint32>(),
                        mWindowHeight->as<uint32>(),
                        mFullScreen->as<bool>(),
                        grabCursor->as<bool>(),
                        hWnd);
                } catch (SDLInputManager::InitializationException exc) {
                    SILOG(ogre,error,exc.what());
                    return false;
                }
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
                sRenderTarget=mRenderTarget=static_cast<Ogre::RenderTarget*>(mRenderWindow=getRoot()->createRenderWindow(windowTitle->as<String>(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>(),&misc));
		mOgreOwnedRenderWindow = false;
                SILOG(ogre,warning,"Setting window width from "<<mWindowWidth->as<uint32>()<< " to "<<mRenderWindow->getWidth()<<'\n'<<"Setting window height from "<<mWindowHeight->as<uint32>()<< " to "<<mRenderWindow->getHeight()<<'\n');
                *mWindowWidth->get()=Any(mRenderWindow->getWidth());
                *mWindowHeight->get()=Any(mRenderWindow->getHeight());
                mRenderWindow->setVisible(true);
            }
            sRenderTarget = mRenderTarget = mRenderWindow;

        } else if (createWindow->as<bool>()) {
                mRenderTarget = mRenderWindow = sRoot->createRenderWindow(windowTitle->as<String>(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>());
                mRenderWindow->setVisible(true);
		mOgreOwnedRenderWindow = (mRenderWindow != NULL);
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
        initialized = true;
        return false;
    }
    if (mRenderWindow != NULL) {
        Ogre::WindowEventUtilities::addWindowEventListener(mRenderWindow, this);
    }
    try {
        mSceneManager=getRoot()->createSceneManager(ogreSceneManager->as<String>());
        mOverlaySceneManager=getRoot()->createSceneManager(Ogre::ST_GENERIC, "overlay");
    }catch (Ogre::Exception &e) {
        if (e.getNumber()==Ogre::Exception::ERR_ITEM_NOT_FOUND) {
            SILOG(ogre,warning,"Cannot find ogre scene manager: "<<ogreSceneManager->as<String>());
            getRoot()->createSceneManager(0);
        } else
            throw e;
    }
    mSceneManager->setShadowTechnique(shadowTechnique->as<Ogre::ShadowTechnique>());
    mSceneManager->setShadowFarDistance(shadowFarDistance->as<float32>());
    mSceneManager->setAmbientLight(Ogre::ColourValue(1.0,1.0,1.0,1.0));
    sActiveOgreScenes.push_back(this);

    mResourceLoader = new ResourceLoader(mContext, frameLoadDuration->as<Duration>());
    PriorityDownloadPlannerMetricPtr metric(new SolidAngleDownloadPlannerMetric());
    mDownloadPlanner = new PriorityDownloadPlanner(mContext, this, metric);

    if (with_berkelium)
        new WebViewManager(0, mInputManager, getBerkeliumBinaryDir(mSearchPaths), getOgreResourcesDir(mSearchPaths));

    loadSystemLights();

    initialized = true;
    return true;
}

namespace {
bool ogreLoadPlugin(const String& _filename, const String& root = "") {
    using namespace boost::filesystem;

    String filename = _filename;
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
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
        path("dependencies/installed-ogre/lib") / filename, // Mac
        path("lib") / filename, // Mac installed
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
    path plugin_path = path(findResource(search_paths, nsearch_paths, std::vector<String>(), false, root, not_found));
    if (plugin_path == not_found)
    {
        return false;
    }

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

bool OgreRenderer::loadBuiltinPlugins () {
    bool retval = true;
    String exeDir = mOgreRootDir->as<String>();
    if (exeDir.empty())
        exeDir = Path::Get(Path::DIR_EXE);

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
        //retval=ogreLoadPlugin("Plugin_CgProgramManager" OGRE_DEBUG_MACRO, exeDir) && retval;
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


OgreRenderer::~OgreRenderer() {
    if (Liveness::livenessAlive())
        Liveness::letDie();

    mParsingThread->join();
    delete mParsingThread;
    delete mParsingIOService;

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
    if (mOverlaySceneManager) {
        Ogre::Root::getSingleton().destroySceneManager(mOverlaySceneManager);
    }

    for (std::list<OgreRenderer*>::iterator iter=sActiveOgreScenes.begin()
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

    delete mInputManager;

    delete mModelFilter;
    delete mCenteringFilter;
    delete mModelParser;
}


Ogre::RenderTarget* OgreRenderer::createRenderTarget(String name, uint32 width, uint32 height) {
    if (name.length()==0&&mRenderTarget)
        name=mRenderTarget->getName();
    if (width==0) width=mWindowWidth->as<uint32>();
    if (height==0) height=mWindowHeight->as<uint32>();
    return createRenderTarget(name,width,height,true,mWindowDepth->as<Ogre::PixelFormat>());
}

Ogre::RenderTarget* OgreRenderer::createRenderTarget(const String&name, uint32 width, uint32 height, bool automipmap, int pixelFmt) {
    Ogre::PixelFormat pf = (Ogre::PixelFormat)pixelFmt;
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

void OgreRenderer::destroyRenderTarget(const String&name) {
    if (mRenderTarget->getName()==name) {

    }else {
        Ogre::ResourcePtr renderTargetTexture=Ogre::TextureManager::getSingleton().getByName(name);
        if (!renderTargetTexture.isNull())
            destroyRenderTarget(renderTargetTexture);
    }
}

void OgreRenderer::destroyRenderTarget(Ogre::ResourcePtr&name) {
    Ogre::TextureManager::getSingleton().remove(name);
}

bool OgreRenderer::useModelLights() const {
    return mModelLights->as<bool>();
}

void OgreRenderer::constructSystemLight(const String& name, const Vector3f& direction, float brightness) {
    LightInfo li;

    Color lcol(brightness, brightness, brightness);
    li.setLightDiffuseColor(lcol);
    li.setLightSpecularColor(lcol);
    li.setLightFalloff(1.f, 0.f, 0.f);
    li.setLightType(LightInfo::DIRECTIONAL);
    li.setLightRange(10000.f);

    Ogre::Light* light = constructOgreLight(getSceneManager(), String("____system_") + name, li);
    light->setDirection(toOgre(direction));

    getSceneManager()->getRootSceneNode()->attachObject(light);
}

void OgreRenderer::loadSystemLights() {
    if (useModelLights()) return;

    float brightness = 0.8;
    constructSystemLight("forward", Vector3f(0.f, 0.f, 1.f), brightness);
    constructSystemLight("back", Vector3f(0.f, 0.f, -1.f), brightness);
    constructSystemLight("left", Vector3f(-1.f, 0.f, 0.f), brightness);
    constructSystemLight("right", Vector3f(1.f, 0.f, 0.f), brightness);
    constructSystemLight("up", Vector3f(0.f, 1.f, 0.f), brightness);
    constructSystemLight("down", Vector3f(0.f, -1.f, 0.f), brightness);
}

void OgreRenderer::toggleSuspend() {
    mSuspended = !mSuspended;
}

void OgreRenderer::suspend() {
    mSuspended = true;
}

void OgreRenderer::resume() {
    mSuspended = false;
}
void OgreRenderer::quit() {
    mQuitRequested = true;
}

Ogre::Root* OgreRenderer::getRoot() {
    return &Ogre::Root::getSingleton();
}

Transfer::TransferPoolPtr OgreRenderer::transferPool() {
    return mTransferPool;
}

ResourceDownloadPlanner* OgreRenderer::downloadPlanner() {
    return mDownloadPlanner;
}

bool OgreRenderer::renderOneFrame(Task::LocalTime curFrameTime, Duration deltaTime) {
    for (std::list<OgreRenderer*>::iterator iter=sActiveOgreScenes.begin();iter!=sActiveOgreScenes.end();) {
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
    for (std::list<OgreRenderer*>::iterator iter=sActiveOgreScenes.begin();iter!=sActiveOgreScenes.end();) {
        (*iter++)->postFrame(postFrameTime, postFrameDelta);
    }

    if (!mNextFrameScreenshotFile.empty()) {
        screenshot(mNextFrameScreenshotFile);
        mNextFrameScreenshotFile = "";
    }

    static int counter=0;
    counter++;

    return continueRendering;
}

void OgreRenderer::preFrame(Task::LocalTime currentTime, Duration frameTime) {
    if (mOnTickCallback != NULL) {
        std::vector<boost::any> tick_args;
        tick_args.push_back(Invokable::asAny( (currentTime-Task::LocalTime::epoch()).toSeconds() ));
        tick_args.push_back(Invokable::asAny(frameTime.toSeconds()));
        mOnTickCallback->invoke(tick_args);
    }

    std::list<Entity*>::iterator iter;
    Time cur_time = mContext->simTime();
    for (iter = mMovingEntities.begin(); iter != mMovingEntities.end(); )
        (*iter++)->tick(cur_time, frameTime);

    mResourceLoader->tick();
}

void OgreRenderer::postFrame(Task::LocalTime current, Duration frameTime) {
}

void OgreRenderer::poll()
{
    if (!initialized)
        return;

    if (stopped)
        return;


    Task::LocalTime curFrameTime(Task::LocalTime::now());

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

void OgreRenderer::stop()
{
    simStrand->post(
        std::tr1::bind(&OgreRenderer::iStop, this,
            livenessToken()),
        "OgreRenderer::iStop");
}

void OgreRenderer::iStop(Liveness::Token rendererAlive)
{
    if (!rendererAlive) return;
    Liveness::Lock locked(rendererAlive);
    if (!locked)
        return;

    while (! initialized){}

    delete mParsingWork;
    TimeSteppedSimulation::stop();
    stopped = true;
}

// Invokable Interface
boost::any OgreRenderer::invoke(std::vector<boost::any>& params) {
    // Decode the command. First argument is the "function name"
    if (params.empty() || !Invokable::anyIsString(params[0]))
        return boost::any();

    std::string name = Invokable::anyAsString(params[0]);
    SILOG(ogre,detailed,"Invoking the function " << name);

    if (name == "onTick")
        return setOnTick(params);
    else if (name == "setMaxObjects")
        return setMaxObjects(params);
    else if (name == "setObjectPrioritization")
        return setObjectPrioritization(params);
    else
        SILOG(ogre, warn, "Function " << name << " was invoked but this function was not found.");

    return boost::any();
}

boost::any OgreRenderer::setOnTick(std::vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsInvokable(params[1])) return boost::any();

    Invokable* handler = Invokable::anyAsInvokable(params[1]);
    mOnTickCallback = handler;
    return boost::any();
}

boost::any OgreRenderer::setMaxObjects(std::vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsNumeric(params[1])) return boost::any();
    uint32 new_max_objects = Invokable::anyAsNumeric(params[1]);

    mDownloadPlanner->setMaxObjects(new_max_objects);

    return boost::any();
}

boost::any OgreRenderer::setObjectPrioritization(std::vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsString(params[1])) return boost::any();
    String new_prioritization = Invokable::anyAsString(params[1]);

    if (new_prioritization == "distance") {
        PriorityDownloadPlannerMetricPtr metric(new DistanceDownloadPlannerMetric());
        mDownloadPlanner->setPrioritizationMetric(metric);
        return Invokable::asAny(true);
    }
    else if (new_prioritization == "solid_angle") {
        PriorityDownloadPlannerMetricPtr metric(new SolidAngleDownloadPlannerMetric());
        mDownloadPlanner->setPrioritizationMetric(metric);
        return Invokable::asAny(true);
    }

    return Invokable::asAny(false);
}

void OgreRenderer::injectWindowResized(uint32 w, uint32 h) {
    // You might think we would do this:
    //   mRenderWindow->windowMovedOrResized();
    // but it turns out that Ogre isn't handling externally created windows
    // properly. Instead, we force a resize directly.
    if (!mOgreOwnedRenderWindow) {
        mRenderWindow->resize(w, h);
    }
    // Then, we force the resize event because apparently calling resize()
    // doesn't trigger it.
    windowResized(mRenderWindow);
}

void OgreRenderer::windowResized(Ogre::RenderWindow *rw) {
    SILOG(ogre,insane,"Ogre resized window: " << rw->getWidth() << "x" << rw->getHeight());

    for(CameraSet::iterator cam_it = mAttachedCameras.begin(); cam_it != mAttachedCameras.end(); cam_it++)
        (*cam_it)->windowResized();
}

void OgreRenderer::windowFocusChange(Ogre::RenderWindow *rw) {
    mInputManager->windowFocusChange();
}

float32 OgreRenderer::nearPlane() {
    return mOptions->referenceOption("nearplane")->as<float32>();
}

float32 OgreRenderer::farPlane() {
    return mOptions->referenceOption("farplane")->as<float32>();
}

float32 OgreRenderer::parallaxSteps() {
    return mOptions->referenceOption("parallax-steps")->as<float32>();
}

int32 OgreRenderer::parallaxShadowSteps() {
    return mOptions->referenceOption("parallax-shadow-steps")->as<int>();
}

void OgreRenderer::attachCamera(const String &renderTargetName, Camera* entity) {
    mDownloadPlanner->setCamera(entity);
    mAttachedCameras.insert(entity);
}

void OgreRenderer::detachCamera(Camera* entity) {
    if (mAttachedCameras.find(entity) == mAttachedCameras.end()) return;
    mAttachedCameras.erase(entity);
}

void OgreRenderer::addObject(Entity* ent, const Transfer::URI& mesh) {
    mDownloadPlanner->addNewObject(ent, mesh);
}


void OgreRenderer::removeObject(Entity* ent) {
    mDownloadPlanner->removeObject(ent);
}

ParseMeshTaskHandle OgreRenderer::parseMesh(
    const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp,
    Transfer::DenseDataPtr data, bool isAggregate, ParseMeshCallback cb)
{
    ParseMeshTaskHandle handle(new ParseMeshTaskInfo);
    mParsingIOService->post(
        std::tr1::bind(&OgreRenderer::parseMeshWork, this,
            livenessToken(), handle, metadata, fp, data, isAggregate, cb),
        "OgreRenderer::parseMeshWork"
    );
    return handle;
}

void OgreRenderer::parseMeshWork(
    Liveness::Token rendererAlive,
    ParseMeshTaskHandle handle,
    const Transfer::RemoteFileMetadata& metadata,
    const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data,
    bool isAggregate, ParseMeshCallback cb)
{
    if (!rendererAlive) return;
    Liveness::Lock locked(rendererAlive);
    if (!locked)
        return;

    if (stopped)
        return;

    // Check for user cancellation.
    if (!handle->process())
        return;

    mParserProfiler->started();
    Mesh::VisualPtr parsed = parseMeshWorkSync(metadata, fp, data, isAggregate);
    simStrand->post(std::tr1::bind(cb,parsed),
        "OgreRenderer::parseMeshWork callback"
    );
    mParserProfiler->finished();
}

Mesh::VisualPtr OgreRenderer::parseMeshWorkSync(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, bool isAggregate) {
    Mesh::VisualPtr parsed = mModelParser->load(metadata, fp, data);
    if (parsed && mModelFilter) {
        Mesh::MutableFilterDataPtr input_data(new Mesh::FilterData);
        input_data->push_back(parsed);
        Mesh::FilterDataPtr output_data = mModelFilter->apply(input_data);

        parsed = output_data->get();
        input_data->clear();
        input_data->push_back(parsed);
        output_data = mCenteringFilter->apply(input_data);

        assert(output_data->single());
        parsed = output_data->get();
    }
    return parsed;
}

void OgreRenderer::screenshot(const String& filename) {
    if (mRenderTarget != NULL)
        mRenderTarget->writeContentsToFile(filename);
}

void OgreRenderer::screenshotNextFrame(const String& filename) {
    mNextFrameScreenshotFile = filename;
}

} // namespace Graphics
} // namespace Sirikata
