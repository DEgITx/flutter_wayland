// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <flutter_embedder.h>
#include "macros.h"

namespace flutter {

std::string GetExecutableName();

std::string GetExecutableDirectory();

bool FileExistsAtPath(const std::string &path);

bool FlutterAssetBundleIsValid(const std::string &bundle_path);

bool FlutterSendMessage(FlutterEngine engine, const char *channel, const uint8_t *message, const size_t message_size);

int toGLFWKeyCode(const uint32_t key);

} // namespace flutter
