// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WL_EGL_PLATFORM
#define WL_EGL_PLATFORM 1
#endif

#include "wayland_display.h"

#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-names.h>

#include <cstring>

#include "logger.h"

namespace flutter {

#define DISPLAY reinterpret_cast<WaylandDisplay*>(data)

#ifdef USE_XDG_SHELL

void WaylandDisplay::XdgToplevelConfigureHandler(
    void* data,
    struct xdg_toplevel* xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array* states) {}

void WaylandDisplay::XdgToplevelCloseHandler(
    void* data,
    struct xdg_toplevel* xdg_toplevel) {}

const xdg_toplevel_listener WaylandDisplay::kXdgToplevelListener = {
    .configure = XdgToplevelConfigureHandler,
    .close = XdgToplevelCloseHandler};

void WaylandDisplay::XdgSurfaceConfigureHandler(void* data,
                                                struct xdg_surface* xdg_surface,
                                                uint32_t serial) {
  xdg_surface_ack_configure(xdg_surface, serial);
}

const xdg_surface_listener WaylandDisplay::kXdgSurfaceListener = {
    .configure = XdgSurfaceConfigureHandler};

void WaylandDisplay::XdgWmBasePingHandler(void* data,
                                          struct xdg_wm_base* xdg_wm_base,
                                          uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial);
}

const xdg_wm_base_listener WaylandDisplay::kXdgWmBaseListener = {
    .ping = XdgWmBasePingHandler};

#endif

void WaylandDisplay::KeyboardHandleKeymap(void* data,
                                          struct wl_keyboard* keyboard,
                                          uint32_t format,
                                          int fd,
                                          uint32_t size) {
  SPDLOG_DEBUG("format = {} fd = {} size = {}", format, fd, size);

  keymap_format_ = static_cast<wl_keyboard_keymap_format>(format);
  char* const keymap_string = reinterpret_cast<char* const>(
      mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
  xkb_keymap_unref(xkb_keymap_);
  xkb_keymap_ = xkb_keymap_new_from_string(xkb_context_, keymap_string,
                                           XKB_KEYMAP_FORMAT_TEXT_V1,
                                           XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(keymap_string, size);
  close(fd);
  xkb_state_unref(xkb_state_);
  xkb_state_ = xkb_state_new(xkb_keymap_);
}

void WaylandDisplay::KeyboardHandleEnter(void* data,
                                         struct wl_keyboard* keyboard,
                                         uint32_t serial,
                                         struct wl_surface* surface,
                                         struct wl_array* keys) {
  SPDLOG_DEBUG("Keyboard gained focus");
}

void WaylandDisplay::KeyboardHandleLeave(void* data,
                                         struct wl_keyboard* keyboard,
                                         uint32_t serial,
                                         struct wl_surface* surface) {
  SPDLOG_DEBUG("Keyboard lost focus");
  if (key_repeat_timer_handle_) {
    uv_timer_stop(key_repeat_timer_handle_);
  }
}

SimpleKeyboardModifiers WaylandDisplay::KeyboardGetModifiers() {
  return SimpleKeyboardModifiers(
      xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_SHIFT,
                                   XKB_STATE_MODS_EFFECTIVE) == 1,
      xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_CTRL,
                                   XKB_STATE_MODS_EFFECTIVE) == 1,
      xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_ALT,
                                   XKB_STATE_MODS_EFFECTIVE) == 1,
      xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_LOGO,
                                   XKB_STATE_MODS_EFFECTIVE) == 1,
      xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_CAPS,
                                   XKB_STATE_MODS_EFFECTIVE) == 1,
      xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_NUM,
                                   XKB_STATE_MODS_EFFECTIVE) == 1);
}

