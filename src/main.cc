// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>

#include <string>
#include <vector>

#include "flutter_application.h"
#include "logger.h"
#include "utils.h"
#include "wayland_display.h"
#include "global_helpers.h"

#include <weston/config-parser.h>

namespace flutter {

static void PrintUsage() {
  std::cerr << "Flutter Wayland Embedder" << std::endl << std::endl;
  std::cerr << "========================" << std::endl;
  std::cerr << "Usage: `" << GetExecutableName()
            << " <asset_bundle_path> <flutter_flags>`" << std::endl
            << std::endl;
  std::cerr << R"~(
This utility runs an instance of a Flutter application and renders using
Wayland core protocols.

The Flutter tools can be obtained at https://flutter.io/

asset_bundle_path: The Flutter application code needs to be snapshotted using
                   the Flutter tools and the assets packaged in the appropriate
                   location. This can be done for any Flutter application by
                   running `flutter build bundle` while in the directory of a
                   valid Flutter project. This should package all the code and
                   assets in the "build/flutter_assets" directory. Specify this
                   directory as the first argument to this utility.

    flutter_flags: Typically empty. These extra flags are passed directly to the
                   Flutter engine. To see all supported flags, run
                   `flutter_tester --help` using the test binary included in the
                   Flutter tools.
)~" << std::endl;
}

#ifdef RDK

static std::pair<unsigned int, unsigned int> getDisplaySize() {
  unsigned int width = 0;
  unsigned int height = 0;

  weston_config* pConfig = weston_config_parse("/usr/bin/weston.ini");
  if (pConfig != NULL) {
    weston_config_section* pSection = weston_config_get_section(pConfig, "output", NULL, NULL);
    if (pSection != NULL) {
      char* displayModeStr = NULL;
      weston_config_section_get_string(pSection, "mode", &displayModeStr, "1280x720");

      if (displayModeStr != NULL) {
        std::string screenGeometry(displayModeStr);
        auto screenGeometryPair = splitStr(screenGeometry, "x");
        width = std::stoi(screenGeometryPair[0]);
        height = std::stoi(screenGeometryPair[1]);
        free(displayModeStr);
      } else {
        SPDLOG_ERROR("cannot get 'mode' from 'output' section from weston.ini");
      }
    } else {
      SPDLOG_ERROR("cannot get 'output' section from weston.ini");
    }

    weston_config_destroy(pConfig);
  } else {
    SPDLOG_ERROR("cannot read/parse weston.ini");
  }

  return std::make_pair(width, height);
}

#endif

static bool Main(std::vector<std::string> args) {
  if (args.size() == 0) {
    std::cerr << "   <Invalid Arguments>   " << std::endl;
    PrintUsage();
    return false;
  }

  const auto asset_bundle_path = args[0];

  if (!FlutterAssetBundleIsValid(asset_bundle_path)) {
    std::cerr << "   <Invalid Flutter Asset Bundle>   " << std::endl;
    PrintUsage();
    return false;
  }

  size_t kWidth = 800;
  size_t kHeight = 600;

#ifdef RDK
  auto screenGeometry = getDisplaySize();
  kWidth = screenGeometry.first;
  kHeight = screenGeometry.second;

  SPDLOG_DEBUG("weston display size = {}x{}",
               kWidth, kHeight);
#endif

  for (const auto& arg : args) {
    SPDLOG_DEBUG("Arg: {}", arg);
  }

  WaylandDisplay display(kWidth, kHeight);

  if (!display.IsValid()) {
    SPDLOG_ERROR("Wayland display was not valid.");
    return false;
  }

  FlutterApplication application(asset_bundle_path, args, display, display);
  if (!application.IsValid()) {
    SPDLOG_ERROR("Flutter application was not valid.");
    return false;
  }

  if (!application.SetWindowSize(kWidth, kHeight)) {
    SPDLOG_ERROR("Could not update Flutter application size.");
    return false;
  }

  display.Run();

  return true;
}

}  // namespace flutter

int main(int argc, char* argv[]) {
  auto console = spdlog::stdout_color_mt("console");
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_default_logger(console);
  spdlog::set_pattern("[%H:%M:%S.%e][%^%L%$] %s:%#: %!: %v");

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.push_back(argv[i]);
  }
  return flutter::Main(std::move(args)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
