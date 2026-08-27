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

#include "input/mklib/mklib_input_bridge.hpp"

#ifdef ENABLE_MKLIB

#include "graphics/irr_driver.hpp"
#include "input/device_manager.hpp"
#include "input/input_manager.hpp"
#include "input/keyboard_device.hpp"
#include "input/mklib/mklib_key_map.hpp"
#include "utils/log.hpp"

#include <IEventReceiver.h>
#include <IrrlichtDevice.h>
#include <Keycodes.h>

MklibInputBridge *MklibInputBridge::m_instance = NULL;

// ============================================================================
MklibInputBridge *MklibInputBridge::get()
{
    if (m_instance == NULL)
        m_instance = new MklibInputBridge();
    return m_instance;
}   // get

// ----------------------------------------------------------------------------
void MklibInputBridge::destroy()
{
    delete m_instance;
    m_instance = NULL;
}   // destroy

// ----------------------------------------------------------------------------
MklibInputBridge::MklibInputBridge()
{
    m_handle          = NULL;
    m_running         = false;
    m_start_attempted = false;
    m_refresh_timer   = 0.0f;
    m_synthetic_event = false;
#ifdef _WIN32
    m_attached        = false;
#endif
}   // MklibInputBridge

// ----------------------------------------------------------------------------
MklibInputBridge::~MklibInputBridge()
{
    stop();
}   // ~MklibInputBridge

// ----------------------------------------------------------------------------
bool MklibInputBridge::start()
{
    if (m_running)
        return true;
    if (m_start_attempted)
        return false;
    m_start_attempted = true;

    mklib_config config;
    if (mklib_config_init(&config) != MKLIB_OK)
    {
        Log::warn("mklib", "mklib_config_init failed.");
        return false;
    }

    config.event_queue_capacity = 4096;
    config.request_input_access = true;
    config.device_kind_mask     = MKLIB_DEVICE_MASK_KEYBOARD;

    mklib_status status = mklib_create(&config, &m_handle);
    if (status != MKLIB_OK || m_handle == NULL)
    {
        Log::warn("mklib", "mklib_create failed: %s",
                  mklib_status_string(status));
        m_handle = NULL;
        return false;
    }

    status = mklib_start(m_handle);
    if (status != MKLIB_OK)
    {
        Log::warn("mklib", "mklib_start failed: %s",
                  mklib_status_string(status));
        mklib_destroy(&m_handle);
        m_handle = NULL;
        return false;
    }

    m_running = true;
    refreshDevices();
    Log::info("mklib", "Keyboard input started on %s (access=%s, %d keyboard(s)).",
              mklib_platform_name(),
              mklib_access_status_string(mklib_input_access_status()),
              getKeyboardCount());
    return true;
}   // start

// ----------------------------------------------------------------------------
void MklibInputBridge::stop()
{
    if (!m_running)
        return;

    m_running = false;
    mklib_stop(m_handle);
    mklib_destroy(&m_handle);
    m_handle = NULL;
    m_keyboards.clear();
    m_shift_down.clear();
}   // stop

// ----------------------------------------------------------------------------
#ifdef _WIN32
bool MklibInputBridge::attachWindow(void *hwnd)
{
    if (!m_running || hwnd == NULL)
        return false;

    mklib_status status = mklib_windows_attach_window(
        m_handle, reinterpret_cast<mklib_windows_window_handle>(hwnd),
        MKLIB_WINDOWS_ATTACH_REGISTER_RAW_INPUT);
    if (status != MKLIB_OK)
    {
        Log::warn("mklib", "mklib_windows_attach_window failed: %s",
                  mklib_status_string(status));
        return false;
    }
    m_attached = true;
    Log::info("mklib", "Attached game window for Raw Input.");
    return true;
}   // attachWindow

// ----------------------------------------------------------------------------
bool MklibInputBridge::attachToSDLWindow(SDL_Window *window)
{
    if (m_attached || window == NULL)
        return m_attached;

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info))
    {
        Log::warn("mklib", "SDL_GetWindowWMInfo failed: %s", SDL_GetError());
        return false;
    }
    if (info.subsystem != SDL_SYSWM_WINDOWS)
        return false;

    return attachWindow((void *)info.info.win.window);
}   // attachToSDLWindow

