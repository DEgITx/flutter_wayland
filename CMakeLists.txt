# Copyright 2018 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


cmake_minimum_required(VERSION 3.3)

project(flutter-launcher-wayland)

find_package(PkgConfig)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
find_package(WaylandScanner REQUIRED)
find_package(WaylandProtocols REQUIRED)
pkg_search_module(XKB xkbcommon REQUIRED)
pkg_search_module(EGL egl REQUIRED)
pkg_search_module(WAYLAND_CLIENT wayland-client REQUIRED)
pkg_search_module(WAYLAND_EGL wayland-egl REQUIRED)
pkg_search_module(GDK gdk-3.0 REQUIRED) # dw: Not used for linking, we just need an access to the header
# If you do not have flutter-engine.pc file
# just provide all FLUTTER_ENGINE* variables using -D
pkg_search_module(FLUTTER_ENGINE flutter-engine)

if (CMAKE_VERSION VERSION_LESS "3.8.2")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else()
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(SOURCES
    src/main.cc
    src/elf.cc
    src/keys.cc
    src/egl_utils.cc
    src/utils.cc
    src/wayland_display.cc
    src/elf.h
    src/macros.h
    src/keys.h
    src/utils.h
    src/egl_utils.h
    src/wayland_display.h
)

ecm_add_wayland_client_protocol(
    SOURCES
    PROTOCOL "${WaylandProtocols_DATADIR}/stable/presentation-time/presentation-time.xml"
    BASENAME "presentation-time"
)

ecm_add_wayland_client_protocol(
    SOURCES
    PROTOCOL "${WaylandProtocols_DATADIR}/unstable/xwayland-keyboard-grab/xwayland-keyboard-grab-unstable-v1.xml"
    BASENAME "xwayland-keyboard-grab"
)

link_directories(
    ${XKB_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
    ${WAYLAND_EGL_LIBRARY_DIRS}
    ${FLUTTER_ENGINE_LIBRARY_DIRS}
)

add_executable(flutter-launcher-wayland ${SOURCES})

target_compile_definitions(flutter-launcher-wayland PRIVATE "-DGLFW_INCLUDE_ES2")

target_include_directories(flutter-launcher-wayland
  PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GLFW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${XKB_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${WAYLAND_CLIENT_INCLUDE_DIRS}
  ${WAYLAND_EGL_INCLUDE_DIRS}
  ${GDK_INCLUDE_DIRS}
  ${FLUTTER_ENGINE_INCLUDE_DIRS}
)

target_link_libraries(flutter-launcher-wayland
  ${CMAKE_DL_LIBS}
  ${XKB_LIBRARIES}
  ${EGL_LIBRARIES}
  ${WAYLAND_CLIENT_LIBRARIES}
  ${WAYLAND_EGL_LIBRARIES}
  ${FLUTTER_ENGINE_LIBRARIES}
)

install(TARGETS flutter-launcher-wayland DESTINATION bin)
