/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  OgreSystemMouseHandler.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#include "OgreSystemMouseHandler.hpp"

#include "OgreSystem.hpp"
#include <sirikata/ogre/Camera.hpp>
#include <sirikata/ogre/Lights.hpp>
#include <sirikata/ogre/Entity.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/ogre/input/InputEvents.hpp>

#include <sirikata/ogre/input/SDLInputDevice.hpp>
#include <sirikata/ogre/input/SDLInputManager.hpp>
#include <sirikata/ogre/input/InputManager.hpp>

#include <sirikata/core/task/Time.hpp>
#include <set>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <sirikata/ogre/OgreConversions.hpp>

#include "ProxyEntity.hpp"

namespace Sirikata {
namespace Graphics {
using namespace Input;
using namespace Task;
using namespace std;

Vector3f pixelToDirection(Camera *cam, float xPixel, float yPixel) {
    float xRadian, yRadian;
    //pixelToRadians(cam, xPixel/2, yPixel/2, xRadian, yRadian);
    xRadian = sin(cam->getOgreCamera()->getFOVy().valueRadians()*.5) * cam->getOgreCamera()->getAspectRatio() * xPixel;
    yRadian = sin(cam->getOgreCamera()->getFOVy().valueRadians()*.5) * yPixel;

    Quaternion orient = cam->getOrientation();
    return Vector3f(-orient.zAxis()*cos(cam->getOgreCamera()->getFOVy().valueRadians()*.5) +
                    orient.xAxis() * xRadian +
                    orient.yAxis() * yRadian);
}

void OgreSystemMouseHandler::mouseOverWebView(Camera *cam, Time time, float xPixel, float yPixel, bool mousedown, bool mouseup) {
    Vector3d pos = cam->getPosition();
    Vector3f dir (pixelToDirection(cam, xPixel, yPixel));
    Ogre::Ray traceFrom(toOgre(pos, mParent->getOffset()), toOgre(dir));
    ProxyObjectPtr obj(mMouseDownObject.lock());
    Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
    if (mMouseDownTri.intersected && ent) {
        Entity *me = ent;
        IntersectResult res = mMouseDownTri;
        res.distance = 1.0e38;
/* fixme */
        Ogre::Node *node = ent->getSceneNode();
        const Ogre::Vector3 &position = node->_getDerivedPosition();
        const Ogre::Quaternion &orient = node->_getDerivedOrientation();
        const Ogre::Vector3 &scale = node->_getDerivedScale();
        Triangle newt = res.tri;
        newt.v1.coord = (orient * (newt.v1.coord * scale)) + position;
        newt.v2.coord = (orient * (newt.v2.coord * scale)) + position;
        newt.v3.coord = (orient * (newt.v3.coord * scale)) + position;
/* */
        OgreMesh::intersectTri(OgreMesh::transformRay(ent->getSceneNode(), traceFrom), res, &newt, true); // &res.tri
    }
}

ProxyEntity* OgreSystemMouseHandler::hoverEntity (Camera *cam, Time time, float xPixel, float yPixel, bool mousedown, int *hitCount,int which, Vector3f* hitPointOut, SpaceObjectReference ignore) {
    Vector3d pos = cam->getPosition();
    Vector3f dir (pixelToDirection(cam, xPixel, yPixel));
    SILOG(input,detailed,"OgreSystemMouseHandler::hoverEntity: X is "<<xPixel<<"; Y is "<<yPixel<<"; pos = "<<pos<<"; dir = "<<dir);

    double dist;
    Vector3f normal;
    IntersectResult res;
    int subent=-1;
    Ogre::Ray traceFrom(toOgre(pos, mParent->getOffset()), toOgre(dir));
    ProxyEntity *mouseOverEntity = mParent->internalRayTrace(traceFrom, false, *hitCount, dist, normal, subent, &res, mousedown, which, ignore);
    if (mousedown && mouseOverEntity) {
        ProxyEntity *me = mouseOverEntity;
        if (me) {
            mMouseDownTri = res;
            mMouseDownObject = me->getProxyPtr();
            mMouseDownSubEntity = subent;
        }
    }
    if (mouseOverEntity) {
        if (hitPointOut != NULL) *hitPointOut = Vector3f(pos) + dir.normal()*dist;
        return mouseOverEntity;
    }
    return NULL;
}

bool OgreSystemMouseHandler::recentMouseInRange(float x, float y, float *lastX, float *lastY) {
    float delx = x-*lastX;
    float dely = y-*lastY;

    if (delx<0) delx=-delx;
    if (dely<0) dely=-dely;
    if (delx>.03125||dely>.03125) {
        *lastX=x;
        *lastY=y;

        return false;
    }
    return true;
}

SpaceObjectReference OgreSystemMouseHandler::pick(Vector2f p, int direction, const SpaceObjectReference& ignore, Vector3f* hitPointOut) {
    if (!mParent||!mParent->mPrimaryCamera) SpaceObjectReference::null();

    Camera *camera = mParent->mPrimaryCamera;
    Time time = mParent->simTime();

    int numObjectsUnderCursor=0;
    ProxyEntity *mouseOver = hoverEntity(camera, time, p.x, p.y, true, &numObjectsUnderCursor, mWhichRayObject, hitPointOut, ignore);
    if (recentMouseInRange(p.x, p.y, &mLastHitX, &mLastHitY)==false||numObjectsUnderCursor!=mLastHitCount)
        mouseOver = hoverEntity(camera, time, p.x, p.y, true, &mLastHitCount, mWhichRayObject=0, hitPointOut, ignore);
    if (mouseOver)
        return mouseOver->getProxyPtr()->getObjectReference();

    return SpaceObjectReference::null();
}

/** Create a UI element using a web view. */
void OgreSystemMouseHandler::createUIAction(const String& ui_page)
{
    WebView* ui_wv =
        WebViewManager::getSingleton().createWebView(
            mParent->context(), "__object", "__object", 300,
            300, OverlayPosition(RP_BOTTOMCENTER),
            mParent->renderStrand());

    ui_wv->loadFile(ui_page);
}

inline Vector3f direction(Quaternion cameraAngle) {
    return -cameraAngle.zAxis();
}


///// Top Level Input Event Handlers //////

EventResponse OgreSystemMouseHandler::onInputDeviceEvent(InputDeviceEventPtr ev) {
    switch (ev->mType) {
      case InputDeviceEvent::ADDED:
        break;
      case InputDeviceEvent::REMOVED:
        break;
    }
    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::onKeyEvent(ButtonEventPtr buttonev) {
    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onButton(buttonev);
    if (browser_resp == EventResponse::cancel())
        return EventResponse::cancel();

    delegateEvent(buttonev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::onAxisEvent(AxisEventPtr axisev) {
    float multiplier = mParent->mInputManager->wheelToAxis();

    if (axisev->mAxis == SDLMouse::WHEELY) {
        bool used = WebViewManager::getSingleton().injectMouseWheel(WebViewCoord(0, axisev->mValue.getCentered()/multiplier));
        if (used)
            return EventResponse::cancel();
    }
    if (axisev->mAxis == SDLMouse::WHEELX) {
        bool used = WebViewManager::getSingleton().injectMouseWheel(WebViewCoord(axisev->mValue.getCentered()/multiplier, 0));
        if (used)
            return EventResponse::cancel();
    }

    delegateEvent(axisev);

    return EventResponse::cancel();
}

EventResponse OgreSystemMouseHandler::onTextInputEvent(TextInputEventPtr textev) {
    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onKeyTextInput(textev);
    if (browser_resp == EventResponse::cancel())
        return EventResponse::cancel();

    delegateEvent(textev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::onMouseHoverEvent(MouseHoverEventPtr mouseev) {
    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onMouseHover(mouseev);
    if (browser_resp == EventResponse::cancel())
        return EventResponse::cancel();

    delegateEvent(mouseev);

    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        mouseOverWebView(camera, time, mouseev->mX, mouseev->mY, false, false);
    }

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::onMousePressedEvent(MousePressedEventPtr mouseev) {
    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onMousePressed(mouseev);
    if (browser_resp == EventResponse::cancel()) {
        mWebViewActiveButtons.insert(mouseev->mButton);
        return EventResponse::cancel();
    }

    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        hoverEntity(camera, time, mouseev->mXStart, mouseev->mYStart, true, &lhc, mWhichRayObject);
        mouseOverWebView(camera, time, mouseev->mXStart, mouseev->mYStart, true, false);
    }

    delegateEvent(mouseev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::onMouseReleasedEvent(MouseReleasedEventPtr mouseev) {
    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        hoverEntity(camera, time, mouseev->mXStart, mouseev->mYStart, true, &lhc, mWhichRayObject);
        mouseOverWebView(camera, time, mouseev->mXStart, mouseev->mYStart, true, false);
    }

    delegateEvent(mouseev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::onMouseClickEvent(MouseClickEventPtr mouseev) {
    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onMouseClick(mouseev);
    if (mWebViewActiveButtons.find(mouseev->mButton) != mWebViewActiveButtons.end()) {
        mWebViewActiveButtons.erase(mouseev->mButton);
        return EventResponse::cancel();
    }
    if (browser_resp == EventResponse::cancel()) {
        return EventResponse::cancel();
    }
    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        mouseOverWebView(camera, time, mouseev->mX, mouseev->mY, false, true);
    }
    mMouseDownObject.reset();

    delegateEvent(mouseev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::onMouseDragEvent(MouseDragEventPtr ev) {
    if (!mParent||!mParent->mPrimaryCamera) return EventResponse::nop();
    std::set<int>::iterator iter = mWebViewActiveButtons.find(ev->mButton);
    if (iter != mWebViewActiveButtons.end()) {
        // Give the browser a chance to use this input
        EventResponse browser_resp = WebViewManager::getSingleton().onMouseDrag(ev);

        if (ev->mType == Input::DRAG_END) {
            mWebViewActiveButtons.erase(iter);
        }

        if (browser_resp == EventResponse::cancel()) {
            return EventResponse::cancel();
        }
    }

    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        mouseOverWebView(camera, time, ev->mX, ev->mY, false, ev->mType == Input::DRAG_END);
    }
    if (ev->mType == Input::DRAG_END) {
        mMouseDownObject.reset();
    }

    delegateEvent(ev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::onWebViewEvent(WebViewEventPtr webview_ev) {
    // For everything else we let the browser go first, but in this case it should have
    // had its chance, so we just let it go
    delegateEvent(webview_ev);

    return EventResponse::nop();
}



void OgreSystemMouseHandler::fpsUpdateTick(const Task::LocalTime& t) {
    if(mUIWidgetView) {
        Task::DeltaTime dt = t - mLastFpsTime;
        if(dt.toSeconds() > 1) {
            mLastFpsTime = t;
            Ogre::RenderTarget::FrameStats stats = mParent->getRenderTarget()->getStatistics();
            ostringstream os;
            os << stats.avgFPS;
            mUIWidgetView->evaluateJS("update_fps(" + os.str() + ")");
        }
    }
}

void OgreSystemMouseHandler::renderStatsUpdateTick(const Task::LocalTime& t) {
    if(mUIWidgetView) {
        Task::DeltaTime dt = t - mLastRenderStatsTime;
        if(dt.toSeconds() > 1) {
            mLastRenderStatsTime = t;
            Ogre::RenderTarget::FrameStats stats = mParent->getRenderTarget()->getStatistics();
            mUIWidgetView->evaluateJS(
                "update_render_stats(" +
                boost::lexical_cast<String>(stats.batchCount) +
                ", " +
                boost::lexical_cast<String>(stats.triangleCount) +
                ")"
            );
        }
    }
}


OgreSystemMouseHandler::OgreSystemMouseHandler(OgreSystem *parent)
 : mUIWidgetView(NULL),
   mParent(parent),
   mWhichRayObject(0),
   mLastCameraTime(Task::LocalTime::now()),
   mLastFpsTime(Task::LocalTime::now()),
   mLastRenderStatsTime(Task::LocalTime::now()),
   mUIReady(false)
{
    mLastHitCount=0;
    mLastHitX=0;
    mLastHitY=0;

    mParent->mInputManager->addListener(this);
}

OgreSystemMouseHandler::~OgreSystemMouseHandler() {

    mParent->mInputManager->removeListener(this);

    if(mUIWidgetView) {
        WebViewManager::getSingleton().destroyWebView(mUIWidgetView);
        mUIWidgetView = NULL;
    }
}

void OgreSystemMouseHandler::addDelegate(Invokable* del) {
    mDelegates[del] = del;
}

void OgreSystemMouseHandler::removeDelegate(Invokable* del)
{
    std::map<Invokable*,Invokable*>::iterator delIter = mDelegates.find(del);
    if (delIter != mDelegates.end())
        mDelegates.erase(delIter);
    else
        SILOG(input,error,"Error in OgreSystemMouseHandler::removeDelegate.  Attempting to remove delegate that does not exist.");
}

void OgreSystemMouseHandler::uiReady() {
    mUIReady = true;
}

Input::Modifier OgreSystemMouseHandler::getCurrentModifiers() const {
    Input::Modifier result = MOD_NONE;

    if (mParent->getInputManager()->isModifierDown(Input::MOD_SHIFT))
        result |= MOD_SHIFT;
    if (mParent->getInputManager()->isModifierDown(Input::MOD_CTRL))
        result |= MOD_CTRL;
    if (mParent->getInputManager()->isModifierDown(Input::MOD_ALT))
        result |= MOD_ALT;
    if (mParent->getInputManager()->isModifierDown(Input::MOD_GUI))
        result |= MOD_GUI;

        return result;
}

namespace {

// Fills in modifier fields
void fillModifiers(Invokable::Dict& event_data, Input::Modifier m) {
    Invokable::Dict mods;
    mods["shift"] = Invokable::asAny((bool)(m & MOD_SHIFT));
    mods["ctrl"] = Invokable::asAny((bool)(m & MOD_CTRL));
    mods["alt"] = Invokable::asAny((bool)(m & MOD_ALT));
    mods["super"] = Invokable::asAny((bool)(m & MOD_GUI));
    event_data["modifier"] = Invokable::asAny(mods);
}

}

void OgreSystemMouseHandler::delegateEvent(InputEventPtr inputev) {
    if (mDelegates.empty())
        return;

    Invokable::Dict event_data;
    {
        ButtonPressedEventPtr button_pressed_ev (std::tr1::dynamic_pointer_cast<ButtonPressed>(inputev));
        if (button_pressed_ev) {

            event_data["msg"] = Invokable::asAny(String("button-pressed"));
            event_data["button"] = Invokable::asAny(keyButtonString(button_pressed_ev->mButton));
            event_data["keycode"] = Invokable::asAny((int32)button_pressed_ev->mButton);
            fillModifiers(event_data, button_pressed_ev->mModifier);
        }
    }

    {
        ButtonRepeatedEventPtr button_pressed_ev (std::tr1::dynamic_pointer_cast<ButtonRepeated>(inputev));
        if (button_pressed_ev) {

            event_data["msg"] = Invokable::asAny(String("button-repeat"));
            event_data["button"] = Invokable::asAny(keyButtonString(button_pressed_ev->mButton));
            event_data["keycode"] = Invokable::asAny((int32)button_pressed_ev->mButton);
            fillModifiers(event_data, button_pressed_ev->mModifier);
        }
    }

    {
        ButtonReleasedEventPtr button_released_ev (std::tr1::dynamic_pointer_cast<ButtonReleased>(inputev));
        if (button_released_ev) {

            event_data["msg"] = Invokable::asAny(String("button-up"));
            event_data["button"] = Invokable::asAny(keyButtonString(button_released_ev->mButton));
            event_data["keycode"] = Invokable::asAny((int32)button_released_ev->mButton);
            fillModifiers(event_data, button_released_ev->mModifier);
        }
    }

    {
        ButtonDownEventPtr button_down_ev (std::tr1::dynamic_pointer_cast<ButtonDown>(inputev));
        if (button_down_ev) {
            event_data["msg"] = Invokable::asAny(String("button-down"));
            event_data["button"] = Invokable::asAny(keyButtonString(button_down_ev->mButton));
            event_data["keycode"] = Invokable::asAny((int32)button_down_ev->mButton);
            fillModifiers(event_data, button_down_ev->mModifier);
        }
    }

    {
        AxisEventPtr axis_ev (std::tr1::dynamic_pointer_cast<AxisEvent>(inputev));
        if (axis_ev) {
            event_data["msg"] = Invokable::asAny(String("axis"));
            event_data["axis"] = Invokable::asAny((int32)axis_ev->mAxis);
            event_data["value"] = Invokable::asAny(axis_ev->mValue.value);
        }
    }

    {
        TextInputEventPtr text_input_ev (std::tr1::dynamic_pointer_cast<TextInputEvent>(inputev));
        if (text_input_ev) {
            event_data["msg"] = Invokable::asAny(String("text"));
            event_data["value"] = Invokable::asAny(text_input_ev->mText);
        }
    }

    {
        MouseHoverEventPtr mouse_hover_ev (std::tr1::dynamic_pointer_cast<MouseHoverEvent>(inputev));
        if (mouse_hover_ev) {
            event_data["msg"] = Invokable::asAny(String("mouse-hover"));
            float32 x, y;
            bool valid = mParent->translateToDisplayViewport(mouse_hover_ev->mX, mouse_hover_ev->mY, &x, &y);
            if (!valid) return;
            event_data["x"] = Invokable::asAny(x);
            event_data["y"] = Invokable::asAny(y);
            fillModifiers(event_data, getCurrentModifiers());
        }
    }

    {
        MousePressedEventPtr mouse_press_ev (std::tr1::dynamic_pointer_cast<MousePressedEvent>(inputev));
        if (mouse_press_ev) {
            event_data["msg"] = Invokable::asAny(String("mouse-press"));
            event_data["button"] = Invokable::asAny((int32)mouse_press_ev->mButton);
            float32 x, y;
            bool valid = mParent->translateToDisplayViewport(mouse_press_ev->mX, mouse_press_ev->mY, &x, &y);
            if (!valid) return;
            event_data["x"] = Invokable::asAny(x);
            event_data["y"] = Invokable::asAny(y);
            fillModifiers(event_data, getCurrentModifiers());
        }
    }

    {
        MouseReleasedEventPtr mouse_release_ev (std::tr1::dynamic_pointer_cast<MouseReleasedEvent>(inputev));
        if (mouse_release_ev) {
            event_data["msg"] = Invokable::asAny(String("mouse-release"));
            event_data["button"] = Invokable::asAny((int32)mouse_release_ev->mButton);
            float32 x, y;
            bool valid = mParent->translateToDisplayViewport(mouse_release_ev->mX, mouse_release_ev->mY, &x, &y);
            if (!valid) return;
            event_data["x"] = Invokable::asAny(x);
            event_data["y"] = Invokable::asAny(y);
            fillModifiers(event_data, getCurrentModifiers());
        }
    }

    {
        MouseClickEventPtr mouse_click_ev (std::tr1::dynamic_pointer_cast<MouseClickEvent>(inputev));
        if (mouse_click_ev) {
            event_data["msg"] = Invokable::asAny(String("mouse-click"));
            event_data["button"] = Invokable::asAny((int32)mouse_click_ev->mButton);
            float32 x, y;
            bool valid = mParent->translateToDisplayViewport(mouse_click_ev->mX, mouse_click_ev->mY, &x, &y);
            if (!valid) return;
            event_data["x"] = Invokable::asAny(x);
            event_data["y"] = Invokable::asAny(y);
            fillModifiers(event_data, getCurrentModifiers());
        }
    }

    {
        MouseDragEventPtr mouse_drag_ev (std::tr1::dynamic_pointer_cast<MouseDragEvent>(inputev));
        if (mouse_drag_ev) {
            event_data["msg"] = Invokable::asAny(String("mouse-drag"));
            event_data["button"] = Invokable::asAny((int32)mouse_drag_ev->mButton);
            float32 x, y;
            bool valid = mParent->translateToDisplayViewport(mouse_drag_ev->mX, mouse_drag_ev->mY, &x, &y);
            if (!valid) return;
            event_data["x"] = Invokable::asAny(x);
            event_data["y"] = Invokable::asAny(y);
            event_data["dx"] = Invokable::asAny(mouse_drag_ev->deltaX());
            event_data["dy"] = Invokable::asAny(mouse_drag_ev->deltaY());
            fillModifiers(event_data, getCurrentModifiers());
        }
    }

    {
        DragAndDropEventPtr dd_ev (std::tr1::dynamic_pointer_cast<DragAndDropEvent>(inputev));
        if (dd_ev) {
            event_data["msg"] = Invokable::asAny(String("dragdrop"));
        }
    }

    {
        WebViewEventPtr wv_ev (std::tr1::dynamic_pointer_cast<WebViewEvent>(inputev));
        if (wv_ev) {
            event_data["msg"] = Invokable::asAny((String("webview")));
            event_data["webview"] = Invokable::asAny((wv_ev->webview));
            event_data["name"] = Invokable::asAny((wv_ev->name));
            Invokable::Array wv_args;
            for(uint32 ii = 0; ii < wv_ev->args.size(); ii++)
                wv_args.push_back(wv_ev->args[ii]);
            event_data["args"] = Invokable::asAny(wv_args);
        }
    }

    if (event_data.empty()) return;

    std::vector<boost::any> args;
    args.push_back(Invokable::asAny(event_data));


    for (std::map<Invokable*, Invokable*>::iterator delIter = mDelegates.begin();
         delIter != mDelegates.end(); ++delIter)
    {
        delIter->first->invoke(args);
    }
}


void OgreSystemMouseHandler::alert(const String& title, const String& text) {
    if (!mUIWidgetView) return;

    mUIWidgetView->evaluateJS("alert_permanent('" + title + "', '" + text + "');");
}

void OgreSystemMouseHandler::onUIDirectoryListingFinished(String initial_path,
    std::tr1::shared_ptr<Transfer::DiskManager::ScanRequest::DirectoryListing> dirListing) {
    std::ostringstream os;
    os << "directory_list_request({path:'" << initial_path << "', results:[";
    if(dirListing) {
        bool needComma = false;
        for(Transfer::DiskManager::ScanRequest::DirectoryListing::iterator it =
                dirListing->begin(); it != dirListing->end(); it++) {
            if(needComma) {
                os << ",";
            } else {
                needComma = true;
            }
            os << "{path:'" << it->mPath.filename() << "', directory:" <<
                (it->mFileStatus.type() == Transfer::Filesystem::boost_fs::directory_file ?
                    "true" : "false") << "}";
        }
    }
    os << "]});";
    printf("Calling to JS: %s\n", os.str().c_str());
    mUIWidgetView->evaluateJS(os.str());
}

boost::any OgreSystemMouseHandler::onUIAction(WebView* webview, const JSArguments& args) {
    printf("ui action event fired arg length = %d\n", (int)args.size());
    if (args.size() < 1) {
        printf("expected at least 1 argument, returning.\n");
        return boost::any();
    }

    String action_triggered(args[0].data());

    printf("UI Action triggered. action = '%s'.\n", action_triggered.c_str());

    if(action_triggered == "action_exit") {
        mParent->quit();
    } else if(action_triggered == "action_directory_list_request") {
        if(args.size() != 2) {
            printf("expected 2 arguments, returning.\n");
            return boost::any();
        }
        String pathRequested(args[1].data());
        std::tr1::shared_ptr<Transfer::DiskManager::DiskRequest> scan_req(
            new Transfer::DiskManager::ScanRequest(pathRequested,
                std::tr1::bind(&OgreSystemMouseHandler::onUIDirectoryListingFinished, this, pathRequested, _1)));
        Transfer::DiskManager::getSingleton().addRequest(scan_req);
    }

    return boost::any();
}

void OgreSystemMouseHandler::ensureUI() {
    if(!mUIWidgetView) {
        SILOG(ogre, info, "Creating UI Widget");
        mUIWidgetView = WebViewManager::getSingleton().createWebView(
            mParent->context(),
            "ui_widget","ui_widget",
            mParent->getRenderTarget()->getWidth(), mParent->getRenderTarget()->getHeight(),
            OverlayPosition(RP_TOPLEFT),
            mParent->renderStrand(),
            false,70, TIER_BACK, 0,
            WebView::WebViewBorderSize(0,0,0,0));
        mUIWidgetView->bind("ui-action", std::tr1::bind(&OgreSystemMouseHandler::onUIAction, this, _1, _2));
        mUIWidgetView->loadFile("chrome/ui.html");
        mUIWidgetView->setTransparent(true);
    }
}

void OgreSystemMouseHandler::windowResized(uint32 w, uint32 h) {
    // Make sure our widget overlay gets scaled appropriately.
    if (mUIWidgetView) {
        mUIWidgetView->resize(w, h);
    }
}

void OgreSystemMouseHandler::tick(const Task::LocalTime& t) {
    if (mUIReady) {
        fpsUpdateTick(t);
        renderStatsUpdateTick(t);
    }

    ensureUI();
}

}
}
