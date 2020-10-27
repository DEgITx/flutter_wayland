// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2016 Camilla Berglund <elmindreda@glfw.org>
// Copyright (c) 2019 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//.
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//.
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//.
// 3. This notice may not be removed or altered from any source
//    distribution.
//

#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon.h>
#include <cstring>

#include "keys.h"

namespace flutter {

// Borrowed from glfw library {

static short int publicKeys[256];

static void createKeyTables(void) {
  memset(publicKeys, -1, sizeof(publicKeys));

  publicKeys[KEY_GRAVE] = GLFW_KEY_GRAVE_ACCENT;
  publicKeys[KEY_1] = GLFW_KEY_1;
  publicKeys[KEY_2] = GLFW_KEY_2;
  publicKeys[KEY_3] = GLFW_KEY_3;
  publicKeys[KEY_4] = GLFW_KEY_4;
  publicKeys[KEY_5] = GLFW_KEY_5;
  publicKeys[KEY_6] = GLFW_KEY_6;
  publicKeys[KEY_7] = GLFW_KEY_7;
  publicKeys[KEY_8] = GLFW_KEY_8;
  publicKeys[KEY_9] = GLFW_KEY_9;
  publicKeys[KEY_0] = GLFW_KEY_0;
  publicKeys[KEY_MINUS] = GLFW_KEY_MINUS;
  publicKeys[KEY_EQUAL] = GLFW_KEY_EQUAL;
  publicKeys[KEY_Q] = GLFW_KEY_Q;
  publicKeys[KEY_W] = GLFW_KEY_W;
  publicKeys[KEY_E] = GLFW_KEY_E;
  publicKeys[KEY_R] = GLFW_KEY_R;
  publicKeys[KEY_T] = GLFW_KEY_T;
  publicKeys[KEY_Y] = GLFW_KEY_Y;
  publicKeys[KEY_U] = GLFW_KEY_U;
  publicKeys[KEY_I] = GLFW_KEY_I;
  publicKeys[KEY_O] = GLFW_KEY_O;
  publicKeys[KEY_P] = GLFW_KEY_P;
  publicKeys[KEY_LEFTBRACE] = GLFW_KEY_LEFT_BRACKET;
  publicKeys[KEY_RIGHTBRACE] = GLFW_KEY_RIGHT_BRACKET;
  publicKeys[KEY_A] = GLFW_KEY_A;
  publicKeys[KEY_S] = GLFW_KEY_S;
  publicKeys[KEY_D] = GLFW_KEY_D;
  publicKeys[KEY_F] = GLFW_KEY_F;
  publicKeys[KEY_G] = GLFW_KEY_G;
  publicKeys[KEY_H] = GLFW_KEY_H;
  publicKeys[KEY_J] = GLFW_KEY_J;
  publicKeys[KEY_K] = GLFW_KEY_K;
  publicKeys[KEY_L] = GLFW_KEY_L;
  publicKeys[KEY_SEMICOLON] = GLFW_KEY_SEMICOLON;
  publicKeys[KEY_APOSTROPHE] = GLFW_KEY_APOSTROPHE;
  publicKeys[KEY_Z] = GLFW_KEY_Z;
  publicKeys[KEY_X] = GLFW_KEY_X;
  publicKeys[KEY_C] = GLFW_KEY_C;
  publicKeys[KEY_V] = GLFW_KEY_V;
  publicKeys[KEY_B] = GLFW_KEY_B;
  publicKeys[KEY_N] = GLFW_KEY_N;
  publicKeys[KEY_M] = GLFW_KEY_M;
  publicKeys[KEY_COMMA] = GLFW_KEY_COMMA;
  publicKeys[KEY_DOT] = GLFW_KEY_PERIOD;
  publicKeys[KEY_SLASH] = GLFW_KEY_SLASH;
  publicKeys[KEY_BACKSLASH] = GLFW_KEY_BACKSLASH;
  publicKeys[KEY_ESC] = GLFW_KEY_ESCAPE;
  publicKeys[KEY_TAB] = GLFW_KEY_TAB;
  publicKeys[KEY_LEFTSHIFT] = GLFW_KEY_LEFT_SHIFT;
  publicKeys[KEY_RIGHTSHIFT] = GLFW_KEY_RIGHT_SHIFT;
  publicKeys[KEY_LEFTCTRL] = GLFW_KEY_LEFT_CONTROL;
  publicKeys[KEY_RIGHTCTRL] = GLFW_KEY_RIGHT_CONTROL;
  publicKeys[KEY_LEFTALT] = GLFW_KEY_LEFT_ALT;
  publicKeys[KEY_RIGHTALT] = GLFW_KEY_RIGHT_ALT;
  publicKeys[KEY_LEFTMETA] = GLFW_KEY_LEFT_SUPER;
  publicKeys[KEY_RIGHTMETA] = GLFW_KEY_RIGHT_SUPER;
  publicKeys[KEY_MENU] = GLFW_KEY_MENU;
  publicKeys[KEY_NUMLOCK] = GLFW_KEY_NUM_LOCK;
  publicKeys[KEY_CAPSLOCK] = GLFW_KEY_CAPS_LOCK;
  publicKeys[KEY_PRINT] = GLFW_KEY_PRINT_SCREEN;
  publicKeys[KEY_SCROLLLOCK] = GLFW_KEY_SCROLL_LOCK;
  publicKeys[KEY_PAUSE] = GLFW_KEY_PAUSE;
  publicKeys[KEY_DELETE] = GLFW_KEY_DELETE;
  publicKeys[KEY_BACKSPACE] = GLFW_KEY_BACKSPACE;
  publicKeys[KEY_ENTER] = GLFW_KEY_ENTER;
  publicKeys[KEY_SPACE] = GLFW_KEY_SPACE;
  publicKeys[KEY_HOME] = GLFW_KEY_HOME;
  publicKeys[KEY_END] = GLFW_KEY_END;
  publicKeys[KEY_PAGEUP] = GLFW_KEY_PAGE_UP;
  publicKeys[KEY_PAGEDOWN] = GLFW_KEY_PAGE_DOWN;
  publicKeys[KEY_INSERT] = GLFW_KEY_INSERT;
  publicKeys[KEY_LEFT] = GLFW_KEY_LEFT;
  publicKeys[KEY_RIGHT] = GLFW_KEY_RIGHT;
  publicKeys[KEY_DOWN] = GLFW_KEY_DOWN;
  publicKeys[KEY_UP] = GLFW_KEY_UP;
  publicKeys[KEY_F1] = GLFW_KEY_F1;
  publicKeys[KEY_F2] = GLFW_KEY_F2;
  publicKeys[KEY_F3] = GLFW_KEY_F3;
  publicKeys[KEY_F4] = GLFW_KEY_F4;
  publicKeys[KEY_F5] = GLFW_KEY_F5;
  publicKeys[KEY_F6] = GLFW_KEY_F6;
  publicKeys[KEY_F7] = GLFW_KEY_F7;
  publicKeys[KEY_F8] = GLFW_KEY_F8;
  publicKeys[KEY_F9] = GLFW_KEY_F9;
  publicKeys[KEY_F10] = GLFW_KEY_F10;
  publicKeys[KEY_F11] = GLFW_KEY_F11;
  publicKeys[KEY_F12] = GLFW_KEY_F12;
  publicKeys[KEY_F13] = GLFW_KEY_F13;
  publicKeys[KEY_F14] = GLFW_KEY_F14;
  publicKeys[KEY_F15] = GLFW_KEY_F15;
  publicKeys[KEY_F16] = GLFW_KEY_F16;
  publicKeys[KEY_F17] = GLFW_KEY_F17;
  publicKeys[KEY_F18] = GLFW_KEY_F18;
  publicKeys[KEY_F19] = GLFW_KEY_F19;
  publicKeys[KEY_F20] = GLFW_KEY_F20;
  publicKeys[KEY_F21] = GLFW_KEY_F21;
  publicKeys[KEY_F22] = GLFW_KEY_F22;
  publicKeys[KEY_F23] = GLFW_KEY_F23;
  publicKeys[KEY_F24] = GLFW_KEY_F24;
  publicKeys[KEY_KPSLASH] = GLFW_KEY_KP_DIVIDE;
  publicKeys[KEY_KPDOT] = GLFW_KEY_KP_MULTIPLY;
  publicKeys[KEY_KPMINUS] = GLFW_KEY_KP_SUBTRACT;
  publicKeys[KEY_KPPLUS] = GLFW_KEY_KP_ADD;
  publicKeys[KEY_KP0] = GLFW_KEY_KP_0;
  publicKeys[KEY_KP1] = GLFW_KEY_KP_1;
  publicKeys[KEY_KP2] = GLFW_KEY_KP_2;
  publicKeys[KEY_KP3] = GLFW_KEY_KP_3;
  publicKeys[KEY_KP4] = GLFW_KEY_KP_4;
  publicKeys[KEY_KP5] = GLFW_KEY_KP_5;
  publicKeys[KEY_KP6] = GLFW_KEY_KP_6;
  publicKeys[KEY_KP7] = GLFW_KEY_KP_7;
  publicKeys[KEY_KP8] = GLFW_KEY_KP_8;
  publicKeys[KEY_KP9] = GLFW_KEY_KP_9;
  publicKeys[KEY_KPCOMMA] = GLFW_KEY_KP_DECIMAL;
  publicKeys[KEY_KPEQUAL] = GLFW_KEY_KP_EQUAL;
  publicKeys[KEY_KPENTER] = GLFW_KEY_KP_ENTER;
}

int toGLFWKeyCode(const uint32_t key) {
  static auto initialized = false;

  if (!initialized) {
    createKeyTables();
    initialized = true;
  }

  if (key < sizeof(publicKeys) / sizeof(publicKeys[0]))
    return publicKeys[key];

  return GLFW_KEY_UNKNOWN;
}

#define ADD_FLAG_IF(flag, bitfield, cond) \
  do                                      \
    if (cond)                             \
      bitfield |= flag;                   \
  while (0)

int toGLFWModifiers(SimpleKeyboardModifiers& mods) {
  int glfw_flags = 0;
  ADD_FLAG_IF(GLFW_MOD_SHIFT, glfw_flags, mods.getShift());
  ADD_FLAG_IF(GLFW_MOD_CONTROL, glfw_flags, mods.getCtrl());
  ADD_FLAG_IF(GLFW_MOD_ALT, glfw_flags, mods.getAlt());
  ADD_FLAG_IF(GLFW_MOD_SUPER, glfw_flags, mods.getSuper());
  ADD_FLAG_IF(GLFW_MOD_CAPS_LOCK, glfw_flags, mods.getCaps());
  ADD_FLAG_IF(GLFW_MOD_NUM_LOCK, glfw_flags, mods.getNum());
  return glfw_flags;
}
// }


}  // namespace flutter