// ----------------------------------------------------------------------------
bool MklibInputBridge::processWindowsMessage(unsigned int message,
                                             uintptr_t wparam, intptr_t lparam)
{
    if (!m_running || !m_attached)
        return false;

    bool handled = false;
    mklib_status status = mklib_windows_process_message(m_handle, message,
                                                        wparam, lparam,
                                                        &handled);
    if (status != MKLIB_OK)
        return false;
    return handled;
}   // processWindowsMessage
#endif

// ----------------------------------------------------------------------------
void MklibInputBridge::refreshDevices()
{
    if (!m_running)
        return;

    DeviceManager *device_manager = input_manager->getDeviceManager();
    if (device_manager == NULL)
        return;

    size_t count = 0;
    if (mklib_get_devices(m_handle, NULL, 0, &count) != MKLIB_OK || count == 0)
        return;

    std::vector<mklib_device_info> devices(count);
    if (mklib_get_devices(m_handle, devices.data(), count, &count) != MKLIB_OK)
        return;

    std::vector<mklib_device_info> keyboards;
    for (size_t i = 0; i < count; i++)
    {
        if (devices[i].kind == MKLIB_DEVICE_KEYBOARD)
            keyboards.push_back(devices[i]);
    }

    // Drop registrations for keyboards that are gone, so their STK device can
    // be reused by a keyboard that is plugged in later.
    for (std::map<mklib_device_id, KeyboardDevice *>::iterator it =
             m_keyboards.begin(); it != m_keyboards.end();)
    {
        bool still_present = false;
        for (size_t i = 0; i < keyboards.size(); i++)
        {
            if (keyboards[i].id == it->first)
            {
                still_present = true;
                break;
            }
        }
        if (!still_present)
        {
            it->second->setMklibId(-1);
            it = m_keyboards.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (size_t i = 0; i < keyboards.size(); i++)
    {
        const mklib_device_info &info = keyboards[i];
        std::string name = info.product[0] != '\0' ? info.product : "Keyboard";

        // Already known - just keep the name up to date.
        std::map<mklib_device_id, KeyboardDevice *>::iterator it =
            m_keyboards.find(info.id);
        if (it != m_keyboards.end())
        {
            it->second->setName(name);
            continue;
        }

        // Prefer to reuse a keyboard device that does not stand for a physical
        // keyboard yet (for example the default one loaded from input.xml).
        // Creating a new configuration on every start would make the stored
        // configuration grow without bound.
        KeyboardDevice *keyboard = NULL;
        const int amount = device_manager->getKeyboardAmount();
        for (int n = 0; n < amount; n++)
        {
            KeyboardDevice *candidate = device_manager->getKeyboard(n);
            if (candidate != NULL && candidate->getMklibId() < 0)
            {
                keyboard = candidate;
                break;
            }
        }

        if (keyboard == NULL)
        {
            device_manager->addEmptyKeyboard();
            keyboard =
                device_manager->getKeyboard(device_manager->getKeyboardAmount() - 1);
        }

        if (keyboard == NULL)
            continue;

        keyboard->setMklibId((int)info.id);
        keyboard->setName(name);
        m_keyboards[info.id] = keyboard;
        Log::info("mklib", "Registered keyboard id=%u name=%s persistent=%s",
                  info.id, name.c_str(), info.persistent_id);
    }
}   // refreshDevices

// ----------------------------------------------------------------------------
int MklibInputBridge::routeDeviceId(mklib_device_id physical_id)
{
    DeviceManager *device_manager = input_manager->getDeviceManager();
    if (device_manager == NULL)
        return 0;

    // In menus (before players have claimed devices) any keyboard must be
    // able to drive the UI. Report "no specific device" (-1) so the manager
    // falls back to its historical behaviour; routing everything to one
    // keyboard's id instead would make every keyboard impersonate that one.
    if (device_manager->getAssignMode() == NO_ASSIGN)
        return -1;

    // During kart selection / racing each keyboard keeps its own identity.
    return (int)physical_id;
}   // routeDeviceId

// ----------------------------------------------------------------------------
void MklibInputBridge::handleKeyEvent(const mklib_event &event)
{
    if (event.usage_page != MklibKeyMap::KEYBOARD_USAGE_PAGE)
        return;

    const int irr_key = MklibKeyMap::usageToIrrKey(event.usage);
    if (irr_key == 0)
        return;

    const bool down = (event.type == MKLIB_KEY_DOWN);

    if (MklibKeyMap::isShift(event.usage))
        m_shift_down[event.device_id] = down;

    bool shift = false;
    std::map<mklib_device_id, bool>::const_iterator shift_it =
        m_shift_down.find(event.device_id);
    if (shift_it != m_shift_down.end())
        shift = shift_it->second;

    const int device_id = routeDeviceId(event.device_id);

    // Diagnostics: which physical keyboard produced what, and where it was
    // routed. Only logged on key press to keep the log readable.
    if (down)
    {
        Log::info("mklib", "key phys=%u usage=0x%02x irr=0x%02x shift=%d -> device=%d",
                  event.device_id, event.usage, irr_key, (int)shift, device_id);
    }

    input_manager->dispatchInput(Input::IT_KEYBOARD, device_id, irr_key,
                                 Input::AD_POSITIVE,
                                 down ? Input::MAX_VALUE : 0, shift);

    // Text entry: Irrlicht text boxes need the typed character, which mklib
    // does not report. Derive it from the HID usage and deliver a synthesised
    // event that only the GUI consumes (InputManager skips its mapping).
    if (down)
    {
        const wchar_t character = MklibKeyMap::usageToChar(event.usage, shift);
        if (character != 0)
            postSyntheticCharEvent(irr_key, character, shift);
    }
}   // handleKeyEvent

// ----------------------------------------------------------------------------
void MklibInputBridge::postSyntheticCharEvent(int irr_key, wchar_t character,
                                              bool shift)
{
    if (irr_driver == NULL || irr_driver->getDevice() == NULL)
        return;

    irr::SEvent event;
    event.EventType            = irr::EET_KEY_INPUT_EVENT;
    event.KeyInput.Key         = (irr::EKEY_CODE)irr_key;
    event.KeyInput.Char        = character;
    event.KeyInput.PressedDown = true;
    event.KeyInput.Shift       = shift;
    event.KeyInput.Control     = false;

    m_synthetic_event = true;
    irr_driver->getDevice()->postEventFromUser(event);
    m_synthetic_event = false;
}   // postSyntheticCharEvent

// ----------------------------------------------------------------------------
void MklibInputBridge::update(float dt)
{
    if (!m_running)
        return;

    // Bounded drain: input can outpace the frame (e.g. key repeat), and an
    // unbounded loop would stall the main thread.
    const int max_events_per_frame = 256;
    mklib_event event;
    for (int i = 0; i < max_events_per_frame; i++)
    {
        mklib_status status = mklib_poll_event(m_handle, &event, 0);
        if (status != MKLIB_OK)
            break;
        if (event.type == MKLIB_KEY_DOWN || event.type == MKLIB_KEY_UP)
            handleKeyEvent(event);
        // Mouse and gamepad input stay with SDL (deliberate decision).
    }

    // Keep in sync with keyboards plugged in or removed mid-session. Device
    // enumeration is not free, so it is not done every frame.
    m_refresh_timer += dt;
    if (m_refresh_timer >= 1.0f)
    {
        m_refresh_timer = 0.0f;
#ifdef _WIN32
        // Raw Input needs the window handle. Retry here so it also succeeds
        // when the window did not have focus during start-up.
        if (!m_attached)
            attachToSDLWindow(SDL_GetKeyboardFocus());
#endif
        refreshDevices();
    }
}   // update

#endif // ENABLE_MKLIB
