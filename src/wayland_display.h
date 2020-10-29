// Copyright 2018 The Flutter Authors. All rights reserved.
// Copyright 2019 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <gdk/gdk.h>
#include <xkbcommon/xkbcommon.h>
#include <flutter_embedder.h>

#include <memory>
#include <string>
#include <sys/types.h>

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
  double surface_x = 0;
  double surface_y = 0;

  static const wl_keyboard_listener kKeyboardListener;
  wl_keyboard_keymap_format keymap_format = WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP;
  struct xkb_state *xkb_state             = nullptr;
  struct xkb_keymap *keymap               = nullptr;
  struct xkb_context *xkb_context         = nullptr;
  GdkModifierType key_modifiers           = static_cast<GdkModifierType>(0);

  bool valid_ = false;
  int screen_width_;
  int screen_height_;
  double pixel_ratio_              = 1.0;
  wl_display *display_             = nullptr;
  wl_registry *registry_           = nullptr;
  wl_compositor *compositor_       = nullptr;
  wl_shell *shell_                 = nullptr;
  wl_seat *seat_                   = nullptr;
  wl_output *output_               = nullptr;
  wl_shell_surface *shell_surface_ = nullptr;
  wl_surface *surface_             = nullptr;
  wl_egl_window *window_           = nullptr;
  EGLDisplay egl_display_          = EGL_NO_DISPLAY;
  EGLSurface egl_surface_          = nullptr;
  EGLContext egl_context_          = EGL_NO_CONTEXT;

  EGLSurface resource_egl_surface_ = nullptr;
  EGLContext resource_egl_context_ = EGL_NO_CONTEXT;

  FlutterEngine engine_ = nullptr;

  bool SetupEGL();

  bool SetupEngine(const std::string &bundle_path, const std::vector<std::string> &command_line_args);

  void AnnounceRegistryInterface(struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version);

  void UnannounceRegistryInterface(struct wl_registry *wl_registry, uint32_t name);

  bool StopRunning();

  bool OnApplicationContextMakeCurrent();

  bool OnApplicationMakeResourceCurrent();

  bool OnApplicationContextClearCurrent();

  bool OnApplicationPresent();

  uint32_t OnApplicationGetOnscreenFBO();

  FLWAY_DISALLOW_COPY_AND_ASSIGN(WaylandDisplay);
};

} // namespace flutter
