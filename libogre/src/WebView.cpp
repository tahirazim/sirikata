/*  Sirikata
 *  WebView.cpp
 *
 *  Copyright (c) 2009, Adam Jean Simmons
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

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/core/util/TemporalValue.hpp>
#include <sirikata/ogre/WebView.hpp>
#include <sirikata/ogre/WebViewManager.hpp>
#include <OgreBitwise.h>
#include <sirikata/core/util/UUID.hpp>

#ifdef HAVE_BERKELIUM
#include "berkelium/StringUtil.hpp"
#include "berkelium/ScriptVariant.hpp"
#endif

#include <boost/lexical_cast.hpp>

using namespace Ogre;

namespace Sirikata {
namespace Graphics {

#ifdef HAVE_BERKELIUM
using Berkelium::UTF8String;
#endif

WebView::WebView(
    Context* ctx,
    const std::string& name, const std::string& type,
    unsigned short width, unsigned short height,
    const OverlayPosition &viewPosition, Ogre::uchar zOrder, Tier tier,
    Ogre::Viewport* viewport,Network::IOStrandPtr strand,
    const WebViewBorderSize& border)
 : mContext(ctx),
   postingStrand(strand)
{
#ifdef HAVE_BERKELIUM
	webView = 0;
#endif
	viewName = name;
        viewType = type;
        viewURL = "";
	viewWidth = width;
	viewHeight = height;
	maxUpdatePS = 0;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	isWebViewTransparent = false;
	ignoringBounds = false;
	okayToDelete = false;
	compensateNPOT = false;
	texWidth = width;
	texHeight = height;
	alphaCache = 0;
	alphaCachePitch = 0;
	matPass = 0;
	baseTexUnit = 0;
	maskTexUnit = 0;
	fadeValue = 1;
	isFading = false;
	deltaFadePerMS = 0;
	lastFadeTimeMS = 0;
        mExceptionHandler = NULL;
	texFiltering = Ogre::FO_NONE;

        mReady = false;
        mUnresponsive = false;

    mBorderLeft = border.mBorderLeft;
    mBorderRight = border.mBorderRight;
    mBorderTop = border.mBorderTop;
    mBorderBottom = border.mBorderBottom;

    createMaterial();

#ifdef HAVE_BERKELIUM
    overlay = new ViewportOverlay(name + "_overlay", viewport, width, height, viewPosition, getMaterialName(), zOrder, tier);
    if(compensateNPOT)
        overlay->panel->setUV(0, 0, (Real)viewWidth/(Real)texWidth, (Real)viewHeight/(Real)texHeight);
#else
    overlay = NULL;
#endif
}

WebView::WebView(
    Context* ctx,
    const std::string& name, const std::string& type, unsigned short width, unsigned short height,
    Ogre::FilterOptions texFiltering,
    Network::IOStrandPtr strand)
 : mContext(ctx),
   postingStrand(strand)
{
#ifdef HAVE_BERKELIUM
	webView = 0;
#endif

    mBorderLeft=2;
    mBorderRight=2;
    mBorderTop=25;
    mBorderBottom=2;
	viewName = name;
        viewType = type;
	viewWidth = width;
	viewHeight = height;
	overlay = 0;
	maxUpdatePS = 0;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05;
	ignoringBounds = false;
	okayToDelete = false;
	compensateNPOT = false;
	texWidth = width;
	texHeight = height;
	alphaCache = 0;
	alphaCachePitch = 0;
	matPass = 0;
	baseTexUnit = 0;
	maskTexUnit = 0;
	fadeValue = 1;
	isFading = false;
	deltaFadePerMS = 0;
	lastFadeTimeMS = 0;
        mExceptionHandler = NULL;
	this->texFiltering = texFiltering;

        mReady = false;
        mUnresponsive = false;

	createMaterial();
}

WebView::~WebView()
{
    Liveness::letDie();

	if(alphaCache)
		delete[] alphaCache;
#ifdef HAVE_BERKELIUM
        cleanupWebView();
#endif
	if(overlay)
		delete overlay;

	MaterialManager::getSingletonPtr()->remove(getMaterialName());
	if (!this->viewTexture.isNull()) {
        ResourcePtr res(this->viewTexture);
        this->viewTexture.setNull();
        Ogre::TextureManager::getSingleton().remove(res);
    }
}

void WebView::createWebView(bool resetting)
{
#ifdef HAVE_BERKELIUM
    initializeWebView(Berkelium::Window::create(
            WebViewManager::getSingleton().bkContext),
        false
    );
#endif
}

void WebView::initializeWebView(
#ifdef HAVE_BERKELIUM
    Berkelium::Window *win,
#endif
    bool resetting
    )
{
    // The resetting flag allows us to not double-bind builtin
    // functions

#ifdef HAVE_BERKELIUM
    webView = win;
    webView->setDelegate(this);
    // Unlike chrome.send (below) this version does not do logging.
    webView->addBindOnStartLoading(WideString::point_to(L"sirikata"),
                  Berkelium::Script::Variant::emptyObject());
    webView->addEvalOnStartLoading(WideString::point_to(L"__sirikata = sirikata;\n"));
    webView->addBindOnStartLoading(WideString::point_to(L"sirikata.version"),
                  Berkelium::Script::Variant::emptyObject());
    webView->addBindOnStartLoading(WideString::point_to(L"sirikata.version.major"),
        Berkelium::Script::Variant((int)SIRIKATA_VERSION_MAJOR));
    webView->addBindOnStartLoading(WideString::point_to(L"sirikata.version.minor"),
        Berkelium::Script::Variant((int)SIRIKATA_VERSION_MINOR));
    webView->addBindOnStartLoading(WideString::point_to(L"sirikata.version.revision"),
        Berkelium::Script::Variant((int)SIRIKATA_VERSION_REVISION));
    webView->addBindOnStartLoading(WideString::point_to(L"sirikata.version.commit"),
        Berkelium::Script::Variant(SIRIKATA_GIT_REVISION));
    webView->addBindOnStartLoading(WideString::point_to(L"sirikata.version.string"),
        Berkelium::Script::Variant(SIRIKATA_VERSION));
    webView->addBindOnStartLoading(WideString::point_to(L"sirikata.__event"),
                  Berkelium::Script::Variant::bindFunction(
                      WideString::point_to(L"send"), true));
    // Note that the setup here is a little weird -- single arguments
    // are passed through as their value, everything else as a single
    // array. Note also that we handle the event name separately
    webView->addEvalOnStartLoading(
        WideString::point_to(
            L"sirikata.event = function() {\n"
            L"var args = [];\n"
            L"for(var i = 0; i < arguments.length; i++) { args.push(arguments[i]); }\n"
            L"sirikata.__event.apply(this, args);\n"
            L"};"));
    if (!resetting) bind("__log", std::tr1::bind(&WebView::userLog, this, _1, _2));
    // Deprecated
    webView->addBindOnStartLoading(WideString::point_to(L"chrome"),
                  Berkelium::Script::Variant::emptyObject());
    webView->addBindOnStartLoading(WideString::point_to(L"chrome.send_"),
                  Berkelium::Script::Variant::bindFunction(
                      WideString::point_to(L"send"), false));
    webView->addEvalOnStartLoading(
        WideString::point_to(L"chrome.send = function(n, args) {\n"
                             L"console.log([n].concat(args));\n"
                             L"chrome.send_.apply(this, [n].concat(args));\n"
                             L"};"));
    if (!resetting) bind("__ready", std::tr1::bind(&WebView::handleReadyCallback, this, _1, _2));
    if (!resetting) bind("__setViewport", std::tr1::bind(&WebView::handleSetUIViewport, this, _1, _2));
    if (!resetting) bind("__openBrowser", std::tr1::bind(&WebView::handleOpenBrowser, this, _1, _2));
    if (!resetting) bind("__listenToBrowser", std::tr1::bind(&WebView::handleListenToBrowser, this, _1, _2));
    if (!resetting) bind("__getBrowserURL", std::tr1::bind(&WebView::handleGetBrowserURL, this, _1, _2));
    if (!resetting) bind("__closeBrowser", std::tr1::bind(&WebView::handleCloseBrowser, this, _1, _2));
    //make sure that the width and height of the border do not dominate the size
    if (viewWidth>mBorderLeft+mBorderRight&&viewHeight>mBorderTop+mBorderBottom) {
        webView->resize(viewWidth-mBorderLeft-mBorderRight, viewHeight-mBorderTop-mBorderBottom);
    } else {
        webView->resize(0, 0);
    }
#endif
}

void WebView::cleanupWebView() {
#ifdef HAVE_BERKELIUM
    if (webView == NULL) return;
    webView->destroy();
    webView = NULL;
#endif
}

void WebView::setUpdateViewportCallback(UpdateViewportCallback cb) {
    mUpdateViewportCallback = cb;
}

void WebView::setReadyCallback(ReadyCallback cb) {
    mReadyCallback = cb;
#ifndef HAVE_BERKELIUM
    // Without berkelium, we'll never get a ready callback. Force one now, but
    // only after the current event finishes being handled (because this is
    // called during initialization and callbacks fired in response to this one
    // may not be registered quite yet).
    if (mReadyCallback)
        mContext->mainStrand->post(mReadyCallback);
#endif
}


void WebView::setResetReadyCallback(ResetReadyCallback cb) {
    mResetReadyCallback = cb;
}

void WebView::setNavigatedCallback(NavigatedCallback cb) {
    mNavigatedCallback = cb;
}

boost::any WebView::handleReadyCallback(WebView* wv, const JSArguments& args) {
    if (mReady) { // This is a reset
        if (mResetReadyCallback) mResetReadyCallback();
    }
    else {
        mReady = true;
        if (mReadyCallback) mReadyCallback();
    }
    return boost::any();
}

boost::any WebView::handleSetUIViewport(WebView* wv, const JSArguments& args) {
    assert(args.size() == 4);

    int32 left = boost::lexical_cast<int32>(String(args[0].begin()));
    int32 top = boost::lexical_cast<int32>(String(args[1].begin()));
    int32 right = boost::lexical_cast<int32>(String(args[2].begin()));
    int32 bottom = boost::lexical_cast<int32>(String(args[3].begin()));

    if (mUpdateViewportCallback) mUpdateViewportCallback(left, top, right, bottom);

    return boost::any();
}

boost::any WebView::userLog(WebView* wv, const JSArguments& args) {
    if (args.size() == 0)
        return boost::any(); // not sure why they would do this

    String level(args[0].begin());
    String msg;
    for(int i = 1; i < (int)args.size(); i++) {
        if (i > 1) msg = msg + ' ';
        msg = msg + String(args[i].begin());
    }

    // This kinda sucks, but SILOG requires the literal value, not a variable
    if (level == "fatal")
        SILOG(ui, fatal, msg);
    else if (level == "error")
        SILOG(ui, error, msg);
    else if (level == "warning" || level == "warn")
        SILOG(ui, warning, msg);
    else if (level == "info")
        SILOG(ui, info, msg);
    else if (level == "debug")
        SILOG(ui, debug, msg);
    else if (level == "detailed")
        SILOG(ui, detailed, msg);
    else if (level == "insane")
        SILOG(ui, insane, msg);

    return boost::any();
}

boost::any WebView::handleOpenBrowser(WebView* wv, const JSArguments& args) {
    String name(args[0].begin());
    String url(args[1].begin());
    if (url.empty() || name.empty()) return boost::any();
    int32 w = 0, h = 0;
    if (args.size() >= 4) {
        w = boost::lexical_cast<int32>(String(args[2].begin()));
        h = boost::lexical_cast<int32>(String(args[3].begin()));
    }
    if (w == 0) w = 400;
    if (h == 0) h = 400;

    WebView* child_wv = WebViewManager::getSingleton().createWebView(
        mContext,
        name, name, w, h,
        OverlayPosition(RP_CENTER), postingStrand, false, 70,
        TIER_MIDDLE,0);

    
    child_wv->loadURL(url);
    child_wv->setTransparent(false);

    return boost::any();
}

boost::any WebView::handleListenToBrowser(WebView* wv, const JSArguments& args) {
    String name(args[0].begin());
    String cb_name(args[1].begin());
    if (name.empty() || cb_name.empty()) return boost::any();

    WebView* child_wv = WebViewManager::getSingleton().getWebView(name);
    if (child_wv == NULL) return boost::any();

    child_wv->setNavigatedCallback(
        std::tr1::bind(
            &WebView::forwardBrowserNavigatedCallback, this,
            livenessToken(),
            cb_name, std::tr1::placeholders::_1
        )
    );

    return boost::any();
}

boost::any WebView::handleGetBrowserURL(WebView* wv, const JSArguments& args) {
    String name(args[0].begin());
    if (name.empty()) return boost::any();

    WebView* child_wv = WebViewManager::getSingleton().getWebView(name);
    if (child_wv == NULL) return boost::any();

    return boost::any( child_wv->viewURL );
}

void WebView::forwardBrowserNavigatedCallback(Liveness::Token alive, const String& cb_name, const String& url) {
    // Note that this is forwarding an event about a *different*
    // webview to *this* webview's javascript. We need to check
    // liveness since the child may outlive the parent (although it
    // shouldn't, or you're probably going to have orphan WebViews
    // that will never get closed.
    Liveness::Lock locked(alive);
    if (!locked)
        return;

    String escaped_url = "";
    for(uint32 i = 0; i < url.size(); i++) {
        if (url[i] == '\'') escaped_url += "\\'";
        else escaped_url.push_back(url[i]);
    }
    evaluateJS( cb_name + "( 'navigate', '" + escaped_url + "')" );
}

void WebView::defaultEvent(const String& name) {
#ifndef HAVE_BERKELIUM
    mContext->mainStrand->post(
        std::tr1::bind(&WebView::dispatchToDelegate, this, name, JSArguments())
    );
#endif
}

boost::any WebView::handleCloseBrowser(WebView* wv, const JSArguments& args) {
    String name(args[0].begin());
    if (!name.empty())
        WebViewManager::getSingleton().destroyWebView(name);
    return boost::any();
}


void WebView::createMaterial()
{
	if(opacity > 1) opacity = 1;
	else if(opacity < 0) opacity = 0;

	if(!Bitwise::isPO2(viewWidth) || !Bitwise::isPO2(viewHeight))
	{
		if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_NON_POWER_OF_2_TEXTURES))
		{
			if(Root::getSingleton().getRenderSystem()->getCapabilities()->getNonPOW2TexturesLimited())
				compensateNPOT = true;
		}
		else compensateNPOT = true;
#ifdef __APPLE__
//cus those fools always report #t when I ask if they support this or that
//and then fall back to their buggy and terrible software driver which has never once in my life rendered a single correct frame.
		compensateNPOT=true;
#endif
		if(compensateNPOT)
		{
			texWidth = Bitwise::firstPO2From(viewWidth);
			texHeight = Bitwise::firstPO2From(viewHeight);
		}
	}


	// Create the texture
#if defined(HAVE_BERKELIUM) || !defined(__APPLE__)
	TexturePtr texture = TextureManager::getSingleton().createManual(
		getViewTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC, this);
    this->viewTexture = texture;

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	unsigned int texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	unsigned int texPitch = (pixelBox.rowPitch*texDepth);

     uint8* pDest = static_cast<uint8*>(pixelBox.data);

     memset(pDest, 0, texHeight*texPitch);

     pixelBuffer->unlock();
 #endif
     MaterialPtr material = MaterialManager::getSingleton().create(getMaterialName(),
         ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
     matPass = material->getTechnique(0)->getPass(0);
     //matPass->setSeparateSceneBlending (SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA, SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA);
     matPass->setSeparateSceneBlending (SBF_ONE, SBF_ONE_MINUS_SOURCE_ALPHA, SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA);
     matPass->setDepthWriteEnabled(false);

     baseTexUnit = matPass->createTextureUnitState(getViewTextureName());

     baseTexUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
     if(texFiltering == FO_ANISOTROPIC)
         baseTexUnit->setTextureAnisotropy(4);
 }

 // This is for when the rendering device has a hiccup and loses the dynamic texture
 void WebView::loadResource(Resource* resource)
 {
     Texture *tex = static_cast<Texture*>(resource);

     tex->setTextureType(TEX_TYPE_2D);
     tex->setWidth(texWidth);
     tex->setHeight(texHeight);
     tex->setNumMipmaps(0);
     tex->setFormat(PF_BYTE_BGRA);
     tex->setUsage(TU_DYNAMIC);
     tex->createInternalResources();

     // force update
 }

 void WebView::update()
 {
     if(maxUpdatePS)
         if(timer.getMilliseconds() - lastUpdateTime < 1000 / maxUpdatePS)
             return;

     updateFade();

     if(usingMask)
         baseTexUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, fadeValue * opacity);
     else if(isWebViewTransparent)
         baseTexUnit->setAlphaOperation(LBX_BLEND_TEXTURE_ALPHA, LBS_MANUAL, LBS_TEXTURE, fadeValue * opacity);
     else
         baseTexUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, fadeValue * opacity);

     lastUpdateTime = timer.getMilliseconds();
 }

 void WebView::updateFade()
 {
     if(isFading)
     {
         fadeValue += deltaFadePerMS * (timer.getMilliseconds() - lastFadeTimeMS);

         if(fadeValue > 1)
         {
             fadeValue = 1;
             isFading = false;
         }
         else if(fadeValue < 0)
         {
             fadeValue = 0;
             isFading = false;
             if(overlay) {overlay->hide();}
         }

         lastFadeTimeMS = timer.getMilliseconds();
     }
 }

 bool WebView::isPointOverMe(int x, int y)
 {
     if(isMaterialOnly())
         return false;
     if(!overlay || !overlay->isVisible || !overlay->viewport)
         return false;

     int localX = overlay->getRelativeX(x);
     int localY = overlay->getRelativeY(y);

     if(localX > 0 && localX < overlay->width)
         if(localY > 0 && localY < overlay->height)
             return !ignoringTrans || !alphaCache ? true :
                 alphaCache[localY * alphaCachePitch + localX] > 255 * transparent;

     return false;
 }

 void WebView::loadURL(const std::string& url)
 {
 #if defined(HAVE_BERKELIUM)
     webView->navigateTo(url.data(),url.length());
 #endif
 }

 void WebView::loadFile(const std::string& file)
 {
 #if defined(HAVE_BERKELIUM)
     std::string url = file;
     if (file.length() == 0) {
         url = "about:blank";
     } else if (file[0]=='/') {
         url = "file://"+file;
     } else {
         url = "file://"+WebViewManager::getSingleton().getBaseDir()+"/"+file;
     }
     webView->navigateTo(url.data(),url.length());
 #endif
 }
 static std::string htmlPrepend("data:text/html;charset=utf-8,");
 void WebView::loadHTML(const std::string& html)
 {
 #if defined(HAVE_BERKELIUM)
     char * data= new char[htmlPrepend.length()+html.length()+1];
     memcpy(data,htmlPrepend.data(),htmlPrepend.length());
     memcpy(data+htmlPrepend.length(),html.data(),html.length());
     data[htmlPrepend.length()+html.length()]=0;
     webView->navigateTo(data,htmlPrepend.length()+html.length());
     delete[] data;
 #endif
 }

 void WebView::evaluateJS(const std::string& utf8jsoo)
 {
     std::string utf8js = utf8jsoo + ";//";
 #if defined(HAVE_BERKELIUM)
     WideString wideJS = Berkelium::UTF8ToWide(
         UTF8String::point_to(utf8js.data(), utf8js.length()));
     UTF8String uJS = Berkelium::WideToUTF8(
         WideString::point_to(wideJS.data(), wideJS.length()));
     SILOG(webview,detailed,"Eval JS: "<<uJS);
     webView->executeJavascript(wideJS);
     Berkelium::stringUtil_free(uJS);
     Berkelium::stringUtil_free(wideJS);
 #endif
 }

 void WebView::bind(const std::string& name, JSDelegate callback)
 {
     delegateMap[name] = callback;
 }

 void WebView::setViewport(Ogre::Viewport* newViewport)
 {
     if(overlay) {
         overlay->setViewport(newViewport);
     }
 }

 void WebView::setTransparent(bool isTransparent)
 {
     if(!isTransparent)
     {
         if(alphaCache && !usingMask)
         {
             delete[] alphaCache;
             alphaCache = 0;
         }
     }
     else
     {
         if(!alphaCache && !usingMask)
         {
             alphaCache = new unsigned char[texWidth * texHeight];
             alphaCachePitch = texWidth;
         }
     }

 #if defined(HAVE_BERKELIUM)
     webView->setTransparent(isTransparent);
 #endif
     isWebViewTransparent = isTransparent;
 }

 void WebView::setIgnoreBounds(bool ignoreBounds)
 {
     ignoringBounds = ignoreBounds;
 }

 void WebView::setIgnoreTransparent(bool ignoreTrans, float threshold)
 {
     ignoringTrans = ignoreTrans;

     if(threshold > 1) threshold = 1;
     else if(threshold < 0) threshold = 0;

     transparent = threshold;
 }

 void WebView::setMaxUPS(unsigned int maxUPS)
 {
     maxUpdatePS = maxUPS;
 }

 void WebView::setOpacity(float opacity)
 {
     if(opacity > 1) opacity = 1;
     else if(opacity < 0) opacity = 0;

     this->opacity = opacity;
 }

 void WebView::setPosition(const OverlayPosition &viewPosition)
 {
     if(overlay){
         overlay->setPosition(viewPosition);
     }
 }

 void WebView::resetPosition()
 {
     if(overlay){
         overlay->resetPosition();
     }
 }

 void WebView::hide()
 {
     hide(false, 0);
 }

 void WebView::hide(bool fade, unsigned short fadeDurationMS)
 {
     updateFade();

     if(fade)
     {
         isFading = true;
         deltaFadePerMS = -1 / (double)fadeDurationMS;
         lastFadeTimeMS = timer.getMilliseconds();
     }
     else
     {
         isFading = false;
         fadeValue = 0;
         if (overlay) {overlay->hide();}
     }
 }

 void WebView::show()
 {
     show(false, 0);
 }

 void WebView::show(bool fade, unsigned short fadeDurationMS)
 {
     updateFade();

     if(fade)
     {
         isFading = true;
         deltaFadePerMS = 1 / (double)fadeDurationMS;
         lastFadeTimeMS = timer.getMilliseconds();
     }
     else
     {
         isFading = false;
         fadeValue = 1;
     }

     if (overlay) {overlay->show();}
 }

 void WebView::focus()
 {
 #if defined(HAVE_BERKELIUM)
     webView->focus();
 #endif
 }

 void WebView::unfocus()
 {
 #if defined(HAVE_BERKELIUM)
     webView->unfocus();
 #endif
 }

 void WebView::raise()
 {
     if(overlay) {
         WebViewManager::getSingleton().focusWebView(this);
     } else {
         focus();
     }
 }

 void WebView::move(int deltaX, int deltaY)
 {
     if(overlay) {
         overlay->move(deltaX, deltaY);
     }
 }

 void WebView::getExtents(unsigned short &width, unsigned short &height)
 {
     width = viewWidth;
     height = viewHeight;
 }

 int WebView::getRelativeX(int absX)
 {
     if(isMaterialOnly())
         return 0;
     else
         return overlay->getRelativeX(absX);
 }

 int WebView::getRelativeY(int absY)
 {
     if(isMaterialOnly())
         return 0;
     else
         return overlay->getRelativeY(absY);
 }

 bool WebView::inDraggableRegion(int relX, int relY) {
     if(relY <= mBorderTop) {
         return true;
     }
     return false;
 }

 bool WebView::isMaterialOnly()
 {
     return !overlay;
 }

 ViewportOverlay* WebView::getOverlay()
 {
     return overlay;
 }

 std::string WebView::getName()
 {
     return viewName;
 }

 std::string WebView::getType()
 {
     return viewType;
 }

std::string WebView::getURL() {
    return viewURL;
}

 std::string WebView::getViewTextureName()
 {
     return viewName;
 }

 std::string WebView::getMaterialName()
 {
     return viewName + "Material";
 }

 bool WebView::getVisibility()
 {
     if(isMaterialOnly())
         return fadeValue != 0;
     else
         return overlay->isVisible;
 }

 bool WebView::getNonStrictVisibility() {
	 if (!overlay) {return true;}
     if (isFading) {
         // When fading, we are actually the *opposite* of what the overlay claims.
         return !overlay->isVisible;
     }
     else {
         // If we're not fading, then we can trust the overlay.
         return overlay->isVisible;
     }
 }

 void WebView::getDerivedUV(Ogre::Real& u1, Ogre::Real& v1, Ogre::Real& u2, Ogre::Real& v2)
 {
	u1 = v1 = 0;
	u2 = v2 = 1;

	if(compensateNPOT)
	{
		u2 = (Ogre::Real)viewWidth/(Ogre::Real)texWidth;
		v2 = (Ogre::Real)viewHeight/(Ogre::Real)texHeight;
	}
}

void WebView::injectMouseMove(int xPos, int yPos)
{
    if (xPos>mBorderLeft&&yPos>mBorderTop&&xPos<viewWidth-mBorderRight) {
#if defined(HAVE_BERKELIUM)
        webView->mouseMoved(xPos-mBorderLeft, yPos-mBorderTop);
#endif
    }else {
        //handle tug on border
    }
}

void WebView::injectMouseWheel(int relScrollX, int relScrollY)
{
#if defined(HAVE_BERKELIUM)
    webView->mouseWheel(relScrollX, relScrollY);
#endif
}

void WebView::injectMouseDown(int xPos, int yPos)
{
    if (xPos>mBorderLeft&&yPos>mBorderTop&&xPos<viewWidth-mBorderRight) {
#if defined(HAVE_BERKELIUM)
        webView->mouseMoved(xPos-mBorderLeft, yPos-mBorderTop);
    webView->mouseButton(0, true);
#endif
    }else {
        //handle tug on border
    }
}

void WebView::injectMouseUp(int xPos, int yPos)
{
    if (xPos>mBorderLeft&&yPos>mBorderTop&&xPos<viewWidth-mBorderRight) {
#if defined(HAVE_BERKELIUM)
        webView->mouseMoved(xPos-mBorderLeft, yPos-mBorderTop);
        webView->mouseButton(0, false);
#endif
    }else {
        //handle tug on border
    }
}

void WebView::injectCut() {
#if defined(HAVE_BERKELIUM)
    webView->cut();
#endif
}

void WebView::injectCopy() {
#if defined(HAVE_BERKELIUM)
    webView->copy();
#endif
}

void WebView::injectPaste() {
#if defined(HAVE_BERKELIUM)
    webView->paste();
#endif
}

void WebView::injectKeyEvent(bool press, int modifiers, int vk_code, int scancode) {
#if defined(HAVE_BERKELIUM)
	webView->keyEvent(press, modifiers, vk_code, scancode);
#endif
}

void WebView::injectTextEvent(std::string utf8) {
#if defined(HAVE_BERKELIUM)
    wchar_t *outchars = new wchar_t[utf8.size()+1];
    size_t len = mbstowcs(outchars, utf8.c_str(), utf8.size());
    webView->textEvent(outchars,len);
    delete []outchars;
#endif
}

void WebView::resize(int width, int height)
{
	if(width == viewWidth && height == viewHeight)
		return;

	viewWidth = width;
	viewHeight = height;

	int newTexWidth = viewWidth;
	int newTexHeight = viewHeight;

	if(!Bitwise::isPO2(viewWidth) || !Bitwise::isPO2(viewHeight))
	{
		if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_NON_POWER_OF_2_TEXTURES))
		{
			if(Root::getSingleton().getRenderSystem()->getCapabilities()->getNonPOW2TexturesLimited())
				compensateNPOT = true;
		}
		else compensateNPOT = true;
		compensateNPOT=true;
		if(compensateNPOT)
		{
			newTexWidth = Bitwise::firstPO2From(viewWidth);
			newTexHeight = Bitwise::firstPO2From(viewHeight);
		}
	}

	if (overlay) {overlay->resize(viewWidth, viewHeight);}
#if defined(HAVE_BERKELIUM)
    //make sure that the width and height of the border do not dominate the size
    if (viewWidth>mBorderLeft+mBorderRight&&viewHeight>mBorderTop+mBorderBottom) {
        webView->resize(viewWidth-mBorderLeft-mBorderRight, viewHeight-mBorderTop-mBorderBottom);
    } else {
        webView->resize(0, 0);
    }
#endif

    uint16 oldTexWidth = texWidth;
    uint16 oldTexHeight = texHeight;

    texWidth = newTexWidth;
    texHeight = newTexHeight;

    if (compensateNPOT) {
        // FIXME: How can we adjust UV coordinates on a mesh that we are bound to?
        Ogre::Real u1,v1,u2,v2;
        getDerivedUV(u1, v1,  u2,v2);
        if (overlay) {
            overlay->panel->setUV(u1, v1, u2, v2);
        }
    }

    if (texWidth == oldTexWidth && texHeight == oldTexHeight)
        return;

	matPass->removeAllTextureUnitStates();
	maskTexUnit = 0;
#if defined(HAVE_BERKELIUM)

	if (!this->viewTexture.isNull()) {
        ResourcePtr res(this->viewTexture);
        this->viewTexture.setNull();
        Ogre::TextureManager::getSingleton().remove(res);
    }

	this->viewTexture = TextureManager::getSingleton().createManual(
		getViewTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC, this);

    TexturePtr texture = this->viewTexture;

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	unsigned int texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	unsigned int texPitch = (pixelBox.rowPitch*texDepth);

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	memset(pDest, 0, texHeight*texPitch);

	pixelBuffer->unlock();

	if (!this->backingTexture.isNull()) {
        ResourcePtr res(this->backingTexture);
        this->backingTexture.setNull();
        Ogre::TextureManager::getSingleton().remove(res);

        this->backingTexture = TextureManager::getSingleton().createManual(
            "B"+getViewTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
            TU_DYNAMIC, this);
    }

#endif

	baseTexUnit = matPass->createTextureUnitState(viewTexture->getName());

	baseTexUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
	if(texFiltering == FO_ANISOTROPIC)
		baseTexUnit->setTextureAnisotropy(4);

    if(alphaCache)
	{
		delete[] alphaCache;
		alphaCache = new unsigned char[texWidth * texHeight];
		alphaCachePitch = texWidth;
	}
}


boost::any WebView::dispatchToDelegate(const String& name, const JSArguments& args) {
    std::map<std::string, JSDelegate>::iterator it = delegateMap.find(name);
    if (it != delegateMap.end())
        return it->second(this, args);
    return boost::any();
}


#ifdef HAVE_BERKELIUM
///////// Berkelium Callbacks...
void WebView::onAddressBarChanged(Berkelium::Window*, URLString newURL) {
    SILOG(webview,detailed,"onAddressBarChanged"<<newURL);
    viewURL = newURL.get<String>();
    if (mNavigatedCallback) mNavigatedCallback(viewURL);
}
void WebView::onStartLoading(Berkelium::Window*, URLString newURL) {
    SILOG(webview,detailed,"onStartLoading"<<newURL);
    viewURL = newURL.get<String>();
    if (mNavigatedCallback) mNavigatedCallback(viewURL);
}
void WebView::onTitleChanged(Berkelium::Window*, WideString wtitle) {
    UTF8String textString = Berkelium::WideToUTF8(wtitle);
    std::string title (textString.data(), textString.length());
    Berkelium::stringUtil_free(textString);
    SILOG(webview,detailed,"onTitleChanged"<<title);
}
void WebView::onTooltipChanged(Berkelium::Window*, WideString wtext) {
    UTF8String textString = Berkelium::WideToUTF8(wtext);
    std::string tooltip (textString.data(), textString.length());
    Berkelium::stringUtil_free(textString);
    SILOG(webview,detailed,"onTooltipChanged"<<tooltip);
}

void WebView::onLoad(Berkelium::Window*) {
    SILOG(webview,detailed,"onLoad");
}

void WebView::onConsoleMessage(Berkelium::Window *win, WideString wmessage,
                               WideString wsourceId, int line_no) {
    UTF8String textString = Berkelium::WideToUTF8(wmessage);
    UTF8String sourceString = Berkelium::WideToUTF8(wsourceId);
    std::string message (textString.data(), textString.length());
    std::string sourceId (sourceString.data(), sourceString.length());
    Berkelium::stringUtil_free(textString);
    Berkelium::stringUtil_free(sourceString);

    static String exc_prefix = "Uncaught";
    if (message.substr(0, exc_prefix.size()) == exc_prefix) {
        SILOG(webview,error,"WebView Exception: " << message << " at file " << sourceId << ":" << line_no);
        if (this->mExceptionHandler != NULL) {
            std::vector<boost::any> params;
            params.push_back(Invokable::asAny(String(message)));
            params.push_back(Invokable::asAny(String(sourceId)));
            params.push_back(Invokable::asAny(line_no));
            this->mExceptionHandler->invoke(params);
        }
    }
    else {
        SILOG(webview,detailed,"onConsoleMessage " << message << " at file " << sourceId << ":" << line_no);
    }
}
void WebView::onScriptAlert(Berkelium::Window *win, WideString message,
                            WideString defaultValue, URLString url,
                            int flags, bool &success, WideString &value) {
    // FIXME: C++ string conversion functions are a pile of garbage.
    UTF8String textString = Berkelium::WideToUTF8(message);
    SILOG(webview,detailed,"onScriptAlert "<<textString);
}

Berkelium::Rect WebView::getBorderlessRect(Ogre::HardwarePixelBufferSharedPtr pixelBuffer) const {
    Berkelium::Rect pixelBufferRect;
    pixelBufferRect.mHeight=pixelBuffer->getHeight()-mBorderTop-mBorderBottom;
    pixelBufferRect.mWidth=pixelBuffer->getWidth()-mBorderLeft-mBorderRight;
    pixelBufferRect.mTop=0;
    pixelBufferRect.mLeft=0;
    return pixelBufferRect;
}

Berkelium::Rect WebView::getBorderedRect(const Berkelium::Rect& orig) const {
    return orig.translate(mBorderLeft, mBorderTop);
}

void WebView::blitNewImage(
    HardwarePixelBufferSharedPtr pixelBuffer,
    const unsigned char* srcBuffer, const Berkelium::Rect& srcRect,
    const Berkelium::Rect& copyRect,
    bool updateAlphaCache
) {
    Berkelium::Rect destRect = getBorderlessRect(pixelBuffer);

    destRect = destRect.intersect(srcRect);
    destRect = destRect.intersect(copyRect);

    // Find the location of the rect in the source data. It needs to be offset
    // since newPixelBuffer doesn't necessarily cover the entire image.
    Berkelium::Rect destRectInSrc = destRect.translate(-srcRect.left(), -srcRect.top());

    Berkelium::Rect borderedDestRect = getBorderedRect(destRect);

    // For new data, because of the way HardwarePixelBuffers work (no copy
    // subregions from *Memory*), we have to copy each line over individually
    for(int y = 0; y < destRectInSrc.height(); y++) {
        pixelBuffer->blitFromMemory(
            Ogre::PixelBox(
                destRectInSrc.width(), 1, 1, Ogre::PF_A8R8G8B8,
                const_cast<unsigned char*>(srcBuffer + (srcRect.width()*(destRectInSrc.top()+y) + destRectInSrc.left()) * 4)
            ),
            Ogre::Box(borderedDestRect.left(), borderedDestRect.top() + y, borderedDestRect.right(), borderedDestRect.top() + y + 1)
        );

        if(updateAlphaCache && isWebViewTransparent && !usingMask && ignoringTrans && alphaCache && alphaCachePitch) {
            for(int x = 0; x < destRectInSrc.width(); x++)
                alphaCache[ alphaCachePitch*(borderedDestRect.top()+y) + (borderedDestRect.left()+x) ] =
                    srcBuffer[ (srcRect.width()*(destRectInSrc.top()+y) + destRectInSrc.left()+x)*4 + 3 ];
        }
    }
}

void WebView::blitScrollImage(
    HardwarePixelBufferSharedPtr pixelBuffer,
    const Berkelium::Rect& scrollOrigRect,
    int scroll_dx, int scroll_dy,
    bool updateAlphaCache
) {
    assert(scroll_dx != 0 || scroll_dy != 0);

    Berkelium::Rect scrolledRect = scrollOrigRect.translate(scroll_dx, scroll_dy);

    Berkelium::Rect scrolled_shared_rect = scrollOrigRect.intersect(scrolledRect);
    // Only do scrolling if they have non-zero intersection
    if (scrolled_shared_rect.width() == 0 || scrolled_shared_rect.height() == 0) return;
    Berkelium::Rect shared_rect = scrolled_shared_rect.translate(-scroll_dx, -scroll_dy);

    size_t width = shared_rect.width();
    size_t height = shared_rect.height();

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
    // For some reason, copying between hardware buffers on Linux doesn't work
    // properly, and its not even card specific (both ATI and NVidia cards have
    // this problem). Instead, we have to copy between GPU and RAM on Linux.

    {
        char* scrollbuf = new char[width*height*4];

        Berkelium::Rect borderedScrollRect = getBorderedRect(shared_rect);
        Berkelium::Rect borderedScrolledRect = getBorderedRect(scrolled_shared_rect);

        pixelBuffer->blitToMemory(
            Ogre::Box(borderedScrollRect.left(), borderedScrollRect.top(), borderedScrollRect.right(), borderedScrollRect.bottom()),
            Ogre::PixelBox(borderedScrollRect.width(), borderedScrollRect.height(), 1, PF_BYTE_BGRA, (void*)scrollbuf)
        );

        pixelBuffer->blitFromMemory(
            Ogre::PixelBox(borderedScrollRect.width(), borderedScrollRect.height(), 1, PF_BYTE_BGRA, (void*)scrollbuf),
            Ogre::Box(borderedScrolledRect.left(), borderedScrolledRect.top(), borderedScrolledRect.right(), borderedScrolledRect.bottom())
        );

        delete scrollbuf;
    }

#else
    Ogre::TexturePtr shadow = Ogre::TextureManager::getSingleton().createManual(
        "_ _ webview scroll buffer _ _",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Ogre::TEX_TYPE_2D,
        Bitwise::firstPO2From(width), Bitwise::firstPO2From(height), 1, 1,
        PF_BYTE_BGRA
    );
    {
        HardwarePixelBufferSharedPtr shadowBuffer = shadow->getBuffer();

        Berkelium::Rect borderedScrollRect = getBorderedRect(shared_rect);
        Berkelium::Rect borderedScrolledRect = getBorderedRect(scrolled_shared_rect);

        shadowBuffer->blit(
            pixelBuffer,
            Ogre::Box( borderedScrollRect.left(), borderedScrollRect.top(), borderedScrollRect.right(), borderedScrollRect.bottom()),
            Ogre::Box(0,0,width,height));

        pixelBuffer->blit(
            shadowBuffer,
            Ogre::Box(0,0,width,height),
            Ogre::Box(borderedScrolledRect.left(), borderedScrolledRect.top(), borderedScrolledRect.right(), borderedScrolledRect.bottom()));
    }
    Ogre::ResourcePtr shadowResource(shadow);
    Ogre::TextureManager::getSingleton().remove(shadowResource);

#endif

    // FIXME We should be updating the alpha cache here, but that would require
    // pulling data back from the card...
    //if(updateAlphaCache && isWebViewTransparent && !usingMask && ignoringTrans && alphaCache && alphaCachePitch) {
    //}
}

void WebView::onPaint(Berkelium::Window*win,
                      const unsigned char*srcBuffer, const Berkelium::Rect& srcRect,
                      size_t num_copy_rects, const Berkelium::Rect *copy_rects,
                      int dx, int dy, const Berkelium::Rect& scroll_rect) {
    TexturePtr texture = backingTexture.isNull()?viewTexture:backingTexture;
    HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();

    // Now, we first handle scrolling. We need to do this first since it
    // requires shifting existing data, some of which will be overwritten by
    // the regular dirty rect update.
    if (dx != 0 || dy != 0)
        blitScrollImage(pixelBuffer, scroll_rect, dx, dy, false);

    for (size_t i = 0; i < num_copy_rects; i++)
        blitNewImage(pixelBuffer, srcBuffer, srcRect, copy_rects[i], true);

    if (!backingTexture.isNull())
        compositeWidgets(win);
}
void WebView::onCrashed(Berkelium::Window*) {
    SILOG(webview,error,"WebView crashed: " << viewName);
    restartPage();
}
void WebView::onResponsive(Berkelium::Window*) {
    SILOG(webview,error,"WebView became responsive again: " << viewName);
    mUnresponsive = false;
}
void WebView::onUnresponsive(Berkelium::Window*) {
    SILOG(webview,error,"WebView unresponsive: " << viewName);
    // Start a timer and flag this as unresponsive. In the callback
    // we'll check the flag and forcibly kill the webview if it
    // doesn't seem to be coming back.
    mUnresponsive = true;
    mContext->mainStrand->post(
        Duration::seconds(10),
        std::tr1::bind(&WebView::handleUnresponsiveTimeout, this, livenessToken())
    );
}

void WebView::handleUnresponsiveTimeout(Liveness::Token alive) {
    Liveness::Lock locked(alive);
    if (!locked)
        return;

    if (!mUnresponsive) return;
    restartPage();
}

void WebView::restartPage() {
    // Try to cleanup and reinitialize everything.
    cleanupWebView();
    createWebView(true);

    // All delegates should still be fine since they are just sitting
    // in a map -- they don't get registered with the webview.

    // Get us loading the right page again.
    this->loadURL(viewURL);

    // Make sure webview's transparency setting is correct
    this->setTransparent(isWebViewTransparent);
}

void WebView::onCreatedWindow(Berkelium::Window*, Berkelium::Window*newwin) {
    std::string name;
    {
        static int i = 0;
        i++;
        std::ostringstream os;
        os << "_blank" << i;
        name = os.str();
    }
    SILOG(webview,detailed,"onCreatedWindow "<<name);

    Berkelium::Rect r;
    r.mLeft = 0;
    r.mTop = 0;
    r.mWidth = 600;
    r.mHeight = 400;
    Berkelium::Widget *wid = newwin->getWidget();
    if (wid && wid->getRect().mWidth > 0 && wid->getRect().mHeight > 0) {
        r = wid->getRect();
    }
    WebViewManager::getSingleton().createWebViewPopup(
        mContext,
        name, r.width(), r.height(),
        OverlayPosition(r.left(), r.top()),
        newwin, postingStrand,TIER_MIDDLE,
        overlay?overlay->viewport:WebViewManager::getSingleton().defaultViewport);
}

void WebView::onWidgetCreated(Berkelium::Window *win, Berkelium::Widget *newWidget, int zIndex) {
    SILOG(webview,detailed,"onWidgetCreated");
}
void WebView::onWidgetDestroyed(Berkelium::Window *win, Berkelium::Widget *wid) {
    std::map<Berkelium::Widget*,TexturePtr>::iterator where=widgetTextures.find(wid);
    if (where!=widgetTextures.end()) {
        if (!where->second.isNull()) {
            ResourcePtr mfd(where->second);
            widgetTextures.erase(where);
            TextureManager::getSingleton().remove(mfd);
        }else widgetTextures.erase(where);
    }
    if (widgetTextures.size()==0&&!backingTexture.isNull()) {

        ResourcePtr mfd(backingTexture);
        viewTexture->getBuffer()->blit(backingTexture->getBuffer(),
                                       Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()),
                                       Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()));
        backingTexture.setNull();
        TextureManager::getSingleton().remove(mfd);
    }
    SILOG(webview,detailed,"onWidgetDestroyed");
}
void WebView::onWidgetResize(Berkelium::Window *win, Berkelium::Widget *widg, int w, int h) {
    TexturePtr tmp (TextureManager::getSingleton().createManual(
                        "Widget"+UUID::random().toString(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        TEX_TYPE_2D, Bitwise::firstPO2From(w), Bitwise::firstPO2From(h), 0, PF_BYTE_BGRA,
        TU_DYNAMIC, this));
    TexturePtr old=widgetTextures[widg];
    if (!old.isNull()) {
        int minwid=old->getWidth()<(unsigned int)w?old->getWidth():w;
        int minhei=old->getHeight()<(unsigned int)h?old->getHeight():h;
        tmp->getBuffer()->blit(old->getBuffer(),Ogre::Box(0,0,minwid,minhei),Ogre::Box(0,0,minwid,minhei));
/*        viewBuffer->blit(backingBuffer,
                         Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()),
                         Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()));
*/
    }
    widgetTextures[widg]=tmp;
    if (!old.isNull()){
        ResourcePtr mfd(old);
        old.setNull();
        TextureManager::getSingleton().remove(mfd);
    }
    SILOG(webview,detailed,"onWidgetResize");
}
void WebView::onWidgetMove(Berkelium::Window *win, Berkelium::Widget *widg, int x, int y) {
    SILOG(webview,detailed,"onWidgetMove");
    if (!backingTexture.isNull()) {
        compositeWidgets(win);
    }
}

