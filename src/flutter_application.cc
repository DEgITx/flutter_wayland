// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_application.h"

#include <EGL/egl.h>
#include <sys/types.h>

#include <chrono>
#include <sstream>
#include <vector>

#include "logger.h"
#include "utils.h"

namespace flutter {

static_assert(FLUTTER_ENGINE_VERSION == 1, "");

static const char* kICUDataFileName = "icudtl.dat";

static std::string GetICUDataPath() {
  auto exe_dir = GetExecutableDirectory();
  if (exe_dir == "") {
    return "";
  }
  std::stringstream stream;
  stream << exe_dir << kICUDataFileName;

  auto icu_path = stream.str();

  if (!FileExistsAtPath(icu_path.c_str())) {
    SPDLOG_ERROR("Could not find {}", icu_path);
    return "";
  }

  return icu_path;
}

FlutterApplication::FlutterApplication(
    std::string bundle_path,
    const std::vector<std::string>& command_line_args,
    RenderDelegate& render_delegate,
    EventEmitter& event_emitter)
    : render_delegate_(render_delegate),
      event_emitter_(event_emitter),
      display_event_listener_(this) {
  if (!FlutterAssetBundleIsValid(bundle_path)) {
    SPDLOG_ERROR("Flutter asset bundle was not valid.");
    return;
  }

  FlutterRendererConfig config = {};
  config.type = kOpenGL;
  config.open_gl.struct_size = sizeof(config.open_gl);
  config.open_gl.make_current = [](void* userdata) -> bool {
    return reinterpret_cast<FlutterApplication*>(userdata)
        ->render_delegate_.OnApplicationContextMakeCurrent();
  };
  config.open_gl.clear_current = [](void* userdata) -> bool {
    return reinterpret_cast<FlutterApplication*>(userdata)
        ->render_delegate_.OnApplicationContextClearCurrent();
  };
  config.open_gl.present = [](void* userdata) -> bool {
    return reinterpret_cast<FlutterApplication*>(userdata)
        ->render_delegate_.OnApplicationPresent();
  };
  config.open_gl.fbo_callback = [](void* userdata) -> uint32_t {
    return reinterpret_cast<FlutterApplication*>(userdata)
        ->render_delegate_.OnApplicationGetOnscreenFBO();
  };
  config.open_gl.gl_proc_resolver = [](void* userdata,
                                       const char* name) -> void* {
    auto address = eglGetProcAddress(name);
    if (address != nullptr) {
      return reinterpret_cast<void*>(address);
    }
    SPDLOG_ERROR("Tried unsuccessfully to resolve: {}", name);
    return nullptr;
  };

  auto icu_data_path = GetICUDataPath();

  if (icu_data_path == "") {
    SPDLOG_ERROR(
        "Could not find ICU data. It should be placed next to the "
        "executable but it wasn't there.");
    return;
  }

  std::vector<const char*> command_line_args_c;

  for (const auto& arg : command_line_args) {
    command_line_args_c.push_back(arg.c_str());
  }

  FlutterProjectArgs args = {0};
  args.struct_size = sizeof(FlutterProjectArgs);
  args.assets_path = bundle_path.c_str();
  args.icu_data_path = icu_data_path.c_str();
  args.command_line_argc = static_cast<int>(command_line_args_c.size());
  args.command_line_argv = command_line_args_c.data();

  auto result = FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config, &args,
                                 this /* userdata */, &engine_);

  if (result != kSuccess) {
    SPDLOG_ERROR("Could not run the Flutter engine");
    return;
  }

  event_emitter_.addListener(&display_event_listener_);

  valid_ = true;
}

FlutterApplication::~FlutterApplication() {
  event_emitter_.removeListener(&display_event_listener_);

  if (engine_ == nullptr) {
    return;
  }

  auto result = FlutterEngineShutdown(engine_);

  if (result != kSuccess) {
    SPDLOG_ERROR("Could not shutdown the Flutter engine.");
  }
}

bool FlutterApplication::IsValid() const {
  return valid_;
}

bool FlutterApplication::SetWindowSize(size_t width, size_t height) {
  FlutterWindowMetricsEvent event = {};
  event.struct_size = sizeof(event);
  event.width = width;
  event.height = height;
  event.pixel_ratio = 1.0;
  return FlutterEngineSendWindowMetricsEvent(engine_, &event) == kSuccess;
}

void FlutterApplication::ProcessEvents() {
  __FlutterEngineFlushPendingTasksNow();
}

bool FlutterApplication::SendPointerEvent(int button, int x, int y) {
  if (!valid_) {
    SPDLOG_ERROR("Pointer events on an invalid application.");
    return false;
  }

  // Simple hover event. Nothing to do.
  if (last_button_ == 0 && button == 0) {
    return true;
  }

  FlutterPointerPhase phase = kCancel;

  if (last_button_ == 0 && button != 0) {
    phase = kDown;
  } else if (last_button_ == button) {
    phase = kMove;
  } else {
    phase = kUp;
  }

  last_button_ = button;
  return SendFlutterPointerEvent(phase, x, y);
}

bool FlutterApplication::SendFlutterPointerEvent(FlutterPointerPhase phase,
                                                 double x,
                                                 double y) {
  FlutterPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
  event.x = x;
  event.y = y;
  event.timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  return FlutterEngineSendPointerEvent(engine_, &event, 1) == kSuccess;
}

bool FlutterApplication::SendPlatformMessage(const char* channel,
                                             const uint8_t* message,
                                             const size_t message_size) {
  FlutterPlatformMessageResponseHandle* response_handle = nullptr;

  FlutterPlatformMessage platform_message = {
      sizeof(FlutterPlatformMessage),
      .channel = channel,
      .message = message,
      .message_size = message_size,
      .response_handle = response_handle,
  };

  FlutterEngineResult message_result =
      FlutterEngineSendPlatformMessage(engine_, &platform_message);

  if (response_handle != nullptr) {
    FlutterPlatformMessageReleaseResponseHandle(engine_, response_handle);
  }

  return message_result == kSuccess;
}

void FlutterApplication::OnKeyboardKey(uint32_t evdevKeycode,
                                       uint32_t xkbKeycode,
                                       uint32_t utf32,
                                       bool pressed,
                                       SimpleKeyboardModifiers& mods) {
  kKeyEventMessage["keyCode"] = toGLFWKeyCode(evdevKeycode);
  kKeyEventMessage["scanCode"] = xkbKeycode;
  kKeyEventMessage["modifiers"] = toGLFWModifiers(mods);
  kKeyEventMessage["unicodeScalarValues"] = utf32;
  kKeyEventMessage["type"] = pressed ? "keydown" : "keyup";

  std::string s = kKeyEventMessage.dump();

  SPDLOG_TRACE("Sending PlatformMessage: {}", s);

  bool success = SendPlatformMessage(
      "flutter/keyevent", reinterpret_cast<const uint8_t*>(s.c_str()),
      s.size());

  if (!success) {
    SPDLOG_ERROR("Error sending PlatformMessage: {}", s);
  }
}

}  // namespace flutter
