// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>
#include <unistd.h>
#include <string.h>

#include "utils.h"

namespace flutter {

std::string GetICUDataPath() {
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

static std::string GetExecutablePath() {
  char executable_path[1024] = {0};
  std::stringstream stream;
  stream << "/proc/" << getpid() << "/exe";
  auto path                 = stream.str();
  auto executable_path_size = ::readlink(path.c_str(), executable_path, sizeof(executable_path));
  if (executable_path_size <= 0) {
    return "";
  }
  return std::string{executable_path, static_cast<size_t>(executable_path_size)};
}

std::string GetExecutableName() {
  auto path_string = GetExecutablePath();
  auto found       = path_string.find_last_of('/');
  if (found == std::string::npos) {
    return "";
  }
  return path_string.substr(found + 1);
}

std::string GetExecutableDirectory() {
  auto path_string = GetExecutablePath();
  auto found       = path_string.find_last_of('/');
  if (found == std::string::npos) {
    return "";
  }
  return path_string.substr(0, found + 1);
}

bool FileExistsAtPath(const std::string &path) {
  return ::access(path.c_str(), R_OK) == 0;
}

bool FlutterAssetBundleIsValid(const std::string &bundle_path) {
  if (!FileExistsAtPath(bundle_path)) {
    FLWAY_ERROR << "Bundle directory: '" << bundle_path << "' does not exist." << std::endl;
    return false;
  }

  auto kernel_path = bundle_path + std::string{"/kernel_blob.bin"};
  auto aotelf_path = bundle_path + std::string{"/"} + FlutterGetAppAotElfName();
  auto kernel      = FileExistsAtPath(kernel_path);
  auto aotelf      = FileExistsAtPath(aotelf_path);

  if (!(kernel || aotelf)) {
    FLWAY_ERROR << "Could not found neither " << kernel_path << " nor " << aotelf_path << std::endl;
    return false;
  }

  return true;
}

std::string FlutterGetAppAotElfName() {
  return std::string("../../lib/libapp.so"); // assumes 'flutter build' directory layout
}

bool FlutterSendMessage(FlutterEngine engine, const char *channel, const uint8_t *message, const size_t message_size) {
  FlutterPlatformMessageResponseHandle *response_handle = nullptr;

  FlutterPlatformMessage platform_message = {
      .struct_size     = sizeof(FlutterPlatformMessage),
      .channel         = channel,
      .message         = message,
      .message_size    = message_size,
      .response_handle = response_handle,
  };

  FlutterEngineResult message_result = FlutterEngineSendPlatformMessage(engine, &platform_message);

  if (response_handle != nullptr) {
    FlutterPlatformMessageReleaseResponseHandle(engine, response_handle);
  }

  return message_result == kSuccess;
}

} // namespace flutter