void WebView::compositeWidgets(Berkelium::Window*win) {
    if (viewTexture.isNull()||backingTexture.isNull()) {
        SILOG(webview,fatal,"View or backing texture null during ocmpositing step");
        assert(false);
    }else {
        viewTexture->getBuffer()->blit(backingTexture->getBuffer(),
                                       Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()),
                                       Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()));
        int count=0;
        for (Berkelium::Window::BackToFrontIter i=win->backIter(),ie=win->backEnd();
             i!=ie;
             ++i,++count) {
            SILOG(webkit,warning,"Widget count: "<<count);
            std::map<Berkelium::Widget*,Ogre::TexturePtr>::iterator where=widgetTextures.find(*i);
            if (where!=widgetTextures.end()) {
                SILOG(webkit,warning,"Widget found: "<<*i);
                if (!where->second.isNull()){
                    Berkelium::Rect rect=(*i)->getRect();
                    Berkelium::Rect srcRect = rect;
                    SILOG(webkit,warning,"Blitting to "<<rect.left()<<","<<rect.top()<<" - "<<rect.right()<<","<<rect.bottom());
                    Berkelium::Rect windowRect;
					windowRect.mTop = 0;
					windowRect.mLeft = 0;
					windowRect.mHeight = viewTexture->getBuffer()->getHeight();
					windowRect.mWidth = viewTexture->getBuffer()->getWidth();
					rect = rect.intersect(windowRect);
					srcRect.mTop = rect.mTop - srcRect.mTop;
					srcRect.mLeft = rect.mLeft - srcRect.mLeft;
					srcRect.mHeight = rect.height();
					srcRect.mWidth = rect.width();
                    viewTexture->getBuffer()->blit(where->second->getBuffer(),
                                                   Ogre::Box(srcRect.left(),srcRect.top(),srcRect.right(),srcRect.bottom()),
                                                   Ogre::Box(rect.left(),rect.top(),rect.right(),rect.bottom()));
                }else {
                    widgetTextures.erase(where);
                }
            }
        }

    }
}