void WaylandDisplay::KeyboardHandleKey(void* data,
                                       struct wl_keyboard* keyboard,
                                       uint32_t serial,
                                       uint32_t time,
                                       uint32_t key,
                                       uint32_t state) {
  SPDLOG_DEBUG("key = {} state = {} keymap_format_ = {}", key, state,
               keymap_format_);

  uint32_t evdev_keycode = key;
  uint32_t xkb_keycode;
  uint32_t utf32;
  SimpleKeyboardModifiers mods;

  switch (keymap_format_) {
    case WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP: {
      xkb_keycode = 0;
      utf32 = 0;
      break;
    }
    case WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1: {
      xkb_keycode = evdev_keycode + 8;  // See wl_keyboard_keymap_format docs

      utf32 = xkb_state_key_get_utf32(xkb_state_, xkb_keycode);

      char name[64];
      xkb_keysym_t keysym = xkb_state_key_get_one_sym(xkb_state_, xkb_keycode);
      xkb_keysym_get_name(keysym, name, sizeof(name));

      mods = KeyboardGetModifiers();

      SPDLOG_DEBUG("keysym = {} utf32 = U+{:X} mods = {}", keysym, utf32,
                   mods.ToString());
      SPDLOG_DEBUG("The key '{}' was {}", name,
                   WL_KEYBOARD_KEY_STATE_PRESSED ? "pressed" : "released");

      if (key_repeat_timer_handle_) {
        if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
            xkb_keymap_key_repeats(xkb_keymap_, xkb_keycode) == 1) {
          last_evdev_keycode_ = evdev_keycode;
          last_xkb_keycode_ = xkb_keycode;
          last_utf32_ = utf32;
          uv_timer_start(key_repeat_timer_handle_,
                         cify([self = this](uv_timer_t* handle) {
                           self->KeyboardHandleRepeat(handle);
                         }),
                         repeat_delay_, 1000 / repeat_rate_);
        } else if (state == WL_KEYBOARD_KEY_STATE_RELEASED &&
                   xkb_keycode == last_xkb_keycode_) {
          uv_timer_stop(key_repeat_timer_handle_);
        }
      }
      break;
    }

    default: {
      SPDLOG_ERROR("Unexpected keymap format: {}", keymap_format_);
      return;
    }
  }

  for (auto listener = kEventListeners.begin();
       listener != kEventListeners.end(); ++listener) {
    (*listener)->OnKeyboardKey(evdev_keycode, xkb_keycode, utf32, state, mods);
  }
}

void WaylandDisplay::KeyboardHandleRepeat(uv_timer_t* handle) {
  if (handle == key_repeat_timer_handle_) {
    switch (keymap_format_) {
      case WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP: {
        return;
      }

      case WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1: {
        uint32_t evdev_keycode = last_evdev_keycode_;
        uint32_t xkb_keycode = last_xkb_keycode_;
        uint32_t utf32 = last_utf32_;
        SimpleKeyboardModifiers mods = KeyboardGetModifiers();

        for (auto listener = kEventListeners.begin();
             listener != kEventListeners.end(); ++listener) {
          (*listener)->OnKeyboardKey(evdev_keycode, xkb_keycode, utf32, false,
                                     mods);
          (*listener)->OnKeyboardKey(evdev_keycode, xkb_keycode, utf32, true,
                                     mods);
        }
        break;
      }

      default: {
        return;
      }
    }
  }
}

void WaylandDisplay::KeyboardHandleModifiers(void* data,
                                             struct wl_keyboard* keyboard,
                                             uint32_t serial,
                                             uint32_t mods_depressed,
                                             uint32_t mods_latched,
                                             uint32_t mods_locked,
                                             uint32_t group) {
  SPDLOG_DEBUG("Modifiers depressed = {} latched = {} locked = {} group = {}",
               mods_depressed, mods_latched, mods_locked, group);

  xkb_state_update_mask(xkb_state_, mods_depressed, mods_latched, mods_locked,
                        0, 0, group);
}

void WaylandDisplay::KeyboardHandleRepeatInfo(void* data,
                                              struct wl_keyboard* wl_keyboard,
                                              int32_t rate,
                                              int32_t delay) {
  SPDLOG_DEBUG("rate = {} delay = {}", rate, delay);
  repeat_rate_ = rate;
  repeat_delay_ = delay;
}

void WaylandDisplay::SeatHandleCapabilities(void* data,
                                            struct wl_seat* seat,
                                            uint32_t caps) {
  if (caps & WL_SEAT_CAPABILITY_POINTER) {
    SPDLOG_INFO("Display has a pointer");
  }

  if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
    SPDLOG_INFO("Display has a keyboard");
    keyboard_ = wl_seat_get_keyboard(seat);
    SPDLOG_DEBUG("keyboard_ = {}", fmt::ptr(keyboard_));
    wl_keyboard_add_listener(keyboard_, &kKeyboardListener, NULL);
  }

  if (caps & WL_SEAT_CAPABILITY_TOUCH) {
    SPDLOG_INFO("Display has a touch screen");
  }
}

void WaylandDisplay::SeatHandleName(void* data,
                                    struct wl_seat* wl_seat,
                                    const char* name) {
  SPDLOG_DEBUG("name = {}", name);
}

