// Copyright 2018 The Flutter Authors. All rights reserved.
// Copyright 2019 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <wayland-egl.h>
#include <EGL/egl.h>
#include "egl_utils.h"
#include "utils.h"

namespace flutter {

void LogLastEGLError() {
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

} // namespace flutter
