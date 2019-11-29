// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WL_EGL_PLATFORM
#define WL_EGL_PLATFORM 1
#endif

#include <fstream>
#include <chrono>
#include <sstream>
#include <vector>
#include <functional>

#include <cstring>
#include <cassert>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/mman.h>

#include "utils.h"
#include "wayland_display.h"

namespace flutter {

static std::string GetICUDataPath() {
  auto base_directory = GetExecutableDirectory();
  if (base_directory == "") {
    base_directory = ".";
  }

  std::string data_directory = base_directory + "/data";
  std::string icu_data_path  = data_directory + "/icudtl.dat";

  do {
    if (std::ifstream(icu_data_path)) {
      std::cout << "Using: " << icu_data_path << std::endl;
      break;
    }

    icu_data_path = "/usr/share/flutter/icudtl.dat";

    if (std::ifstream(icu_data_path)) {
      std::cout << "Using: " << icu_data_path << std::endl;
      break;
    }

    FLWAY_ERROR << "Unnable to locate icudtl.dat file" << std::endl;
    icu_data_path = "";
  } while (0);

  return icu_data_path;
}

#define DISPLAY reinterpret_cast<WaylandDisplay *>(data)

const wl_registry_listener WaylandDisplay::kRegistryListener = {
    .global = [](void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) -> void { DISPLAY->AnnounceRegistryInterface(wl_registry, name, interface, version); },

    .global_remove = [](void *data, struct wl_registry *wl_registry, uint32_t name) -> void { DISPLAY->UnannounceRegistryInterface(wl_registry, name); },
};

const wl_shell_surface_listener WaylandDisplay::kShellSurfaceListener = {
    .ping = [](void *data, struct wl_shell_surface *wl_shell_surface, uint32_t serial) -> void { wl_shell_surface_pong(DISPLAY->shell_surface_, serial); },

    .configure = [](void *data, struct wl_shell_surface *wl_shell_surface, uint32_t edges, int32_t width, int32_t height) -> void {
      WaylandDisplay *wd = reinterpret_cast<WaylandDisplay *>(data);

      if (wd == nullptr)
        return;

      if (wd->window_ == nullptr)
        return;

      wl_egl_window_resize(wd->window_, width, height, 0, 0);

      FLWAY_LOG << "Window resized: " << width << "x" << height << std::endl;
    },

    .popup_done = [](void *data, struct wl_shell_surface *wl_shell_surface) -> void {
      // Nothing to do.
    },
};

const wl_pointer_listener WaylandDisplay::kPointerListener = {
    .enter = [](void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {},

    .leave = [](void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface) {},

    .motion =
        [](void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
          WaylandDisplay *wd = DISPLAY;
          assert(wd);
          // just store raw values
          wd->surface_x = surface_x;
          wd->surface_y = surface_y;
        },

    .button =
        [](void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
          WaylandDisplay *wd = DISPLAY;
          assert(wd);

          uint32_t button_number = button - 0x110; // dw: 0x110 is BTN_LEFT - a magic value from the header we don't want to pull in as a build dependency
          button_number          = button_number == 1 ? 2 : button_number == 2 ? 1 : button_number;

          FlutterPointerEvent event = {
              .struct_size    = sizeof(event),
              .phase          = state == WL_POINTER_BUTTON_STATE_PRESSED ? FlutterPointerPhase::kDown : FlutterPointerPhase::kUp,
              .timestamp      = time * 1000,
              .x              = wl_fixed_to_double(wd->surface_x),
              .y              = wl_fixed_to_double(wd->surface_y),
              .device         = 0,
              .signal_kind    = kFlutterPointerSignalKindNone,
              .scroll_delta_x = 0,
              .scroll_delta_y = 0,
              .device_kind    = static_cast<FlutterPointerDeviceKind>(0), // dw: TODO: Why kFlutterPointerDeviceKindMouse does not work?
              .buttons        = 0,
          };

          FlutterEngineSendPointerEvent(wd->engine_, &event, 1);
        },

    .axis = [](void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {},

    .frame = [](void *data, struct wl_pointer *wl_pointer) {},

    .axis_source = [](void *data, struct wl_pointer *wl_pointer, uint32_t axis_source) {},

    .axis_stop = [](void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis) {},

    .axis_discrete = [](void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete) {},

};

const wl_keyboard_listener WaylandDisplay::kKeyboardListener = {
    .keymap =
        [](void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd, uint32_t size) {
          WaylandDisplay *wd = DISPLAY;
          assert(wd);

          wd->keymap_format         = static_cast<wl_keyboard_keymap_format>(format);
          char *const keymap_string = reinterpret_cast<char *const>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
          xkb_keymap_unref(wd->keymap);
          wd->keymap = xkb_keymap_new_from_string(wd->xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
          munmap(keymap_string, size);
          close(fd);
          xkb_state_unref(wd->xkb_state);
          wd->xkb_state = xkb_state_new(wd->keymap);
        },

    .enter = [](void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) { printf("keyboard enter\n"); },

    .leave = [](void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface) { printf("keyboard leave\n"); },

    .key =
        [](void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
          if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
            WaylandDisplay *wd = DISPLAY;
            assert(wd);

            if (wd->keymap_format == WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP) {
              printf("Hmm - no keymap, no key\n");
              return;
            }

            xkb_keysym_t keysym = xkb_state_key_get_one_sym(wd->xkb_state, key + (wd->keymap_format * 8));
            uint32_t utf32      = xkb_keysym_to_utf32(keysym);

            if (utf32) {
              if (utf32 >= 0x21 && utf32 <= 0x7E) {
                printf("the key %c was pressed\n", (char)utf32);
              } else {
                printf("the key U+%04X was pressed\n", utf32);
              }
            } else {
              char name[64];
              xkb_keysym_get_name(keysym, name, sizeof(name));
              printf("the key %s was pressed\n", name);
            }
          }
        },

    .modifiers =
        [](void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
          WaylandDisplay *wd = DISPLAY;
          assert(wd);

          xkb_state_update_mask(wd->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
        },

    .repeat_info = [](void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay) {},
};

const wl_seat_listener WaylandDisplay::kSeatListener = {
    .capabilities =
        [](void *data, struct wl_seat *seat, uint32_t capabilities) {
          printf("capabilities(data:%p, seat:%p, capabilities:0x%x)\n", data, seat, capabilities);

          WaylandDisplay *wd = DISPLAY;
          assert(wd);
          assert(seat == wd->seat_);

          if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
            printf("seat_capabilities - pointer\n");
            struct wl_pointer *pointer = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(pointer, &kPointerListener, wd);
          }

          if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
            printf("seat_capabilities - keyboard\n");
            struct wl_keyboard *keyboard = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(keyboard, &kKeyboardListener, wd);
          }

          if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
            printf("seat_capabilities - touch\n");
          }
        },

    .name =
        [](void *data, struct wl_seat *wl_seat, const char *name) {
          // Nothing to do.
        },
};

WaylandDisplay::WaylandDisplay(size_t width, size_t height, const std::string &bundle_path, const std::vector<std::string> &command_line_args)
    : xkb_context(xkb_context_new(XKB_CONTEXT_NO_FLAGS))
    , screen_width_(width)
    , screen_height_(height) {
  if (screen_width_ == 0 || screen_height_ == 0) {
    FLWAY_ERROR << "Invalid screen dimensions." << std::endl;
    return;
  }

  display_ = wl_display_connect(nullptr);

  if (!display_) {
    FLWAY_ERROR << "Could not connect to the wayland display." << std::endl;
    return;
  }

  registry_ = wl_display_get_registry(display_);
  if (!registry_) {
    FLWAY_ERROR << "Could not get the wayland registry." << std::endl;
    return;
  }

  wl_registry_add_listener(registry_, &kRegistryListener, this);

  wl_display_roundtrip(display_);

  if (!SetupEGL()) {
    FLWAY_ERROR << "Could not setup EGL." << std::endl;
    return;
  }

  if (!SetupEngine(bundle_path, command_line_args)) {
    FLWAY_ERROR << "Could not setup Flutter Engine." << std::endl;
    return;
  }

  valid_ = true;
}

bool WaylandDisplay::SetupEngine(const std::string &bundle_path, const std::vector<std::string> &command_line_args) {
  FlutterRendererConfig config    = {};
  config.type                     = kOpenGL;
  config.open_gl.struct_size      = sizeof(config.open_gl);
  config.open_gl.make_current     = [](void *userdata) -> bool { return reinterpret_cast<WaylandDisplay *>(userdata)->OnApplicationContextMakeCurrent(); };
  config.open_gl.clear_current    = [](void *userdata) -> bool { return reinterpret_cast<WaylandDisplay *>(userdata)->OnApplicationContextClearCurrent(); };
  config.open_gl.present          = [](void *userdata) -> bool { return reinterpret_cast<WaylandDisplay *>(userdata)->OnApplicationPresent(); };
  config.open_gl.fbo_callback     = [](void *userdata) -> uint32_t { return reinterpret_cast<WaylandDisplay *>(userdata)->OnApplicationGetOnscreenFBO(); };
  config.open_gl.gl_proc_resolver = [](void *userdata, const char *name) -> void * {
    auto address = eglGetProcAddress(name);
    if (address != nullptr) {
      return reinterpret_cast<void *>(address);
    }
    FLWAY_ERROR << "Tried unsuccessfully to resolve: " << name << std::endl;
    return nullptr;
  };

  auto icu_data_path = GetICUDataPath();

  if (icu_data_path == "") {
    return false;
  }

  std::vector<const char *> command_line_args_c;

  for (const auto &arg : command_line_args) {
    command_line_args_c.push_back(arg.c_str());
  }

  FlutterProjectArgs args = {
      .struct_size       = sizeof(FlutterProjectArgs),
      .assets_path       = bundle_path.c_str(),
      .icu_data_path     = icu_data_path.c_str(),
      .command_line_argc = static_cast<int>(command_line_args_c.size()),
      .command_line_argv = command_line_args_c.data(),
  };

  auto result = FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config, &args, this /* userdata */, &engine_);