void WebView::onWidgetPaint(
        Berkelium::Window *win,
        Berkelium::Widget *wid,
        const unsigned char *srcBuffer,
        const Berkelium::Rect &srcRect,
        size_t num_copy_rects,
        const Berkelium::Rect *copy_rects,
        int dx, int dy,
        const Berkelium::Rect &scroll_rect) {
    return;
    if (backingTexture.isNull()&&!viewTexture.isNull()) {
        backingTexture=TextureManager::getSingleton().createManual(
            "B"+getViewTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            TEX_TYPE_2D, viewTexture->getWidth(), viewTexture->getHeight(), 0, PF_BYTE_BGRA,
		TU_DYNAMIC, this);
        HardwarePixelBufferSharedPtr viewBuffer = viewTexture->getBuffer();
        HardwarePixelBufferSharedPtr backingBuffer = backingTexture->getBuffer();
        backingBuffer->blit(viewBuffer,
                            Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()),
                            Ogre::Box(0,0,viewTexture->getWidth(),viewTexture->getHeight()));
    }
    TexturePtr widgetTex=widgetTextures[wid];
    if (widgetTex.isNull()) {
        onWidgetResize(win,wid,wid->getRect().width(),wid->getRect().height());
        widgetTex=widgetTextures[wid];
    }

    HardwarePixelBufferSharedPtr pixelBuffer = widgetTex->getBuffer();
    if (dx != 0 || dy != 0)
        blitScrollImage(pixelBuffer, scroll_rect, dx, dy, false);

    for (size_t i = 0; i < num_copy_rects; i++)
        blitNewImage(pixelBuffer, srcBuffer, srcRect, copy_rects[i], false);

    compositeWidgets(win);
    SILOG(webview,detailed,"onWidgetPaint");
}

