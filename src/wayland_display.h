// Copyright 2018 The Flutter Authors. All rights reserved.
// Copyright 2019 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <atomic>
#include <unistd.h>
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <gdk/gdk.h>
#include <xkbcommon/xkbcommon.h>
#include <flutter_embedder.h>

#include <memory>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <wayland-presentation-time-client-protocol.h>
#include <wayland-xwayland-keyboard-grab-client-protocol.h>

#include "macros.h"

namespace flutter {

class WaylandDisplay {
public:
  WaylandDisplay(size_t width, size_t height, const std::string &bundle_path, const std::vector<std::string> &args);

  ~WaylandDisplay();

  bool IsValid() const;

  bool Run();

private:
  static const wl_registry_listener kRegistryListener;
  static const wl_shell_surface_listener kShellSurfaceListener;
  static const wl_seat_listener kSeatListener;
  static const wl_output_listener kOutputListener;
  static const wl_pointer_listener kPointerListener;
  static const wl_callback_listener kFrameListener;
  static const wp_presentation_listener kPresentationListener;
  static const wp_presentation_feedback_listener kPresentationFeedbackListener;

  double surface_x = 0;
  double surface_y = 0;

  struct zwp_xwayland_keyboard_grab_v1 *xwayland_keyboard_grab = nullptr;

  static const wl_keyboard_listener kKeyboardListener;
  wl_keyboard_keymap_format keymap_format = WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP;
  struct xkb_state *xkb_state             = nullptr;
  struct xkb_keymap *keymap               = nullptr;
  struct xkb_context *xkb_context         = nullptr;
  GdkModifierType key_modifiers           = static_cast<GdkModifierType>(0);

  bool valid_ = false;
  int screen_width_;
  int screen_height_;
  int physical_width_                                      = 0;
  int physical_height_                                     = 0;
  bool window_metrix_skipped_                              = false;
  wl_display *display_                                     = nullptr;
  wl_registry *registry_                                   = nullptr;
  wl_compositor *compositor_                               = nullptr;
  wl_shell *shell_                                         = nullptr;
  wl_seat *seat_                                           = nullptr;
  wl_output *output_                                       = nullptr;
  wp_presentation *presentation_                           = nullptr;
  zwp_xwayland_keyboard_grab_manager_v1 *kbd_grab_manager_ = nullptr;
  wl_shell_surface *shell_surface_                         = nullptr;
  wl_surface *surface_                                     = nullptr;
  wl_egl_window *window_                                   = nullptr;
  EGLDisplay egl_display_                                  = EGL_NO_DISPLAY;
  EGLSurface egl_surface_                                  = nullptr;
  EGLContext egl_context_                                  = EGL_NO_CONTEXT;

  EGLSurface resource_egl_surface_ = nullptr;
  EGLContext resource_egl_context_ = EGL_NO_CONTEXT;

  FlutterEngine engine_ = nullptr;

  bool SetupEGL();

  bool SetupEngine(const std::string &bundle_path, const std::vector<std::string> &command_line_args);

  bool StopRunning();

  // vsync related {
  uint32_t presentation_clk_id_     = UINT32_MAX;
  std::atomic<intptr_t> baton_      = 0;
  std::atomic<uint64_t> last_frame_ = 0;
  uint64_t vblank_time_ns_          = 1000000000000 / 60000;
  ssize_t vSyncHandler();
  enum { SOCKET_WRITER = 0, SOCKET_READER };
  int sv_[2] = {-1, -1}; // 0-index is for sending, 1-index is for reading
  ssize_t sendNotifyData();
  ssize_t readNotifyData();
  // }

  FLWAY_DISALLOW_COPY_AND_ASSIGN(WaylandDisplay)
};

} // namespace flutter
