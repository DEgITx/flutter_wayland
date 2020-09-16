#pragma once

#define SPDLOG_ACTIVE_LEVEL \
  SPDLOG_LEVEL_DEBUG  // TODO: make configurable with CMake
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
