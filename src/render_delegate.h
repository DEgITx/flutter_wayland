#pragma once

#include <stdint.h>

namespace flutter {

class RenderDelegate {
 public:
  virtual bool OnApplicationContextMakeCurrent() = 0;

  virtual bool OnApplicationContextClearCurrent() = 0;

  virtual bool OnApplicationPresent() = 0;

  virtual uint32_t OnApplicationGetOnscreenFBO() = 0;
};

}  // namespace flutter
