# Copyright 2018 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.5.2)

include(DownloadProject.cmake)

project(flutter_wayland)

option(WITH_XDG_SHELL "Use xdg-shell protocol extension" OFF)

set(CMAKE_CXX_STANDARD 14)

set(FLUTTER_ENGINE_SHA b9523318caa1a99ffde8adaf331212eb879cabc9)

set(FLUTTER_EMBEDDER_ARTIFACTS_ZIP ${CMAKE_BINARY_DIR}/flutter_embedder_${FLUTTER_ENGINE_SHA}.zip)
set(FLUTTER_ARTIFACTS_ZIP          ${CMAKE_BINARY_DIR}/flutter_artifact_${FLUTTER_ENGINE_SHA}.zip)
set(FLUTTER_BUCKET_BASE "https://storage.googleapis.com/flutter_infra/flutter")

# Download and setup the Flutter Engine.
if(NOT EXISTS ${FLUTTER_EMBEDDER_ARTIFACTS_ZIP})
  file(DOWNLOAD
    ${FLUTTER_BUCKET_BASE}/${FLUTTER_ENGINE_SHA}/linux-x64/linux-x64-embedder
    ${FLUTTER_EMBEDDER_ARTIFACTS_ZIP}
    SHOW_PROGRESS
  )
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${FLUTTER_EMBEDDER_ARTIFACTS_ZIP}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()

if(NOT EXISTS ${FLUTTER_ARTIFACTS_ZIP})
  file(DOWNLOAD
    ${FLUTTER_BUCKET_BASE}/${FLUTTER_ENGINE_SHA}/linux-x64/artifacts.zip
    ${FLUTTER_ARTIFACTS_ZIP}
    SHOW_PROGRESS
  )
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${FLUTTER_ARTIFACTS_ZIP}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()

find_package(PkgConfig)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_EGL    REQUIRED wayland-egl)
pkg_check_modules(EGL            REQUIRED egl)

download_project(PROJ                fmt
                 GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
                 GIT_TAG             7.0.3
                 UPDATE_DISCONNECTED 1
)
add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})

download_project(PROJ                spdlog
                 GIT_REPOSITORY      https://github.com/gabime/spdlog.git
                 GIT_TAG             v1.8.0
                 UPDATE_DISCONNECTED 1
)
option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" OFF)
set(SPDLOG_FMT_EXTERNAL ON) # Using external fmt library added above (bundled version can be outdated)
add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})

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
  ${CMAKE_BINARY_DIR}
)