namespace {

Berkelium::Script::Variant translateAnyToVariant(const boost::any& a) {
    if (a.empty()) return Berkelium::Script::Variant();

    if (a.type() == typeid(bool)) {
        return Berkelium::Script::Variant(boost::any_cast<bool>(a));
    }
    else if (a.type() == typeid(int8)) {
        return Berkelium::Script::Variant((int)boost::any_cast<int8>(a));
    }
    else if (a.type() == typeid(uint8)) {
        return Berkelium::Script::Variant((int)boost::any_cast<uint8>(a));
    }
    else if (a.type() == typeid(int16)) {
        return Berkelium::Script::Variant((int)boost::any_cast<int16>(a));
    }
    else if (a.type() == typeid(uint16)) {
        return Berkelium::Script::Variant((int)boost::any_cast<uint16>(a));
    }
    else if (a.type() == typeid(int32)) {
        return Berkelium::Script::Variant((int)boost::any_cast<int32>(a));
    }
    else if (a.type() == typeid(uint32)) {
        return Berkelium::Script::Variant((int)boost::any_cast<uint32>(a));
    }
    else if (a.type() == typeid(float32)) {
        return Berkelium::Script::Variant((float64)boost::any_cast<float32>(a));
    }
    else if (a.type() == typeid(float64)) {
        return Berkelium::Script::Variant((float64)boost::any_cast<float64>(a));
    }
    else if (a.type() == typeid(String)) {
        String a_str = boost::any_cast<String>(a);
        return Berkelium::Script::Variant(a_str.c_str());
    }

    // Default fallback case just returns undefined
    return Berkelium::Script::Variant();
}

}

