// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <xkbcommon/xkbcommon.h>

#ifdef USE_XDG_SHELL
#include "wayland-xdg-shell-client-protocol.h"
#endif

#ifdef USE_COMPOSITOR_LAYOUT
#include <weston-nsc-backend/settop-shell-client-protocol.h>
#endif

#include <memory>
#include <set>
#include <string>

#include <uv.h>

#include "cify.h"
#include "display_event_emitter.h"
#include "logger.h"
#include "macros.h"
#include "render_delegate.h"

namespace flutter {

class WaylandDisplay : public RenderDelegate, public DisplayEventEmitter {
 public:
  WaylandDisplay(size_t width,
                 size_t height,
                 size_t widthAlign,
                 size_t heightAlign);

  ~WaylandDisplay();

  bool IsValid() const;

  bool Run();

 private:
  static const wl_registry_listener kRegistryListener;
  static const wl_shell_surface_listener kShellSurfaceListener;
#ifdef USE_XDG_SHELL
  static const struct xdg_wm_base_listener kXdgWmBaseListener;
  static const struct xdg_surface_listener kXdgSurfaceListener;
  static const struct xdg_toplevel_listener kXdgToplevelListener;
#endif
  const wl_seat_listener kSeatListener = {
      .capabilities = cify([self = this](void* data,
                                         struct wl_seat* wl_seat,
                                         uint32_t capabilities) {
        self->SeatHandleCapabilities(data, wl_seat, capabilities);
      }),
      .name = cify(
          [self = this](void* data, struct wl_seat* wl_seat, const char* name) {
            self->SeatHandleName(data, wl_seat, name);
          }),
  };
  const wl_keyboard_listener kKeyboardListener = {
      .keymap = cify([self = this](void* data,
                                   struct wl_keyboard* wl_keyboard,
                                   uint32_t format,
                                   int32_t fd,
                                   uint32_t size) {
        self->KeyboardHandleKeymap(data, wl_keyboard, format, fd, size);
      }),
      .enter = cify([self = this](void* data,
                                  struct wl_keyboard* wl_keyboard,
                                  uint32_t serial,
                                  struct wl_surface* surface,
                                  struct wl_array* keys) {
        self->KeyboardHandleEnter(data, wl_keyboard, serial, surface, keys);
      }),
      .leave = cify([self = this](void* data,
                                  struct wl_keyboard* wl_keyboard,
                                  uint32_t serial,
                                  struct wl_surface* surface) {
        self->KeyboardHandleLeave(data, wl_keyboard, serial, surface);
      }),
      .key = cify([self = this](void* data,
                                struct wl_keyboard* wl_keyboard,
                                uint32_t serial,
                                uint32_t time,
                                uint32_t key,
                                uint32_t state) {
        self->KeyboardHandleKey(data, wl_keyboard, serial, time, key, state);
      }),
      .modifiers = cify([self = this](void* data,
                                      struct wl_keyboard* wl_keyboard,
                                      uint32_t serial,
                                      uint32_t mods_depressed,
                                      uint32_t mods_latched,
                                      uint32_t mods_locked,
                                      uint32_t group) {
        self->KeyboardHandleModifiers(data, wl_keyboard, serial, mods_depressed,
                                      mods_latched, mods_locked, group);
      }),
      .repeat_info = cify([self = this](void* data,
                                        struct wl_keyboard* wl_keyboard,
                                        int32_t rate,
                                        int32_t delay) {
        self->KeyboardHandleRepeatInfo(data, wl_keyboard, rate, delay);
      }),
  };
#ifdef USE_COMPOSITOR_LAYOUT
  const compositor_layout_listener kCompositorLayoutListener = {
      .client_created =
          cify([self = this](void* data,
                             struct compositor_layout* compositor_layout,
                             uint32_t pid,
                             uint32_t surface) {
            SPDLOG_DEBUG(
                "kCompositorLayoutListener.client_created: pid:surface = {}:{}",
                pid, surface);
            compositor_layout_set_focus(compositor_layout, surface);
          }),
      .client_destroyed = cify([self = this](
                                   void* data,
                                   struct compositor_layout* compositor_layout,
                                   uint32_t surface) {
        SPDLOG_DEBUG("kCompositorLayoutListener.client_destroyed: surface = {}",
                     surface);
      }),
      .key_event =
          cify([self = this](void* data,
                             struct compositor_layout* compositor_layout,
                             uint32_t keyCode) {
            SPDLOG_DEBUG("kCompositorLayoutListener.key_event: keyCode = {}",
                         keyCode);
          }),
      .touch_event =
          cify([self = this](void* data,
                             struct compositor_layout* compositor_layout) {
            SPDLOG_DEBUG("kCompositorLayoutListener.touch_event: no params");
          }),
      .size_event =
          cify([self = this](void* data,
                             struct compositor_layout* compositor_layout,
                             uint32_t surface,
                             uint32_t width,
                             uint32_t height) {
            SPDLOG_DEBUG(
                "kCompositorLayoutListener.size_event: surface = {} width = {} "
                "height = {}",
                surface, width, height);
          }),
  };
#endif
  bool valid_ = false;
  const int screen_width_;
  const int screen_height_;
  const int screen_width_align;
  const int screen_height_align;
  wl_display* display_ = nullptr;
  wl_registry* registry_ = nullptr;
  wl_compositor* compositor_ = nullptr;
  wl_shell* shell_ = nullptr;
#ifdef USE_XDG_SHELL
  xdg_wm_base* xdg_wm_base_ = nullptr;
#endif
  wl_seat* seat_ = nullptr;
  wl_keyboard* keyboard_ = nullptr;
  wl_shell_surface* shell_surface_ = nullptr;
  wl_surface* surface_ = nullptr;
  wl_egl_window* window_ = nullptr;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLSurface egl_surface_ = nullptr;
  EGLContext egl_context_ = EGL_NO_CONTEXT;

