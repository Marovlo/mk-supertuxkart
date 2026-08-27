//
//  SuperTuxKart - a fun racing game with go-kart
//  mklib integration: HID usage -> Irrlicht key mapping
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

#ifndef HEADER_MKLIB_KEY_MAP_HPP
#define HEADER_MKLIB_KEY_MAP_HPP

#include <cstdint>

#ifdef ENABLE_MKLIB

/**
  * \brief Maps HID keyboard usage codes to Irrlicht key codes and characters.
  *
  * mklib reports raw HID usages (usage page 0x07 for keyboard/keypad).
  * SuperTuxKart works with irr::EKEY_CODE values, and text entry needs the
  * actual character. This translation layer is purely mechanical: it assumes
  * a US QWERTY layout, which is what the original OS-level keyboard events
  * provided on the platforms mklib targets.
  *
  * \ingroup input
  */
namespace MklibKeyMap
{
    /** HID usage page for the keyboard/keypad. */
    static const uint16_t KEYBOARD_USAGE_PAGE = 0x07;

    /** HID modifier usages (usage page 0x07). */
    static const uint16_t USAGE_LEFT_CONTROL   = 0xe0;
    static const uint16_t USAGE_LEFT_SHIFT     = 0xe1;
    static const uint16_t USAGE_LEFT_ALT       = 0xe2;
    static const uint16_t USAGE_LEFT_GUI       = 0xe3;
    static const uint16_t USAGE_RIGHT_CONTROL  = 0xe4;
    static const uint16_t USAGE_RIGHT_SHIFT    = 0xe5;
    static const uint16_t USAGE_RIGHT_ALT      = 0xe6;
    static const uint16_t USAGE_RIGHT_GUI      = 0xe7;

    /** Returns the irr::EKEY_CODE for a HID keyboard usage, or 0 if unknown. */
    int usageToIrrKey(uint16_t usage);

    /** Returns true if this usage is a modifier key. */
    bool isModifier(uint16_t usage);

    /** Returns true if this usage is a shift key. */
    bool isShift(uint16_t usage);

    /** Returns the character produced by a usage, honouring shift state.
      * Returns 0 for keys that do not produce a printable character. */
    wchar_t usageToChar(uint16_t usage, bool shift);

}   // namespace MklibKeyMap

#endif // ENABLE_MKLIB
#endif // HEADER_MKLIB_KEY_MAP_HPP
