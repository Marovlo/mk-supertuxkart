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

#include "input/mklib/mklib_key_map.hpp"

#ifdef ENABLE_MKLIB

#include <Keycodes.h>

using namespace irr;

namespace MklibKeyMap
{
// ----------------------------------------------------------------------------
int usageToIrrKey(uint16_t usage)
{
    // Letters A-Z
    if (usage >= 0x04 && usage <= 0x1d)
        return IRR_KEY_A + (usage - 0x04);

    // Digits 1-9, 0
    if (usage >= 0x1e && usage <= 0x26)
        return IRR_KEY_1 + (usage - 0x1e);
    if (usage == 0x27)
        return IRR_KEY_0;

    switch (usage)
    {
    case 0x28: return IRR_KEY_RETURN;
    case 0x29: return IRR_KEY_ESCAPE;
    case 0x2a: return IRR_KEY_BACK;
    case 0x2b: return IRR_KEY_TAB;
    case 0x2c: return IRR_KEY_SPACE;
    case 0x2d: return IRR_KEY_MINUS;
    case 0x2e: return IRR_KEY_PLUS;
    case 0x2f: return IRR_KEY_OEM_4;   // [
    case 0x30: return IRR_KEY_OEM_6;   // ]
    case 0x31: return IRR_KEY_OEM_5;   // backslash
    case 0x33: return IRR_KEY_OEM_1;   // ;
    case 0x34: return IRR_KEY_OEM_7;   // '
    case 0x35: return IRR_KEY_OEM_3;   // `
    case 0x36: return IRR_KEY_COMMA;
    case 0x37: return IRR_KEY_PERIOD;
    case 0x38: return IRR_KEY_OEM_2;   // /
    case 0x39: return IRR_KEY_CAPITAL;

    // Function keys
    case 0x3a: return IRR_KEY_F1;
    case 0x3b: return IRR_KEY_F2;
    case 0x3c: return IRR_KEY_F3;
    case 0x3d: return IRR_KEY_F4;
    case 0x3e: return IRR_KEY_F5;
    case 0x3f: return IRR_KEY_F6;
    case 0x40: return IRR_KEY_F7;
    case 0x41: return IRR_KEY_F8;
    case 0x42: return IRR_KEY_F9;
    case 0x43: return IRR_KEY_F10;
    case 0x44: return IRR_KEY_F11;
    case 0x45: return IRR_KEY_F12;

    case 0x46: return IRR_KEY_PRINT;
    case 0x47: return IRR_KEY_SCROLL;
    case 0x48: return IRR_KEY_PAUSE;
    case 0x49: return IRR_KEY_INSERT;
    case 0x4a: return IRR_KEY_HOME;
    case 0x4b: return IRR_KEY_PRIOR;   // page up
    case 0x4c: return IRR_KEY_DELETE;
    case 0x4d: return IRR_KEY_END;
    case 0x4e: return IRR_KEY_NEXT;    // page down
    case 0x4f: return IRR_KEY_RIGHT;
    case 0x50: return IRR_KEY_LEFT;
    case 0x51: return IRR_KEY_DOWN;
    case 0x52: return IRR_KEY_UP;

    case 0x53: return IRR_KEY_NUMLOCK;
    case 0x54: return IRR_KEY_DIVIDE;
    case 0x55: return IRR_KEY_MULTIPLY;
    case 0x56: return IRR_KEY_SUBTRACT;
    case 0x57: return IRR_KEY_ADD;
    case 0x58: return IRR_KEY_RETURN;  // keypad enter
    case 0x59: return IRR_KEY_NUMPAD1;
    case 0x5a: return IRR_KEY_NUMPAD2;
    case 0x5b: return IRR_KEY_NUMPAD3;
    case 0x5c: return IRR_KEY_NUMPAD4;
    case 0x5d: return IRR_KEY_NUMPAD5;
    case 0x5e: return IRR_KEY_NUMPAD6;
    case 0x5f: return IRR_KEY_NUMPAD7;
    case 0x60: return IRR_KEY_NUMPAD8;
    case 0x61: return IRR_KEY_NUMPAD9;
    case 0x62: return IRR_KEY_NUMPAD0;
    case 0x63: return IRR_KEY_DECIMAL;

    // Modifiers
    case USAGE_LEFT_CONTROL:  return IRR_KEY_CONTROL;
    case USAGE_LEFT_SHIFT:    return IRR_KEY_SHIFT;
    case USAGE_LEFT_ALT:      return IRR_KEY_MENU;
    case USAGE_LEFT_GUI:      return IRR_KEY_LWIN;
    case USAGE_RIGHT_CONTROL: return IRR_KEY_CONTROL;
    case USAGE_RIGHT_SHIFT:   return IRR_KEY_SHIFT;
    case USAGE_RIGHT_ALT:     return IRR_KEY_MENU;
    case USAGE_RIGHT_GUI:     return IRR_KEY_RWIN;

    default: return 0;
    }
}   // usageToIrrKey

// ----------------------------------------------------------------------------
bool isModifier(uint16_t usage)
{
    return usage >= USAGE_LEFT_CONTROL && usage <= USAGE_RIGHT_GUI;
}   // isModifier

// ----------------------------------------------------------------------------
bool isShift(uint16_t usage)
{
    return usage == USAGE_LEFT_SHIFT || usage == USAGE_RIGHT_SHIFT;
}   // isShift

// ----------------------------------------------------------------------------
wchar_t usageToChar(uint16_t usage, bool shift)
{
    // Letters
    if (usage >= 0x04 && usage <= 0x1d)
    {
        const wchar_t base = shift ? L'A' : L'a';
        return base + (usage - 0x04);
    }

    // Digits and their shifted symbols
    if (usage >= 0x1e && usage <= 0x26)
    {
        static const wchar_t shifted[] =
            { L'!', L'@', L'#', L'$', L'%', L'^', L'&', L'*', L'(' };
        if (shift)
            return shifted[usage - 0x1e];
        return L'1' + (usage - 0x1e);
    }

    switch (usage)
    {
    case 0x27: return shift ? L')' : L'0';
    case 0x28: return L'\r';
    case 0x2a: return L'\b';
    case 0x2b: return L'\t';
    case 0x2c: return L' ';
    case 0x2d: return shift ? L'_' : L'-';
    case 0x2e: return shift ? L'+' : L'=';
    case 0x2f: return shift ? L'{' : L'[';
    case 0x30: return shift ? L'}' : L']';
    case 0x31: return shift ? L'|' : L'\\';
    case 0x33: return shift ? L':' : L';';
    case 0x34: return shift ? L'"' : L'\'';
    case 0x35: return shift ? L'~' : L'`';
    case 0x36: return shift ? L'<' : L',';
    case 0x37: return shift ? L'>' : L'.';
    case 0x38: return shift ? L'?' : L'/';
    default:  return 0;
    }
}   // usageToChar

}   // namespace MklibKeyMap

#endif // ENABLE_MKLIB
