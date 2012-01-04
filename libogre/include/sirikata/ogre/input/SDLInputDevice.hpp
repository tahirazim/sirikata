/*  Sirikata Input Plugin -- plugins/input
 *  SDLInputDevice.hpp
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

#ifndef SIRIKATA_INPUT_SDLInputDevice_HPP__
#define SIRIKATA_INPUT_SDLInputDevice_HPP__

#include <sirikata/ogre/input/InputDevice.hpp>

struct SDL_MouseMotionEvent;
struct _SDL_Joystick;

namespace Sirikata {
namespace Input {

class SDLInputManager;

class SDLMouse;
typedef std::tr1::shared_ptr<SDLMouse> SDLMousePtr;
class SDLJoystick;
typedef std::tr1::shared_ptr<SDLJoystick> SDLJoystickPtr;
class SDLKeyboard;
typedef std::tr1::shared_ptr<SDLKeyboard> SDLKeyboardPtr;

class SIRIKATA_OGRE_EXPORT SDLMouse: public PointerDevice {
    unsigned int mWhich;
    unsigned int mNumButtons;
public:
    SDLMouse(SDLInputManager *inputManager, unsigned int which);
    virtual ~SDLMouse();

    SDLInputManager* inputManager();

    enum Axes {WHEELX=NUM_POINTER_AXES, WHEELY,
          PRESSURE, CURSORZ, ROTATION, TILT, NUM_AXES};
    virtual int getNumButtons() const;
    virtual std::string getButtonName(unsigned int button) const;
    virtual unsigned int getNumAxes() const {
        // Constant... each axis has a special purpose.
        // Don't include axes labeled "for future use".
        return PRESSURE + 1;
    }
    virtual void setRelativeMode(bool enabled);

    virtual std::string getAxisName(unsigned int axis) const {
        switch (axis) {
          case AXIS_CURSORX:
            return "x (Cursor)";
          case AXIS_CURSORY:
            return "y (Cursor)";
          case AXIS_RELX:
            return "x (Relative)";
          case AXIS_RELY:
            return "y (Relative)";
          case WHEELX:
            return "screll-x";
          case WHEELY:
            return "scroll-y";
          case CURSORZ:
            return "z";
          case PRESSURE:
            return "pressure";
          case ROTATION:
            return "rotation";
          case TILT:
            return "tilt";
        }
        return std::string();
    }

    void fireMotion(const SDLMousePtr &thisptr,
                    const ::SDL_MouseMotionEvent &ev);
    void fireWheel(const SDLMousePtr &thisptr,
                   int xrel,
                   int yrel);
};

class SDLDigitalHatswitch;

class SIRIKATA_OGRE_EXPORT SDLJoystick : public InputDevice {
private:
    std::string mName;
    std::vector<std::string> mButtonNames;
    ::_SDL_Joystick *mJoy;
    unsigned int mWhich;
    unsigned int mNumGeneralAxes;
    unsigned int mNumBalls;
    unsigned int mNumButtons;
    unsigned int mNumHats;
    enum Directions {HAT_UP, HAT_RIGHT, HAT_DOWN, HAT_LEFT, HAT_MAX};
public:

    /// FIXME!!! throws std::runtime_exception if SDL_JoystickOpen fails.
    SDLJoystick(unsigned int which);
    virtual ~SDLJoystick();

    SDLInputManager* inputManager();

    virtual std::string getButtonName(unsigned int button) const;
    virtual std::string getAxisName(unsigned int button) const;
    void setButtonName(unsigned int button, std::string name);
    virtual unsigned int getNumAxes() const;
    virtual int getNumButtons() const;

    void fireHat(const SDLJoystickPtr &thisptr,
                 unsigned int hatNumber,
                 int hatValue);
    void fireBall(const SDLJoystickPtr &thisptr,
                  unsigned int ballNumber,
                  int xrel,
                  int yrel);
};

class SIRIKATA_OGRE_EXPORT SDLKeyboard : public InputDevice {
    unsigned int mWhich;
public:
    SDLKeyboard(unsigned int which);
    virtual ~SDLKeyboard();

    SDLInputManager* inputManager();

    enum Directions {UP, RIGHT, DOWN, LEFT, MAX_DIRECTION};
	virtual std::string getButtonName(unsigned int button) const;
    // Full keyboard--pointless to list all the possible buttons.
    virtual int getNumButtons() const { return -1; }
    virtual bool isKeyboard() { return true; }
    virtual unsigned int getNumAxes() const { return 0; }
    virtual std::string getAxisName(unsigned int) const {
        return std::string();
    }
};


SIRIKATA_OGRE_EXPORT bool keyIsModifier(Input::KeyButton b);
SIRIKATA_OGRE_EXPORT String keyButtonString(Input::KeyButton b);
SIRIKATA_OGRE_EXPORT String keyModifiersString(Input::Modifier m);
SIRIKATA_OGRE_EXPORT String mouseButtonString(Input::MouseButton b);
SIRIKATA_OGRE_EXPORT String axisString(Input::AxisIndex i);
SIRIKATA_OGRE_EXPORT Input::KeyButton keyButtonFromStrings(std::vector<String>& parts);
SIRIKATA_OGRE_EXPORT Input::Modifier keyModifiersFromStrings(std::vector<String>& parts);
SIRIKATA_OGRE_EXPORT Input::MouseButton mouseButtonFromStrings(std::vector<String>& parts);
SIRIKATA_OGRE_EXPORT Input::AxisIndex axisFromStrings(std::vector<String>& parts);

}
}

#endif
