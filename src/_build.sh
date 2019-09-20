#!/bin/bash
EMBEDDER_INC=/home/sw/projects/flutter/engine/src/out/linux_debug_unopt_x64
FLUTTER_LIBDIR=/home/sw/projects/flutter/engine/src/out/linux_debug_unopt_x64

g++ -O0 -ggdb3 -I${EMBEDDER_INC} -L${FLUTTER_LIBDIR} -lflutter_engine -o flutter.bin flutter_application.cc main.cc utils.cc wayland_display.cc  $(pkg-config --cflags --libs wayland-client wayland-egl glesv2 egl)