void WebView::onJavascriptCallback(Berkelium::Window *win, void* replyMsg, URLString origin, WideString funcName, Berkelium::Script::Variant *args, size_t numArgs) {
    boost::any result;
    if (numArgs < 1) {
        if (replyMsg)
            win->synchronousScriptReturn(replyMsg, translateAnyToVariant(result));
        return;
    }
    const Berkelium::Script::Variant &name = args[0];
    args++;
    numArgs--;
    UTF8String nameUTF8 = Berkelium::WideToUTF8(name.toString());
    std::string nameStr(nameUTF8.get<std::string>());
    Berkelium::stringUtil_free(nameUTF8);
    SILOG(webview,detailed,"Handling web view event " << nameStr);
    std::map<std::string, JSDelegate>::iterator i = delegateMap.find(nameStr);

    if(i != delegateMap.end()) {
        JSArguments argVector;
        std::vector<std::string*> argStorage;

        for (size_t j=0;j!=numArgs;++j) {
            UTF8String temp = Berkelium::WideToUTF8(args[j].toString());
            std::string* s = new std::string(temp.get<std::string>());
            argStorage.push_back(s);
            //argVector.push_back(JSArgument(temp.data(), temp.length()));
            argVector.push_back(JSArgument(s->data(), s->length()));
        }
        result = dispatchToDelegate(nameStr, argVector);
        for (size_t j=0;j<argStorage.size();j++) {
            delete(argStorage[j]);
        }
    }
    if (replyMsg)
        win->synchronousScriptReturn(replyMsg, translateAnyToVariant(result));
}

