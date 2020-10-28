// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <flutter_embedder.h>

#include <functional>
#include <vector>

// nasty fix of clang compilation with C++17
#if defined(__clang__) && defined(__cplusplus) && __cplusplus == 201703L
#define __cplusplus 201402L
#include <nlohmann/json.hpp>
#define __cplusplus 201703L
#else
#include <nlohmann/json.hpp>
#endif

#include "keys.h"
#include "logger.h"
#include "macros.h"
#include "render_delegate.h"
#include "display_event_emitter.h"
#include "display_event_listener.h"

namespace flutter {

class FlutterApplication {

public:
  FlutterApplication(std::string bundle_path,
                     const std::vector<std::string>& args,
                     RenderDelegate& render_delegate,
                     DisplayEventEmitter& display_event_emitter);

  ~FlutterApplication();

  bool IsValid() const;

  void ProcessEvents();

  bool SetWindowSize(size_t width, size_t height);

  bool SendPointerEvent(int button, int x, int y);

 private:
  class EventListener : public DisplayEventListener {
    FlutterApplication* parent;

   public:
    EventListener(FlutterApplication* p) { parent = p; }

    void OnKeyboardKey(uint32_t evdev_keycode,
                       uint32_t xkb_keycode,
                       uint32_t utf32,
                       bool pressed,
                       SimpleKeyboardModifiers& mods) {
      SPDLOG_DEBUG(
          "evdev_keycode = {} xkb_keycode = {} utf32 = U+{:X} pressed = {}",
          evdev_keycode, xkb_keycode, utf32, pressed);
      parent->OnKeyboardKey(evdev_keycode, xkb_keycode, utf32, pressed, mods);
    }
  } display_event_listener_;

  nlohmann::json kKeyEventMessageLinux = {
    {"keyCode", 0},
    {"keymap", "linux"},
    {"scanCode", 0},
    {"modifiers", 0},
    {"toolkit", "glfw"},
    {"unicodeScalarValues", 0},
    {"type", ""}
  };

  bool valid_;
  RenderDelegate& render_delegate_;
  DisplayEventEmitter& display_event_emitter_;
  FlutterEngine engine_ = nullptr;
  int last_button_ = 0;

  void OnKeyboardKey(uint32_t evdev_keycode,
                     uint32_t xkb_keycode,
                     uint32_t utf32,
                     bool pressed,
                     SimpleKeyboardModifiers& mods);

  bool SendFlutterPointerEvent(FlutterPointerPhase phase, double x, double y);
  bool SendPlatformMessage(const char* channel,
                           const uint8_t* message,
                           const size_t message_size);

  FLWAY_DISALLOW_COPY_AND_ASSIGN(FlutterApplication);
};

}  // namespace flutter
