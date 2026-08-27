//
//  SuperTuxKart - a fun racing game with go-kart
//  mklib integration: per-device keyboard input bridge
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef HEADER_MKLIB_INPUT_BRIDGE_HPP
#define HEADER_MKLIB_INPUT_BRIDGE_HPP

#ifdef ENABLE_MKLIB

#include "input/input.hpp"

#include <map>
#include <string>

#include <mklib/mklib.h>

#ifdef _WIN32
#include <SDL_syswm.h>
#endif

class KeyboardDevice;

/**
  * \brief Bridges mklib per-device keyboard events into SuperTuxKart.
  *
  * SuperTuxKart already models split-screen multiplayer as "each player owns
  * an input device" (see DeviceManager::mapKeyboardInput and the ASSIGN /
  * DETECT_NEW modes). What is missing is the ability to tell physical
  * keyboards apart: Irrlicht/SDL deliver keyboard events without any device
  * origin, so two keyboards look like one.
  *
  * mklib fills exactly that gap: it reports which physical device produced
  * each key event. This bridge therefore:
  *   - creates one KeyboardDevice per physical keyboard (with a real name),
  *   - feeds key events through InputManager::dispatchInput with the mklib
  *     device id, so mapKeyboardInput can route to the right player,
  *   - in menus (NO_ASSIGN mode, before players are assigned) funnels every
  *     keyboard to the menu keyboard, so any keyboard can drive the UI,
  *   - synthesises Irrlicht key events carrying the typed character, so text
  *     entry (player name, server address, search fields) keeps working even
  *     though mklib owns the keyboard.
  *
  * Mouse and gamepad input are deliberately left to SDL as before.
  *
  * \ingroup input
  */
class MklibInputBridge
{
private:
    static MklibInputBridge *m_instance;

    mklib_handle *m_handle;

    bool m_running;

    /** Set once start() has been attempted, so a failure (e.g. missing input
      * monitoring permission) is not retried on every single frame. */
    bool m_start_attempted;

    /** Seconds since the last device list refresh. */
    float m_refresh_timer;

#ifdef _WIN32
    /** True once the game window has been attached for Raw Input. */
    bool m_attached;
#endif

    /** Physical mklib device id -> the STK keyboard device representing it. */
    std::map<mklib_device_id, KeyboardDevice *> m_keyboards;

    /** Per-device shift state (needed to type shifted characters). */
    std::map<mklib_device_id, bool> m_shift_down;

    /** Set while a synthesised Irrlicht event is being delivered, so
      * InputManager can skip the action mapping for it (the action was
      * already dispatched by this bridge). */
    bool m_synthetic_event;

    void refreshDevices();

    int routeDeviceId(mklib_device_id physical_id);

    void handleKeyEvent(const mklib_event &event);

    void postSyntheticCharEvent(int irr_key, wchar_t character, bool shift);

                 MklibInputBridge();
                ~MklibInputBridge();

public:
    static MklibInputBridge *get();
    static void destroy();

    /** Starts mklib. Returns false if mklib is unavailable, in which case
      * STK keeps using the regular Irrlicht keyboard events. */
    bool start();

    void stop();

    /** Polls pending mklib events; call once per frame. Starts mklib lazily
      * on the first call, because it needs the global input_manager pointer
      * which is only assigned after InputManager has been constructed. */
    void update(float dt);

    /** True once mklib is delivering keyboard events. */
    bool isRunning() const { return m_running; }

    /** True while this bridge is feeding a synthesised event to Irrlicht. */
    bool isSyntheticEvent() const { return m_synthetic_event; }

    void clearSyntheticFlag()     { m_synthetic_event = false; }

    /** Number of physical keyboards currently known to mklib. */
    int  getKeyboardCount() const { return (int)m_keyboards.size(); }

#ifdef _WIN32
    /** Attaches a window handle so mklib can receive Raw Input. */
    bool attachWindow(void *hwnd);

    /** Convenience wrapper: resolves the HWND of an SDL window and attaches
      * it. Returns true once mklib is attached to a window. */
    bool attachToSDLWindow(SDL_Window *window);

    /** Forwards a Windows message to mklib (WM_INPUT and
      * WM_INPUT_DEVICE_CHANGE). Returns true if mklib handled it. */
    bool processWindowsMessage(unsigned int message, uintptr_t wparam,
                               intptr_t lparam);

    /** True once the Windows window has been attached. */
    bool isAttached() const { return m_attached; }
#endif
};   // class MklibInputBridge

#endif // ENABLE_MKLIB
#endif // HEADER_MKLIB_INPUT_BRIDGE_HPP