#endif //HAVE_BERKELIUM

boost::any WebView::invoke(std::vector<boost::any>& params)
{
    SILOG(ogre,detailed,"Inside WebView::invoke()");
  std::string name="";
  /*Check the first param */

  if(Invokable::anyIsString(params[0]))
  {
      name = Invokable::anyAsString(params[0]);
  }

  // This will bind a callback for the graphics to the script
  if(name == "bind")
  {
    // we need to bind a function to some event.
    // second argument is the event name
      SILOG(ogre,detailed,"In WebView::invoke");
    std::string event = "";
    if(Invokable::anyIsString(params[1]))
    {
        event = Invokable::anyAsString(params[1]);
    }

    // the third argument has to be a function ptr
    //This function would take any

    //just _1, _2 for now
        Invokable* invokable = Invokable::anyAsInvokable(params[2]);
    bind(event, std::tr1::bind(&WebView::translateParamsAndInvoke, this, invokable, _1, _2));
    Invokable* inv_result = this;
    return Invokable::asAny(inv_result);

  }

  // This will write message from the script to the graphics window
  if(name == "eval")
  {
      if (params.size() != 2) {
          SILOG(webview,error,"[WEBVIEW] Invoking 'eval' expects 2 arguments." );
          return boost::any();
      }
      if (!Invokable::anyIsString(params[1])){
          SILOG(webview,error,"[WEBVIEW] Invoking 'eval' expects string argument." );
          return boost::any();
      }

      std::string jsscript = Invokable::anyAsString(params[1]);
      if (jsscript.empty()) return boost::any();

      // whenthe jsscript is not empty
      evaluateJS(jsscript);

      // FIXME return value
      return boost::any();
  }

  if(name == "show") {
      this->show();
  }

  if(name == "hide") {
      this->hide();
  }

  if (name == "focus")
      WebViewManager::getSingleton().focusWebView(this);

  if (name == "onException") {
      if (params.size() != 2) {
          SILOG(webview,error,"Invoking 'onException' expects 2 arguments." );
          return boost::any();
      }
      if (!Invokable::anyIsInvokable(params[1])){
          SILOG(webview,error,"Invoking 'onException' expects function argument." );
          return boost::any();
      }

      Invokable* exchandler = Invokable::anyAsInvokable(params[1]);
      this->mExceptionHandler = exchandler;

      // FIXME return value
      return boost::any();

  }

    if (name == "onNavigate") {
        if (params.size() != 2) {
            SILOG(webview,error,"Invoking 'onNavigate' expects 2 arguments." );
            return boost::any();
        }
        if (!Invokable::anyIsInvokable(params[1])){
            SILOG(webview,error,"Invoking 'onNavigate' expects function argument." );
            return boost::any();
        }

        Invokable* handler = Invokable::anyAsInvokable(params[1]);

        setNavigatedCallback(
            std::tr1::bind(
                &WebView::forwardOnNavigateToInvokable, this,
                handler,
                std::tr1::placeholders::_1
            )
        );
    }

    if (name == "url") {
        return Invokable::asAny(viewURL);
    }

    return boost::any();
}

boost::any WebView::translateParamsAndInvoke(Invokable* _invokable, WebView* wv, const JSArguments& args)
{
  std::vector<boost::any> params;
  // Do the translation here
  for(unsigned int i = 0; i < args.size(); i++)
  {
    const char* s = args[i].begin();

    params.push_back(Invokable::asAny(String(s)));
  }

  //After translation
  boost::any result = _invokable->invoke(params);
  return result;
}

void WebView::forwardOnNavigateToInvokable(Invokable* _invokable, const String& url) {
    std::vector<boost::any> params;
    params.push_back(Invokable::asAny(String(url)));
    _invokable->invoke(params);
}


const WebView::WebViewBorderSize WebView::mDefaultBorder(2,2,25,2);

}
}