const wl_registry_listener WaylandDisplay::kRegistryListener = {
    .global = [](void* data,
                 struct wl_registry* wl_registry,
                 uint32_t name,
                 const char* interface,
                 uint32_t version) -> void {
      DISPLAY->AnnounceRegistryInterface(wl_registry, name, interface, version);
    },

    .global_remove =
        [](void* data, struct wl_registry* wl_registry, uint32_t name) -> void {
      DISPLAY->UnannounceRegistryInterface(wl_registry, name);
    },
};

const wl_shell_surface_listener WaylandDisplay::kShellSurfaceListener = {
    .ping = [](void* data,
               struct wl_shell_surface* wl_shell_surface,
               uint32_t serial) -> void {
      wl_shell_surface_pong(DISPLAY->shell_surface_, serial);
    },

    .configure = [](void* data,
                    struct wl_shell_surface* wl_shell_surface,
                    uint32_t edges,
                    int32_t width,
                    int32_t height) -> void {
      SPDLOG_ERROR("Unhandled resize.");
    },

    .popup_done = [](void* data,
                     struct wl_shell_surface* wl_shell_surface) -> void {
      // Nothing to do.
    },
};

WaylandDisplay::WaylandDisplay(size_t width, size_t height)
    : screen_width_(width),
      screen_height_(height),
      xkb_context_(xkb_context_new(XKB_CONTEXT_NO_FLAGS)) {
  if (screen_width_ == 0 || screen_height_ == 0) {
    SPDLOG_ERROR("Invalid screen dimensions.");
    return;
  }

  display_ = wl_display_connect(nullptr);

  if (!display_) {
    SPDLOG_ERROR("Could not connect to the wayland display.");
    return;
  }

  registry_ = wl_display_get_registry(display_);
  if (!registry_) {
    SPDLOG_ERROR("Could not get the wayland registry.");
    return;
  }

  wl_registry_add_listener(registry_, &kRegistryListener, this);

  wl_display_roundtrip(display_);

  if (!SetupEGL()) {
    SPDLOG_ERROR("Could not setup EGL.");
    return;
  }

  valid_ = true;
}