  bool xkb_input_source_enabled_ = false;
  EGLSurface resource_egl_surface_ = nullptr;
  EGLContext resource_egl_context_ = EGL_NO_CONTEXT;

  wl_keyboard_keymap_format keymap_format_ =
      WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP;
  xkb_state* xkb_state_ = nullptr;
  xkb_keymap* xkb_keymap_ = nullptr;
  xkb_context* xkb_context_ = nullptr;

  uv_loop_t* loop_ = nullptr;
  uv_async_t* signal_event_async_ = nullptr;

  uint32_t last_evdev_keycode_ = 0;
  uint32_t last_xkb_keycode_ = 0;
  uint32_t last_utf32_ = 0;
  uint64_t repeat_rate_ = 40;    // characters per second
  uint64_t repeat_delay_ = 400;  // in milliseconds
  uv_timer_t* key_repeat_timer_handle_ = nullptr;

#ifdef USE_COMPOSITOR_LAYOUT
  compositor_layout* compositor_layout_ = nullptr;
#endif

#ifdef USE_XDG_SHELL
  static void XdgWmBasePingHandler(void* data,
                                   struct xdg_wm_base* xdg_wm_base,
                                   uint32_t serial);
  static void XdgSurfaceConfigureHandler(void* data,
                                         struct xdg_surface* xdg_surface,
                                         uint32_t serial);
  static void XdgToplevelConfigureHandler(void* data,
                                          struct xdg_toplevel* xdg_toplevel,
                                          int32_t width,
                                          int32_t height,
                                          struct wl_array* states);
  static void XdgToplevelCloseHandler(void* data,
                                      struct xdg_toplevel* xdg_toplevel);
#endif

  void SignalHandler(int signum);

  void AsyncSignalHandler(uv_async_t* handle);

  void SeatHandleCapabilities(void* data, struct wl_seat* seat, uint32_t caps);

  void SeatHandleName(void* data, struct wl_seat* wl_seat, const char* name);

  void KeyboardHandleKeymap(void* data,
                            struct wl_keyboard* keyboard,
                            uint32_t format,
                            int fd,
                            uint32_t size);

  void KeyboardHandleEnter(void* data,
                           struct wl_keyboard* keyboard,
                           uint32_t serial,
                           struct wl_surface* surface,
                           struct wl_array* keys);

  void KeyboardHandleLeave(void* data,
                           struct wl_keyboard* keyboard,
                           uint32_t serial,
                           struct wl_surface* surface);

  void KeyboardHandleKey(void* data,
                         struct wl_keyboard* keyboard,
                         uint32_t serial,
                         uint32_t time,
                         uint32_t key,
                         uint32_t state);

  void KeyboardHandleModifiers(void* data,
                               struct wl_keyboard* keyboard,
                               uint32_t serial,
                               uint32_t mods_depressed,
                               uint32_t mods_latched,
                               uint32_t mods_locked,
                               uint32_t group);

  void KeyboardHandleRepeatInfo(void* data,
                                struct wl_keyboard* wl_keyboard,
                                int32_t rate,
                                int32_t delay);

  void KeyboardHandleRepeat(uv_timer_t* handle);

  SimpleKeyboardModifiers KeyboardGetModifiers();

  bool SetupEGL();

  void AnnounceRegistryInterface(struct wl_registry* wl_registry,
                                 uint32_t name,
                                 const char* interface,
                                 uint32_t version);

  void UnannounceRegistryInterface(struct wl_registry* wl_registry,
                                   uint32_t name);

  void ProcessWaylandEvents(uv_poll_t* handle, int status, int events);

  bool StopRunning();

  // |flutter::RenderDelegate|
  bool OnApplicationContextMakeCurrent() override;

  // |flutter::RenderDelegate|
  bool OnApplicationContextClearCurrent() override;

  // |flutter::RenderDelegate|
  bool OnApplicationPresent() override;

  // |flutter::RenderDelegate|
  uint32_t OnApplicationGetOnscreenFBO() override;

  bool OnApplicationMakeResourceCurrent() override;

  FLWAY_DISALLOW_COPY_AND_ASSIGN(WaylandDisplay);
};

}  // namespace flutter
