// Copyright 2018 The Flutter Authors. All rights reserved.
// Copyright 2019 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <fstream>
#include <flutter_embedder.h>
#include "macros.h"

namespace flutter {

std::string GetICUDataPath();

std::string GetExecutableName();

std::string GetExecutableDirectory();

bool FileExistsAtPath(const std::string &path);

bool FlutterAssetBundleIsValid(const std::string &bundle_path);

std::string FlutterGetAppAotElfName();

bool FlutterSendMessage(FlutterEngine engine, const char *channel, const uint8_t *message, const size_t message_size);

template <typename T> T getEnv(const char *variable, T default_value);
} // namespace flutter