  if (result != kSuccess) {
    FLWAY_ERROR << "Could not run the Flutter engine" << std::endl;
    return false;
  }

  FlutterWindowMetricsEvent event = {};
  event.struct_size               = sizeof(event);
  event.width                     = screen_width_;
  event.height                    = screen_height_;
  event.pixel_ratio               = 1.0;

  return FlutterEngineSendWindowMetricsEvent(engine_, &event) == kSuccess;
}

WaylandDisplay::~WaylandDisplay() {

  if (engine_ == nullptr) {
    auto result = FlutterEngineShutdown(engine_);
    if (result != kSuccess) {
      FLWAY_ERROR << "Could not shutdown the Flutter engine." << std::endl;
    }
  }

  if (shell_surface_) {
    wl_shell_surface_destroy(shell_surface_);
    shell_surface_ = nullptr;
  }

  if (shell_) {
    wl_shell_destroy(shell_);
    shell_ = nullptr;
  }

  if (seat_) {
    wl_seat_destroy(seat_);
    seat_ = nullptr;
  }

  if (egl_surface_) {
    eglDestroySurface(egl_display_, egl_surface_);
    egl_surface_ = nullptr;
  }

  if (egl_display_) {
    eglTerminate(egl_display_);
    egl_display_ = nullptr;
  }

  if (window_) {
    wl_egl_window_destroy(window_);
    window_ = nullptr;
  }

  if (surface_) {
    wl_surface_destroy(surface_);
    surface_ = nullptr;
  }

  if (compositor_) {
    wl_compositor_destroy(compositor_);
    compositor_ = nullptr;
  }

  if (registry_) {
    wl_registry_destroy(registry_);
    registry_ = nullptr;
  }

  if (display_) {
    wl_display_flush(display_);
    wl_display_disconnect(display_);
    display_ = nullptr;
  }
}

