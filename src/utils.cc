// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"

namespace flutter {

template <> std::string getEnv(const char *variable, std::string default_value) {
  if (variable == nullptr) {
    return default_value;
  }

  const char *str = getenv(variable);

  if (str == nullptr) {
    return default_value;
  }

  return std::string(str);
}

template <> double getEnv(char const *variable, double default_value) {
  std::string val = getEnv(variable, std::string(""));

  if (val.empty()) {
    return default_value;
  }

  return atof(val.c_str());
}

std::string GetICUDataPath() {
  auto base_directory = GetExecutableDirectory();
  if (base_directory == "") {
    base_directory = ".";
  }

  std::string data_directory = base_directory + "/data";
  std::string icu_data_path  = data_directory + "/icudtl.dat";

  do {
    if (std::ifstream(icu_data_path)) {
      FL_INFO("Using: %s", icu_data_path.c_str());
      break;
    }

    icu_data_path = "/usr/share/flutter/icudtl.dat";

    if (std::ifstream(icu_data_path)) {
      FL_INFO("Using: %s", icu_data_path.c_str());
      break;
    }

    FL_ERROR("Unnable to locate icudtl.dat file");
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
    FL_ERROR("Bundle directory: '%s' does not exist.", bundle_path.c_str());
    return false;
  }

  auto kernel_path = bundle_path + std::string{"/kernel_blob.bin"};
  auto aotelf_path = bundle_path + std::string{"/"} + FlutterGetAppAotElfName();
  auto kernel      = FileExistsAtPath(kernel_path);
  auto aotelf      = FileExistsAtPath(aotelf_path);

  if (!(kernel || aotelf)) {
    FL_ERROR("Could not found neither %s nor %s", kernel_path.c_str(), aotelf_path.c_str());
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
