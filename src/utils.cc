// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>
#include <unistd.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <linux/input-event-codes.h>

#include "utils.h"

namespace flutter {

static std::string GetExecutablePath() {
  char executable_path[1024] = {0};
  std::stringstream stream;
  stream << "/proc/" << getpid() << "/exe";
  auto path                 = stream.str();
  auto executable_path_size = ::readlink(path.c_str(), executable_path, sizeof(executable_path));
  if (executable_path_size <= 0) {
    return "";
  }
  return std::string{executable_path, static_cast<size_t>(executable_path_size)};
}

std::string GetExecutableName() {
  auto path_string = GetExecutablePath();
  auto found       = path_string.find_last_of('/');
  if (found == std::string::npos) {
    return "";
  }
  return path_string.substr(found + 1);
}

std::string GetExecutableDirectory() {
  auto path_string = GetExecutablePath();
  auto found       = path_string.find_last_of('/');
  if (found == std::string::npos) {
    return "";
  }
  return path_string.substr(0, found + 1);
}

bool FileExistsAtPath(const std::string &path) {
  return ::access(path.c_str(), R_OK) == 0;
}

bool FlutterAssetBundleIsValid(const std::string &bundle_path) {
  if (!FileExistsAtPath(bundle_path)) {
    FLWAY_ERROR << "Bundle directory: '" << bundle_path << "' does not exist." << std::endl;
    return false;
  }

  if (!FileExistsAtPath(bundle_path + std::string{"/kernel_blob.bin"})) {
    FLWAY_ERROR << "Kernel blob does not exist." << std::endl;
    return false;
  }

  return true;
}

bool FlutterSendMessage(FlutterEngine engine, const char *channel, const uint8_t *message, const size_t message_size) {
  FlutterPlatformMessageResponseHandle *response_handle = nullptr;

  FlutterPlatformMessage platform_message = {
      sizeof(FlutterPlatformMessage), .channel = channel, .message = message, .message_size = message_size, .response_handle = response_handle,
  };

  FlutterEngineResult message_result = FlutterEngineSendPlatformMessage(engine, &platform_message);

  if (response_handle != nullptr) {
    FlutterPlatformMessageReleaseResponseHandle(engine, response_handle);
  }

  return message_result == kSuccess;
}

// Borrowed from glfw library {

static short int publicKeys[256];

static void createKeyTables(void) {
  memset(publicKeys, -1, sizeof(publicKeys));

  publicKeys[KEY_GRAVE]      = GLFW_KEY_GRAVE_ACCENT;
  publicKeys[KEY_1]          = GLFW_KEY_1;
  publicKeys[KEY_2]          = GLFW_KEY_2;
  publicKeys[KEY_3]          = GLFW_KEY_3;
  publicKeys[KEY_4]          = GLFW_KEY_4;
  publicKeys[KEY_5]          = GLFW_KEY_5;
  publicKeys[KEY_6]          = GLFW_KEY_6;
  publicKeys[KEY_7]          = GLFW_KEY_7;
  publicKeys[KEY_8]          = GLFW_KEY_8;
  publicKeys[KEY_9]          = GLFW_KEY_9;
  publicKeys[KEY_0]          = GLFW_KEY_0;
  publicKeys[KEY_MINUS]      = GLFW_KEY_MINUS;
  publicKeys[KEY_EQUAL]      = GLFW_KEY_EQUAL;
  publicKeys[KEY_Q]          = GLFW_KEY_Q;
  publicKeys[KEY_W]          = GLFW_KEY_W;
  publicKeys[KEY_E]          = GLFW_KEY_E;
  publicKeys[KEY_R]          = GLFW_KEY_R;
  publicKeys[KEY_T]          = GLFW_KEY_T;
  publicKeys[KEY_Y]          = GLFW_KEY_Y;
  publicKeys[KEY_U]          = GLFW_KEY_U;
  publicKeys[KEY_I]          = GLFW_KEY_I;
  publicKeys[KEY_O]          = GLFW_KEY_O;
  publicKeys[KEY_P]          = GLFW_KEY_P;
  publicKeys[KEY_LEFTBRACE]  = GLFW_KEY_LEFT_BRACKET;
  publicKeys[KEY_RIGHTBRACE] = GLFW_KEY_RIGHT_BRACKET;
  publicKeys[KEY_A]          = GLFW_KEY_A;
  publicKeys[KEY_S]          = GLFW_KEY_S;
  publicKeys[KEY_D]          = GLFW_KEY_D;
  publicKeys[KEY_F]          = GLFW_KEY_F;
  publicKeys[KEY_G]          = GLFW_KEY_G;
  publicKeys[KEY_H]          = GLFW_KEY_H;
  publicKeys[KEY_J]          = GLFW_KEY_J;
  publicKeys[KEY_K]          = GLFW_KEY_K;
  publicKeys[KEY_L]          = GLFW_KEY_L;
  publicKeys[KEY_SEMICOLON]  = GLFW_KEY_SEMICOLON;
  publicKeys[KEY_APOSTROPHE] = GLFW_KEY_APOSTROPHE;
  publicKeys[KEY_Z]          = GLFW_KEY_Z;
  publicKeys[KEY_X]          = GLFW_KEY_X;
  publicKeys[KEY_C]          = GLFW_KEY_C;
  publicKeys[KEY_V]          = GLFW_KEY_V;
  publicKeys[KEY_B]          = GLFW_KEY_B;
  publicKeys[KEY_N]          = GLFW_KEY_N;
  publicKeys[KEY_M]          = GLFW_KEY_M;
  publicKeys[KEY_COMMA]      = GLFW_KEY_COMMA;
  publicKeys[KEY_DOT]        = GLFW_KEY_PERIOD;
  publicKeys[KEY_SLASH]      = GLFW_KEY_SLASH;
  publicKeys[KEY_BACKSLASH]  = GLFW_KEY_BACKSLASH;
  publicKeys[KEY_ESC]        = GLFW_KEY_ESCAPE;
  publicKeys[KEY_TAB]        = GLFW_KEY_TAB;
  publicKeys[KEY_LEFTSHIFT]  = GLFW_KEY_LEFT_SHIFT;
  publicKeys[KEY_RIGHTSHIFT] = GLFW_KEY_RIGHT_SHIFT;
  publicKeys[KEY_LEFTCTRL]   = GLFW_KEY_LEFT_CONTROL;
  publicKeys[KEY_RIGHTCTRL]  = GLFW_KEY_RIGHT_CONTROL;
  publicKeys[KEY_LEFTALT]    = GLFW_KEY_LEFT_ALT;
  publicKeys[KEY_RIGHTALT]   = GLFW_KEY_RIGHT_ALT;
  publicKeys[KEY_LEFTMETA]   = GLFW_KEY_LEFT_SUPER;
  publicKeys[KEY_RIGHTMETA]  = GLFW_KEY_RIGHT_SUPER;
  publicKeys[KEY_MENU]       = GLFW_KEY_MENU;
  publicKeys[KEY_NUMLOCK]    = GLFW_KEY_NUM_LOCK;
  publicKeys[KEY_CAPSLOCK]   = GLFW_KEY_CAPS_LOCK;
  publicKeys[KEY_PRINT]      = GLFW_KEY_PRINT_SCREEN;
  publicKeys[KEY_SCROLLLOCK] = GLFW_KEY_SCROLL_LOCK;
  publicKeys[KEY_PAUSE]      = GLFW_KEY_PAUSE;
  publicKeys[KEY_DELETE]     = GLFW_KEY_DELETE;
  publicKeys[KEY_BACKSPACE]  = GLFW_KEY_BACKSPACE;
  publicKeys[KEY_ENTER]      = GLFW_KEY_ENTER;
  publicKeys[KEY_HOME]       = GLFW_KEY_HOME;
  publicKeys[KEY_END]        = GLFW_KEY_END;
  publicKeys[KEY_PAGEUP]     = GLFW_KEY_PAGE_UP;
  publicKeys[KEY_PAGEDOWN]   = GLFW_KEY_PAGE_DOWN;
  publicKeys[KEY_INSERT]     = GLFW_KEY_INSERT;
  publicKeys[KEY_LEFT]       = GLFW_KEY_LEFT;
  publicKeys[KEY_RIGHT]      = GLFW_KEY_RIGHT;
  publicKeys[KEY_DOWN]       = GLFW_KEY_DOWN;
  publicKeys[KEY_UP]         = GLFW_KEY_UP;
  publicKeys[KEY_F1]         = GLFW_KEY_F1;
  publicKeys[KEY_F2]         = GLFW_KEY_F2;
  publicKeys[KEY_F3]         = GLFW_KEY_F3;
  publicKeys[KEY_F4]         = GLFW_KEY_F4;
  publicKeys[KEY_F5]         = GLFW_KEY_F5;
  publicKeys[KEY_F6]         = GLFW_KEY_F6;
  publicKeys[KEY_F7]         = GLFW_KEY_F7;
  publicKeys[KEY_F8]         = GLFW_KEY_F8;
  publicKeys[KEY_F9]         = GLFW_KEY_F9;
  publicKeys[KEY_F10]        = GLFW_KEY_F10;
  publicKeys[KEY_F11]        = GLFW_KEY_F11;
  publicKeys[KEY_F12]        = GLFW_KEY_F12;
  publicKeys[KEY_F13]        = GLFW_KEY_F13;
  publicKeys[KEY_F14]        = GLFW_KEY_F14;
  publicKeys[KEY_F15]        = GLFW_KEY_F15;
  publicKeys[KEY_F16]        = GLFW_KEY_F16;
  publicKeys[KEY_F17]        = GLFW_KEY_F17;
  publicKeys[KEY_F18]        = GLFW_KEY_F18;
  publicKeys[KEY_F19]        = GLFW_KEY_F19;
  publicKeys[KEY_F20]        = GLFW_KEY_F20;
  publicKeys[KEY_F21]        = GLFW_KEY_F21;
  publicKeys[KEY_F22]        = GLFW_KEY_F22;
  publicKeys[KEY_F23]        = GLFW_KEY_F23;
  publicKeys[KEY_F24]        = GLFW_KEY_F24;
  publicKeys[KEY_KPSLASH]    = GLFW_KEY_KP_DIVIDE;
  publicKeys[KEY_KPDOT]      = GLFW_KEY_KP_MULTIPLY;
  publicKeys[KEY_KPMINUS]    = GLFW_KEY_KP_SUBTRACT;
  publicKeys[KEY_KPPLUS]     = GLFW_KEY_KP_ADD;
  publicKeys[KEY_KP0]        = GLFW_KEY_KP_0;
  publicKeys[KEY_KP1]        = GLFW_KEY_KP_1;
  publicKeys[KEY_KP2]        = GLFW_KEY_KP_2;
  publicKeys[KEY_KP3]        = GLFW_KEY_KP_3;
  publicKeys[KEY_KP4]        = GLFW_KEY_KP_4;
  publicKeys[KEY_KP5]        = GLFW_KEY_KP_5;
  publicKeys[KEY_KP6]        = GLFW_KEY_KP_6;
  publicKeys[KEY_KP7]        = GLFW_KEY_KP_7;
  publicKeys[KEY_KP8]        = GLFW_KEY_KP_8;
  publicKeys[KEY_KP9]        = GLFW_KEY_KP_9;
  publicKeys[KEY_KPCOMMA]    = GLFW_KEY_KP_DECIMAL;
  publicKeys[KEY_KPEQUAL]    = GLFW_KEY_KP_EQUAL;
  publicKeys[KEY_KPENTER]    = GLFW_KEY_KP_ENTER;
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
// }

} // namespace flutter
