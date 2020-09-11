# Copyright 2018 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.14)

include(FetchContent)

project(flutter_wayland)

option(WITH_XDG_SHELL "Use xdg-shell protocol extension" OFF)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_EGL    REQUIRED wayland-egl)
pkg_check_modules(EGL            REQUIRED egl)
pkg_check_modules(GLFW           REQUIRED glfw3)

set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 7.0.3
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.8.0
)
option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" OFF)
set(SPDLOG_FMT_EXTERNAL ON) # Using external fmt library added above (bundled version can be outdated)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.9.1
)
FetchContent_MakeAvailable(nlohmann_json)

# Executable
file(GLOB_RECURSE FLUTTER_WAYLAND_SRC
  "src/*.cc"
  "src/*.h"
)

if(WITH_XDG_SHELL)
  find_package(ECM REQUIRED)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_FIND_MODULE_DIR})

  find_package(WaylandScanner REQUIRED)
  pkg_check_modules(wayland-protocols REQUIRED wayland-protocols>=1.12)

  execute_process(COMMAND pkg-config wayland-protocols --variable=pkgdatadir
    OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  ecm_add_wayland_client_protocol(FLUTTER_WAYLAND_SRC
      PROTOCOL ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml
      BASENAME xdg-shell
  )

  add_definitions(-DUSE_XDG_SHELL)
endif()

link_directories(${CMAKE_BINARY_DIR})

add_executable(flutter_wayland ${FLUTTER_WAYLAND_SRC})

target_link_libraries(flutter_wayland
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  ${WAYLAND_CLIENT_LIBRARIES}
  ${WAYLAND_EGL_LIBRARIES}
  ${EGL_LIBRARIES}
  flutter_engine
)

target_include_directories(flutter_wayland
  PRIVATE
  ${WAYLAND_CLIENT_INCLUDE_DIRS}
  ${WAYLAND_EGL_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}
)