WaylandDisplay::~WaylandDisplay() {
  xkb_keymap_unref(xkb_keymap_);
  xkb_keymap_ = nullptr;

  xkb_state_unref(xkb_state_);
  xkb_state_ = nullptr;

  xkb_context_unref(xkb_context_);
  xkb_context_ = nullptr;

  if (keyboard_) {
    wl_keyboard_destroy(keyboard_);
    keyboard_ = nullptr;
  }

  if (seat_) {
    wl_seat_destroy(seat_);
    seat_ = nullptr;
  }

  if (shell_surface_) {
    wl_shell_surface_destroy(shell_surface_);
    shell_surface_ = nullptr;
  }

  if (shell_) {
    wl_shell_destroy(shell_);
    shell_ = nullptr;
  }

#ifdef USE_XDG_SHELL
  if (xdg_wm_base_) {
    xdg_wm_base_destroy(xdg_wm_base_);
    shell_ = nullptr;
  }
#endif

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

void WaylandDisplay::ProcessWaylandEvents(uv_poll_t* handle,
                                          int status,
                                          int events) {
  SPDLOG_TRACE("status = {} events = {}", status, events);
  wl_display_dispatch(display_);
}

bool WaylandDisplay::Run() {
  SPDLOG_DEBUG("valid_ = {}", valid_);

  if (!valid_) {
    SPDLOG_ERROR("Could not run an invalid display.");
    return false;
  }

  uv_loop_t* loop = new uv_loop_t;
  uv_loop_init(loop);

  uv_poll_t* wl_events_poll_handle = new uv_poll_t;
  uv_poll_init(loop, wl_events_poll_handle, wl_display_get_fd(display_));

  key_repeat_timer_handle_ = new uv_timer_t;
  uv_timer_init(loop, key_repeat_timer_handle_);

  uv_poll_start(wl_events_poll_handle, UV_READABLE,
                cify([self = this](uv_poll_t* handle, int status, int events) {
                  self->ProcessWaylandEvents(handle, status, events);
                }));

  uv_run(loop, UV_RUN_DEFAULT);

  uv_timer_stop(key_repeat_timer_handle_);
  delete key_repeat_timer_handle_;

  uv_poll_stop(wl_events_poll_handle);
  delete wl_events_poll_handle;

  uv_loop_close(loop);
  delete loop;

  return true;
}

static void LogLastEGLError() {
  struct EGLNameErrorPair {
    const char* name;
    EGLint code;
  };

#define _EGL_ERROR_DESC(a) \
  { #a, a }

  const EGLNameErrorPair pairs[] = {
      _EGL_ERROR_DESC(EGL_SUCCESS),
      _EGL_ERROR_DESC(EGL_NOT_INITIALIZED),
      _EGL_ERROR_DESC(EGL_BAD_ACCESS),
      _EGL_ERROR_DESC(EGL_BAD_ALLOC),
      _EGL_ERROR_DESC(EGL_BAD_ATTRIBUTE),
      _EGL_ERROR_DESC(EGL_BAD_CONTEXT),
      _EGL_ERROR_DESC(EGL_BAD_CONFIG),
      _EGL_ERROR_DESC(EGL_BAD_CURRENT_SURFACE),
      _EGL_ERROR_DESC(EGL_BAD_DISPLAY),
      _EGL_ERROR_DESC(EGL_BAD_SURFACE),
      _EGL_ERROR_DESC(EGL_BAD_MATCH),
      _EGL_ERROR_DESC(EGL_BAD_PARAMETER),
      _EGL_ERROR_DESC(EGL_BAD_NATIVE_PIXMAP),
      _EGL_ERROR_DESC(EGL_BAD_NATIVE_WINDOW),
      _EGL_ERROR_DESC(EGL_CONTEXT_LOST),
  };

#undef _EGL_ERROR_DESC

  const auto count = sizeof(pairs) / sizeof(EGLNameErrorPair);

  EGLint last_error = eglGetError();

  for (size_t i = 0; i < count; i++) {
    if (last_error == pairs[i].code) {
      SPDLOG_ERROR("EGL Error: {} ({})", pairs[i].name, pairs[i].code);
      return;
    }
  }

  SPDLOG_ERROR("Unknown EGL Error");
}

bool WaylandDisplay::SetupEGL() {
  SPDLOG_DEBUG("compositor_ = {} shell_ = {}", fmt::ptr(compositor_),
               fmt::ptr(shell_));
#ifdef USE_XDG_SHELL
  SPDLOG_DEBUG("xdg_wm_base_ = {}", fmt::ptr(xdg_wm_base_));
#endif
  if (!compositor_) {
    SPDLOG_ERROR("EGL setup needs missing compositor connection.");
    return false;
  }

  egl_display_ = eglGetDisplay(display_);
  if (egl_display_ == EGL_NO_DISPLAY) {
    LogLastEGLError();
    SPDLOG_ERROR("Could not access EGL display.");
    return false;
  }

  if (eglInitialize(egl_display_, nullptr, nullptr) != EGL_TRUE) {
    LogLastEGLError();
    SPDLOG_ERROR("Could not initialize EGL display.");
    return false;
  }

  if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
    LogLastEGLError();
    SPDLOG_ERROR("Could not bind the ES API.");
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

    if (eglChooseConfig(egl_display_, attribs, &egl_config, 1, &config_count) !=
        EGL_TRUE) {
      LogLastEGLError();
      SPDLOG_ERROR("Error when attempting to choose an EGL surface config.");
      return false;
    }

    if (config_count == 0 || egl_config == nullptr) {
      LogLastEGLError();
      SPDLOG_ERROR("No matching configs.");
      return false;
    }
  }

  surface_ = wl_compositor_create_surface(compositor_);

  if (!surface_) {
    SPDLOG_ERROR("Could not create compositor surface.");
    return false;
  }

#ifdef USE_XDG_SHELL
  xdg_wm_base_add_listener(xdg_wm_base_, &kXdgWmBaseListener, NULL);
  struct xdg_surface* xdg_surface =
      xdg_wm_base_get_xdg_surface(xdg_wm_base_, surface_);
  xdg_surface_add_listener(xdg_surface, &kXdgSurfaceListener, NULL);

  struct xdg_toplevel* xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
  xdg_toplevel_add_listener(xdg_toplevel, &kXdgToplevelListener, NULL);
  xdg_toplevel_set_title(xdg_toplevel, "Flutter");

  // signal that the surface is ready to be configured
  wl_surface_commit(surface_);

  // wait for the surface to be configured
  wl_display_roundtrip(display_);
#else
  if (shell_) {
    shell_surface_ = wl_shell_get_shell_surface(shell_, surface_);

    if (!shell_surface_) {
      SPDLOG_ERROR("Could not shell surface.");
      return false;
    }

    wl_shell_surface_add_listener(shell_surface_, &kShellSurfaceListener, this);
    wl_shell_surface_set_title(shell_surface_, "Flutter");
    wl_shell_surface_set_toplevel(shell_surface_);
  } else {
    SPDLOG_ERROR(
        "Not using xdg-shell protocol extension and wl_shell is not "
        "available");
    return false;
  }
#endif

  SPDLOG_DEBUG("create egl window = {} size = {}x{}", fmt::ptr(surface_),
               screen_width_, screen_height_);

  window_ = wl_egl_window_create(surface_, screen_width_, screen_height_);

  if (!window_) {
    SPDLOG_ERROR("Could not create EGL window.");
    return false;
  }

  // Create an EGL window surface with the matched config.
  {
    const EGLint attribs[] = {EGL_NONE};

    egl_surface_ =
        eglCreateWindowSurface(egl_display_, egl_config, window_, attribs);

    if (surface_ == EGL_NO_SURFACE) {
      LogLastEGLError();
      SPDLOG_ERROR("EGL surface was null during surface selection.");
      return false;
    }
  }

  // Create an EGL context with the match config.
  {
    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    egl_context_ = eglCreateContext(egl_display_, egl_config,
                                    nullptr /* share group */, attribs);

    if (egl_context_ == EGL_NO_CONTEXT) {
      LogLastEGLError();
      SPDLOG_ERROR("Could not create an onscreen context.");
      return false;
    }
  }

  return true;
}

void WaylandDisplay::AnnounceRegistryInterface(struct wl_registry* wl_registry,
                                               uint32_t name,
                                               const char* interface_name,
                                               uint32_t version) {
  SPDLOG_DEBUG("name = {} interface_name = {} version = {}", name, interface_name, version);

  if (strcmp(interface_name, "wl_compositor") == 0) {
    compositor_ = static_cast<decltype(compositor_)>(
        wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4));
    return;
  }

  if (strcmp(interface_name, "wl_shell") == 0) {
    shell_ = static_cast<decltype(shell_)>(
        wl_registry_bind(wl_registry, name, &wl_shell_interface, 1));
    return;
  }

#ifdef USE_XDG_SHELL
  if (strcmp(interface_name, "xdg_wm_base") == 0) {
    xdg_wm_base_ = static_cast<decltype(xdg_wm_base_)>(
        wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1));
    return;
  }
