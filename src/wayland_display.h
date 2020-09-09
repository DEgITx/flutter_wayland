// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#ifdef USE_XDG_SHELL
  #include "wayland-xdg-shell-client-protocol.h"
#endif

#include <memory>
#include <string>

#include "flutter_application.h"
#include "macros.h"

namespace flutter {

class WaylandDisplay : public FlutterApplication::RenderDelegate {
 public:
  WaylandDisplay(size_t width, size_t height);

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
  bool valid_ = false;
  const int screen_width_;
  const int screen_height_;
  wl_display* display_ = nullptr;
  wl_registry* registry_ = nullptr;
  wl_compositor* compositor_ = nullptr;
  wl_shell* shell_ = nullptr;
#ifdef USE_XDG_SHELL
  xdg_wm_base* xdg_wm_base_ = nullptr;
#endif
  wl_shell_surface* shell_surface_ = nullptr;
  wl_surface* surface_ = nullptr;
  wl_egl_window* window_ = nullptr;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLSurface egl_surface_ = nullptr;
  EGLContext egl_context_ = EGL_NO_CONTEXT;

#ifdef USE_XDG_SHELL
  static void XdgWmBasePingHandler(void *data,
                                  struct xdg_wm_base *xdg_wm_base,
                                  uint32_t serial);
  static void XdgSurfaceConfigureHandler(void *data,
                                        struct xdg_surface *xdg_surface,
                                        uint32_t serial);
  static void XdgToplevelConfigureHandler(void *data,
                                          struct xdg_toplevel *xdg_toplevel,
                                          int32_t width,
                                          int32_t height,
                                          struct wl_array *states);
  static void XdgToplevelCloseHandler(void *data,
                                      struct xdg_toplevel *xdg_toplevel);
#endif

  bool SetupEGL();

  void AnnounceRegistryInterface(struct wl_registry* wl_registry,
                                 uint32_t name,
                                 const char* interface,
                                 uint32_t version);

  void UnannounceRegistryInterface(struct wl_registry* wl_registry,
                                   uint32_t name);

  bool StopRunning();

  // |flutter::FlutterApplication::RenderDelegate|
  bool OnApplicationContextMakeCurrent() override;

  // |flutter::FlutterApplication::RenderDelegate|
  bool OnApplicationContextClearCurrent() override;

  // |flutter::FlutterApplication::RenderDelegate|
  bool OnApplicationPresent() override;

  // |flutter::FlutterApplication::RenderDelegate|
  uint32_t OnApplicationGetOnscreenFBO() override;

  FLWAY_DISALLOW_COPY_AND_ASSIGN(WaylandDisplay);
};

}  // namespace flutter
