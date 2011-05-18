/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  WebViewManager.hpp
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

#ifndef _SIRIKATA_GRAPHICS_WEBVIEWMANAGER_HPP_
#define _SIRIKATA_GRAPHICS_WEBVIEWMANAGER_HPP_

#include <sirikata/ogre/OgreHeaders.hpp>
#include "Ogre.h"
#include "ViewportOverlay.hpp"
#include <sirikata/ogre/input/InputEvents.hpp>
#include <sirikata/ogre/input/InputManager.hpp>
#include <sirikata/ogre/task/EventManager.hpp>
#include "WebView.hpp"

namespace Sirikata {


typedef Sirikata::DataReference<const char*> JSArgument;
typedef std::vector<JSArgument> JSArguments;
typedef JSArguments::const_iterator JSIter;


namespace Graphics {

/**
* Enumerates internal mouse button IDs. Used by WebViewManager::injectMouseDown, WebViewManager::injectMouseUp
*/
enum MouseButtonID
{
	LeftMouseButton = 0,
	RightMouseButton = 1,
	MiddleMouseButton = 2,
        ScrollUpButton = 3,
        ScrollDownButton = 4,
        UnknownMouseButton = 0xFFFF
};

struct WebViewCoord {
    int x;
    int y;

    WebViewCoord(int _x, int _y)
     : x(_x), y(_y)
    {
    }
};


/**
* Supreme dictator and Singleton: WebViewManager
*
* The class you will need to go to for all your WebView-related needs.
*/
class WebViewManager : public Ogre::Singleton<WebViewManager>
{
public:
	/**
	* Creates the WebViewManager singleton.
	* @param defaultViewport The default Ogre::Viewport to place WebViews
	*                        in. This can be overriden per-WebView via the
	*                        last parameter of
	*                        WebViewManager::createWebView.
	* @inputMgr input manager to gather input from
        * @param binDirectory Path to the binary directory, i.e. the one that
	*                     holds the berkelium binary, liblibberkelium, etc.
	* @param baseDirectory The relative path to your base directory. This
	*                      directory is used by WebView::loadFile and
	*                      WebView::loadHTML (to resolve relative URLs).
	* @throws Ogre::Exception::ERR_INTERNAL_ERROR When initialization fails
	*/
    WebViewManager(Ogre::Viewport* defaultViewport, Input::InputManager* inputMgr, const std::string& binDirectory, const std::string& baseDirectory);

	/**
	* Destroys any active WebViews, the WebViewMouse singleton (if instantiated).
	*/
	~WebViewManager();

	/**
	* Gets the WebViewManager Singleton.
	*
	* @return	A reference to the WebViewManager Singleton.
	*
	* @throws	Ogre::Exception::ERR_RT_ASSERTION_FAILED	Throws this if WebViewManager has not been instantiated yet.
	*/
	static WebViewManager& getSingleton();

	/**
	* Gets the WebViewManager Singleton as a pointer.
	*
	* @return	If the WebViewManager has been instantiated, returns a pointer to the WebViewManager Singleton,
	*			otherwise this returns 0.
	*/
	static WebViewManager* getSingletonPtr();

	/**
	* Gives each active WebView a chance to update, each may or may not update their internal textures
	* based on various conditions.
	*/
	void Update();

	/**
	* Creates a WebView.
	*/
    WebView* createWebView(const std::string &webViewName, const std::string& webViewType,unsigned short width, unsigned short height,
	        const OverlayPosition &webViewPosition,	bool asyncRender = false, int maxAsyncRenderRate = 70,
	        Tier tier = TIER_MIDDLE, Ogre::Viewport* viewport = 0,
	        const WebView::WebViewBorderSize& border = WebView::mDefaultBorder);

#ifdef HAVE_BERKELIUM
	/**
	* Creates a WebView from a given Berkelium::Window.
	*/
	WebView* createWebViewPopup(const std::string &webViewName, unsigned short width, unsigned short height, const OverlayPosition &webViewPosition,
		Berkelium::Window *newwin, Tier tier = TIER_MIDDLE, Ogre::Viewport* viewport = 0);
#endif
	/**
	* Creates a WebViewMaterial. WebViewMaterials are just like WebViews except that they lack a movable overlay element.
	* Instead, you handle the material and apply it to anything you like. Mouse input for WebViewMaterials should be
	* injected via the WebView::injectMouse_____ API calls instead of the global WebViewManager::injectMouse_____ calls.
	*/
	WebView* createWebViewMaterial(const std::string &webViewName, unsigned short width, unsigned short height,
		bool asyncRender = false, int maxAsyncRenderRate = 70, Ogre::FilterOptions texFiltering = Ogre::FO_ANISOTROPIC);

	/**
	* Retrieve a pointer to a WebView by name.
	*
	* @param	webViewName	The name of the WebView to retrieve.
	*
	* @return	If the WebView is found, returns a pointer to the WebView, otherwise returns 0.
	*/
	WebView* getWebView(const std::string &webViewName);

	/**
	* Destroys a WebView.
	*
	* @param	webViewName	The name of the WebView to destroy.
	*/
	void destroyWebView(const std::string &webViewName);

	/**
	* Destroys a WebView.
        * \param webViewToDestroy pointer to the WebView to be destroyed
	*/
	void destroyWebView(WebView* webViewToDestroy);

