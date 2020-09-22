# Copyright 2018 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

option(RDK "Compile under RDK platform" OFF)

if(RDK)
  set(CMAKE_TOOLCHAIN_FILE toolchain.cmake)
endif()

cmake_minimum_required(VERSION 3.5.2)
project(flutter_wayland)

set(CMAKE_CXX_STANDARD 14)

include(DownloadProject/DownloadProject.cmake)

option(WITH_XDG_SHELL "Use xdg-shell protocol extension" OFF)

find_package(PkgConfig)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_EGL    REQUIRED wayland-egl)
pkg_check_modules(EGL            REQUIRED egl)
pkg_check_modules(XKB            REQUIRED xkbcommon)

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

download_project(PROJ                nlohmann_json
                 GIT_REPOSITORY      https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
                 GIT_TAG             v3.9.1
                 UPDATE_DISCONNECTED 1
)
add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR})

download_project(PROJ                libuv
                 GIT_REPOSITORY      https://github.com/libuv/libuv
                 GIT_TAG             v1.39.0
                 UPDATE_DISCONNECTED 1
)
add_subdirectory(${libuv_SOURCE_DIR} ${libuv_BINARY_DIR})

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

if(RDK)
  add_definitions(-DGLFW_INCLUDE_ES1 -DRDK)
  set(WAYLAND_EGL_LIBRARIES ${WAYLAND_EGL_LIBRARIES} nxpl display_manager)
endif()

set(FLUTTER_PATH "" CACHE STRING "Flutter Engine path")
if(NOT FLUTTER_PATH STREQUAL "")
  if(RDK)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
  endif()
  find_library(FLUTTER_LIBRARY NAMES flutter_engine HINTS ${FLUTTER_PATH})
  find_path(FLUTTER_INCLUDE_DIR flutter_embedder.h HINTS ${FLUTTER_PATH})
  file(COPY ${FLUTTER_INCLUDE_DIR}/icudtl.dat
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  if(RDK)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  endif()
else()
  set(FLUTTER_LIBRARY "flutter_engine")
  set(FLUTTER_INCLUDE_DIR "")
endif()

link_directories(${CMAKE_BINARY_DIR})

add_executable(flutter_wayland ${FLUTTER_WAYLAND_SRC})

target_link_libraries(flutter_wayland
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  uv_a
  ${WAYLAND_CLIENT_LIBRARIES}
  ${WAYLAND_EGL_LIBRARIES}
  ${EGL_LIBRARIES}
  ${XKB_LIBRARIES}
  ${FLUTTER_LIBRARY}
)

target_include_directories(flutter_wayland
  PRIVATE
  ${WAYLAND_CLIENT_INCLUDE_DIRS}
  ${WAYLAND_EGL_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${XKB_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}
  ${FLUTTER_INCLUDE_DIR}
)