bool WaylandDisplay::IsValid() const {
  return valid_;
}

bool WaylandDisplay::Run() {
  if (!valid_) {
    FLWAY_ERROR << "Could not run an invalid display." << std::endl;
    return false;
  }

  const int fd = wl_display_get_fd(display_);

  while (valid_) {
    while (wl_display_prepare_read(display_) < 0) {
      wl_display_dispatch_pending(display_);
    }

    wl_display_flush(display_);

    int rv;

    do {
      struct pollfd fds = {.fd = fd, .events = POLLIN};

      rv = poll(&fds, 1, 1);
    } while (rv == -1 && rv == EINTR);

    if (rv <= 0) {
      wl_display_cancel_read(display_);
    } else {
      wl_display_read_events(display_);
    }

    wl_display_dispatch_pending(display_);
  }

  return true;
}

static void LogLastEGLError() {
  struct EGLNameErrorPair {
    const char *name;
    EGLint code;
  };

#define _EGL_ERROR_DESC(a)                                                                                                                                                                                                                     \
  { #a, a }

  const EGLNameErrorPair pairs[] = {
      _EGL_ERROR_DESC(EGL_SUCCESS),     _EGL_ERROR_DESC(EGL_NOT_INITIALIZED), _EGL_ERROR_DESC(EGL_BAD_ACCESS),          _EGL_ERROR_DESC(EGL_BAD_ALLOC),         _EGL_ERROR_DESC(EGL_BAD_ATTRIBUTE),
      _EGL_ERROR_DESC(EGL_BAD_CONTEXT), _EGL_ERROR_DESC(EGL_BAD_CONFIG),      _EGL_ERROR_DESC(EGL_BAD_CURRENT_SURFACE), _EGL_ERROR_DESC(EGL_BAD_DISPLAY),       _EGL_ERROR_DESC(EGL_BAD_SURFACE),
      _EGL_ERROR_DESC(EGL_BAD_MATCH),   _EGL_ERROR_DESC(EGL_BAD_PARAMETER),   _EGL_ERROR_DESC(EGL_BAD_NATIVE_PIXMAP),   _EGL_ERROR_DESC(EGL_BAD_NATIVE_WINDOW), _EGL_ERROR_DESC(EGL_CONTEXT_LOST),
  };

#undef _EGL_ERROR_DESC

  const auto count = sizeof(pairs) / sizeof(EGLNameErrorPair);

  EGLint last_error = eglGetError();

  for (size_t i = 0; i < count; i++) {
    if (last_error == pairs[i].code) {
      FLWAY_ERROR << "EGL Error: " << pairs[i].name << " (" << pairs[i].code << ")" << std::endl;
      return;
    }
  }

  FLWAY_ERROR << "Unknown EGL Error" << std::endl;
}

bool WaylandDisplay::SetupEGL() {

  egl_display_ = eglGetDisplay(display_);
  if (egl_display_ == EGL_NO_DISPLAY) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not access EGL display." << std::endl;
    return false;
  }

  if (eglInitialize(egl_display_, nullptr, nullptr) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not initialize EGL display." << std::endl;
    return false;
  }

  if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not bind the ES API." << std::endl;
    return false;
  }

  EGLConfig egl_config = nullptr;

  // Choose an EGL config to use for the surface and context.
  {
    EGLint attribs[] = {
        // clang-format off
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
      EGL_RED_SIZE,        8,
      EGL_GREEN_SIZE,      8,
      EGL_BLUE_SIZE,       8,
      EGL_ALPHA_SIZE,      8,
      EGL_DEPTH_SIZE,      0,
      EGL_STENCIL_SIZE,    0,
      EGL_NONE,            // termination sentinel
        // clang-format on
    };

    EGLint config_count = 0;

    if (eglChooseConfig(egl_display_, attribs, &egl_config, 1, &config_count) != EGL_TRUE) {
      LogLastEGLError();
      FLWAY_ERROR << "Error when attempting to choose an EGL surface config." << std::endl;
      return false;
    }

    if (config_count == 0 || egl_config == nullptr) {
      LogLastEGLError();
      FLWAY_ERROR << "No matching configs." << std::endl;
      return false;
    }
  }

  // Create an EGL context with the match config.
  {
    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    egl_context_ = eglCreateContext(egl_display_, egl_config, nullptr /* share group */, attribs);

    if (egl_context_ == EGL_NO_CONTEXT) {
      LogLastEGLError();
      FLWAY_ERROR << "Could not create an onscreen context." << std::endl;
      return false;
    }
  }

  if (!compositor_ || !shell_) {
    FLWAY_ERROR << "EGL setup needs missing compositor and shell connection." << std::endl;
    return false;
  }

  surface_ = wl_compositor_create_surface(compositor_);

  if (!surface_) {
    FLWAY_ERROR << "Could not create compositor surface." << std::endl;
    return false;
  }

  shell_surface_ = wl_shell_get_shell_surface(shell_, surface_);

  if (!shell_surface_) {
    FLWAY_ERROR << "Could not shell surface." << std::endl;
    return false;
  }

  wl_shell_surface_add_listener(shell_surface_, &kShellSurfaceListener, this);

  wl_shell_surface_set_title(shell_surface_, "Flutter");

  wl_shell_surface_set_toplevel(shell_surface_);

  window_ = wl_egl_window_create(surface_, screen_width_, screen_height_);

  if (!window_) {
    FLWAY_ERROR << "Could not create EGL window." << std::endl;
    return false;
  }

  // Create an EGL window surface with the matched config.
  {
    const EGLint attribs[] = {EGL_NONE};

    egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config, window_, attribs);

    if (egl_surface_ == EGL_NO_SURFACE) {
      LogLastEGLError();
      FLWAY_ERROR << "EGL surface was null during surface selection." << std::endl;
      return false;
    }
  }

  return true;
}