#endif

  if (strcmp(interface_name, "wl_seat") == 0) {
    seat_ = static_cast<decltype(seat_)>(
        wl_registry_bind(wl_registry, name, &wl_seat_interface, 4));
    wl_seat_add_listener(seat_, &kSeatListener, NULL);
    return;
  }
}

void WaylandDisplay::UnannounceRegistryInterface(
    struct wl_registry* wl_registry,
    uint32_t name) {}

// |flutter::FlutterApplication::RenderDelegate|
bool WaylandDisplay::OnApplicationContextMakeCurrent() {
  if (!valid_) {
    SPDLOG_ERROR("Invalid display.");
    return false;
  }

  if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) !=
      EGL_TRUE) {
    LogLastEGLError();
    SPDLOG_ERROR("Could not make the onscreen context current");
    return false;
  }

  return true;
}

// |flutter::FlutterApplication::RenderDelegate|
bool WaylandDisplay::OnApplicationContextClearCurrent() {
  if (!valid_) {
    SPDLOG_ERROR("Invalid display.");
    return false;
  }

  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT) != EGL_TRUE) {
    LogLastEGLError();
    SPDLOG_ERROR("Could not clear the context.");
    return false;
  }

  return true;
}

// |flutter::FlutterApplication::RenderDelegate|
bool WaylandDisplay::OnApplicationPresent() {
  if (!valid_) {
    SPDLOG_ERROR("Invalid display.");
    return false;
  }

  if (eglSwapBuffers(egl_display_, egl_surface_) != EGL_TRUE) {
    LogLastEGLError();
    SPDLOG_ERROR("Could not swap the EGL buffer.");
    return false;
  }

  return true;
}

// |flutter::FlutterApplication::RenderDelegate|
uint32_t WaylandDisplay::OnApplicationGetOnscreenFBO() {
  if (!valid_) {
    SPDLOG_ERROR("Invalid display.");
    return 999;
  }

  return 0;  // FBO0
}

}  // namespace flutter
