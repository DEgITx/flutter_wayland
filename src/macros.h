// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <iostream>
#include <string.h>
#include <sys/time.h>

#define FLWAY_DISALLOW_COPY(TypeName) TypeName(const TypeName &) = delete;

#define FLWAY_DISALLOW_ASSIGN(TypeName) void operator=(const TypeName &) = delete;

#define FLWAY_DISALLOW_COPY_AND_ASSIGN(TypeName)                                                                                                                                                                                               \
  FLWAY_DISALLOW_COPY(TypeName)                                                                                                                                                                                                                \
  FLWAY_DISALLOW_ASSIGN(TypeName)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define FL_PRINTF(out, color, type, msg, ...) {\
  timeval curTime;\
  char timebuffer[28];\
  gettimeofday(&curTime, NULL);\
  strftime(timebuffer, 28, "%H:%M:%S", localtime(&curTime.tv_sec));\
\
  fprintf(out, "[%s:%03ld] [%s] %s:%d: %s: " msg "\n", timebuffer, curTime.tv_usec / 1000, "\x1B[" color "m" type "\033[0m", __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
}

#define FL_ERROR(...) FL_PRINTF(stderr, "31", "E", ##__VA_ARGS__)
#define FL_WARN(...) FL_PRINTF(stdout, "33", "W", ##__VA_ARGS__)
#define FL_INFO(...) FL_PRINTF(stdout, "32", "I", ##__VA_ARGS__)
#define FL_DEBUG(...) FL_PRINTF(stdout, "36", "D", ##__VA_ARGS__)