void WaylandDisplay::AnnounceRegistryInterface(struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
  printf("AnnounceRegistryInterface(registry:%p, name:%2u, interface:%s, version:%u)\n", registry, name, interface, version);

  if (strcmp(interface, "wl_compositor") == 0) {
    compositor_ = static_cast<decltype(compositor_)>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    return;
  }

  if (strcmp(interface, "wl_shell") == 0) {
    shell_ = static_cast<decltype(shell_)>(wl_registry_bind(registry, name, &wl_shell_interface, 1));
    return;
  }

  if (strcmp(interface, "wl_seat") == 0) {
    seat_ = static_cast<decltype(seat_)>(wl_registry_bind(registry, name, &wl_seat_interface, 1));
    wl_seat_add_listener(seat_, &kSeatListener, this);
    return;
  }
}

void WaylandDisplay::UnannounceRegistryInterface(struct wl_registry *wl_registry, uint32_t name) {
}

bool WaylandDisplay::OnApplicationContextMakeCurrent() {
  if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not make the onscreen context current" << std::endl;
    return false;
  }

  return true;
}

bool WaylandDisplay::OnApplicationContextClearCurrent() {
  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not clear the context." << std::endl;
    return false;
  }

  return true;
}

bool WaylandDisplay::OnApplicationPresent() {
  if (eglSwapBuffers(egl_display_, egl_surface_) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not swap the EGL buffer." << std::endl;
    return false;
  }

  return true;
}

uint32_t WaylandDisplay::OnApplicationGetOnscreenFBO() {

  return 0; // FBO0
}

} // namespace flutter
