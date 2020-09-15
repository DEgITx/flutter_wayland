// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <flutter_embedder.h>

#include <functional>
#include <vector>

#include <nlohmann/json.hpp>

#include "keys.h"
#include "logger.h"
#include "macros.h"

namespace flutter {

class FlutterApplication {
 public:
  class RenderDelegate {
   public:
    virtual bool OnApplicationContextMakeCurrent() = 0;

    virtual bool OnApplicationContextClearCurrent() = 0;

    virtual bool OnApplicationPresent() = 0;

    virtual uint32_t OnApplicationGetOnscreenFBO() = 0;
  };
  class EventListener {
   public:
    virtual void OnKeyboardKey(uint32_t evdevKeycode,
                               uint32_t xkbKeycode,
                               uint32_t utf32,
                               bool pressed,
                               SimpleKeyboardModifiers& mods) = 0;
  };
  class EventEmitter {
   protected:
    std::vector<EventListener*> kEventListeners;

   public:
    void addListener(EventListener* l) { kEventListeners.push_back(l); }

    void removeListener(EventListener* l) {
      std::remove(kEventListeners.begin(), kEventListeners.end(), l);
    }
  };

  FlutterApplication(std::string bundle_path,
                     const std::vector<std::string>& args,
                     RenderDelegate& render_delegate,
                     EventEmitter& event_emitter);

  ~FlutterApplication();

  bool IsValid() const;

  void ProcessEvents();

  bool SetWindowSize(size_t width, size_t height);

  bool SendPointerEvent(int button, int x, int y);

 private:
  class DisplayEventListener : public EventListener {
    FlutterApplication* parent;

   public:
    DisplayEventListener(FlutterApplication* p) { parent = p; }

    void OnKeyboardKey(uint32_t evdevKeycode,
                       uint32_t xkbKeycode,
                       uint32_t utf32,
                       bool pressed,
                       SimpleKeyboardModifiers& mods) {
      SPDLOG_DEBUG(
          "evdevKeycode = {} xkbKeycode = {} utf32 = U+{:X} pressed = {}",
          evdevKeycode, xkbKeycode, utf32, pressed);
      parent->OnKeyboardKey(evdevKeycode, xkbKeycode, utf32, pressed, mods);
    }
  } display_event_listener_;

  nlohmann::json kKeyEventMessage = {
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
  EventEmitter& event_emitter_;
  FlutterEngine engine_ = nullptr;
  int last_button_ = 0;

  void OnKeyboardKey(uint32_t evdevKeycode,
                     uint32_t xkbKeycode,
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