	/**
	* Resets the positions of all WebViews to their default positions. (not applicable to WebViewMaterials)
	*/
	void resetAllPositions();

	/**
	* Checks whether or not a WebView is focused/selected. (not applicable to WebViewMaterials)
	*
	* @return	True if a WebView is focused, False otherwise.
	*/
	bool isAnyWebViewFocused();

	/**
	* Gets the currently focused/selected WebView. (not applicable to WebViewMaterials)
	*
	* @return	A pointer to the WebView that is currently focused, returns 0 if none are focused.
	*/
	WebView* getFocusedWebView();

    /**
     * Injects the mouse's current position into WebViewManager. Used to generally keep track of where the mouse
     * is for things like moving WebViews around, telling the internal pages of each WebView where the mouse is and
     * where the user has clicked, etc. (not applicable to WebViewMaterials)
     *
     * @param	coord	The coordinates of the mouse.
     *
     * @return	Returns True if the injected coordinate is over a WebView, False otherwise.
     */
    bool injectMouseMove(const WebViewCoord& coord);

    /**
     * Injects mouse wheel events into WebViewManager. Used to scroll the focused WebView. (not applicable to WebViewMaterials)
     *
     * @param	relScroll	The relative Scroll-Value of the mouse.
     *
     * @note
     *	To inject this using OIS: on a OIS::MouseListener::MouseMoved event, simply
     *	inject "arg.state.Z.rel" of the "MouseEvent".
     *
     * @return	Returns True if the mouse wheel was scrolled while a WebView was focused, False otherwise.
     */
    bool injectMouseWheel(const WebViewCoord& relScroll);

	/**
	* Injects mouse down events into WebViewManager. Used to know when the user has pressed a mouse button
	* and which button they used. (not applicable to WebViewMaterials)
	*
	* @param	buttonID	The ID of the button that was pressed. Left = 0, Right = 1, Middle = 2.
	*
	* @return	Returns True if the mouse went down over a WebView, False otherwise.
	*/
	bool injectMouseDown(int buttonID);

	/**
	* Injects mouse up events into WebViewManager. Used to know when the user has released a mouse button
	* and which button they used. (not applicable to WebViewMaterials)
	*
	* @param	buttonID	The ID of the button that was released. Left = 0, Right = 1, Middle = 2.
	*
	* @return	Returns True if the mouse went up while a WebView was focused, False otherwise.
	*/
	bool injectMouseUp(int buttonID);

    bool injectCut();
    bool injectCopy();
    bool injectPaste();

    bool injectKeyEvent(bool pressed, bool repeat, Input::Modifier mod, Input::KeyButton button);

	bool injectTextEvent(std::string utf8);

	/**
	* De-Focuses any currently-focused WebViews.
	*/
	void deFocusAllWebViews();

	bool focusWebView(WebView* selection);

	void setDefaultViewport(Ogre::Viewport* newViewport);


    enum NavigationAction {
        NavigateNewTab,
        NavigateBack,
        NavigateForward,
        NavigateRefresh,
        NavigateHome,
        NavigateGo,
        NavigateCommand,
        NavigateHelp,
        NavigateDelete
    };
    void navigate(NavigationAction action);
    void navigate(NavigationAction action, const String& arg);

    const std::string &getBaseDir() const {
        return baseDirectory;
    }

protected:
	friend class WebView; // Our very close friend <3

	typedef std::map<std::string,WebView*> WebViewMap;
    WebViewMap activeWebViews;
    WebView* focusedWebView;
    WebView* tooltipWebView;
    WebView* tooltipParent;
    WebView* chromeWebView;
    WebView* focusedNonChromeWebView;

	Ogre::Viewport* defaultViewport;
	int mouseXPos, mouseYPos;
    bool isDragging;
    bool isResizing;
	unsigned short zOrderCounter;
	Ogre::Timer tooltipTimer;
	double lastTooltip, tooltipShowTime;
	bool isDraggingFocusedWebView;
    std::string baseDirectory;

#ifdef HAVE_BERKELIUM
    Berkelium::Context *bkContext;
#endif

	WebView* getTopWebView(int x, int y);
	void onResizeTooltip(WebView* WebView, const JSArguments& args);
	void handleTooltip(WebView* tooltipParent, const std::wstring& tipText);
	void handleRequestDrag(WebView* caller);

    Input::InputManager* mInputManager;

    /** Callback which generates WebView events due to a script in a WebView.  This is the portal
     *  from Javascript into the InputEvent system.
     *
     *  We expose an event() function on the WebView's DOM's Client object.  A script may then call it
     *  to raise an event.  The call
     *    Client.event(name, some, other, args)
     *  will generate a WebView event with the parameters
     *    WebViewEvent.webView = (the WebView that generated the callback)
     *    WebViewEvent.name = name
     *    WebViewEvent.args = [some, other, args]
     *  Note that if the first argument is not a string, then no event will be generated.
     */
    void onRaiseWebViewEvent(WebView* webview, const JSArguments& args);
public:
	Sirikata::Task::EventResponse onMouseMove(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onMousePressed(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onMouseDrag(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onMouseClick(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onButton(Sirikata::Task::EventPtr evt);
	Sirikata::Task::EventResponse onKeyTextInput(Sirikata::Task::EventPtr evt);
};

}
}

#endif
