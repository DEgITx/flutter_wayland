#pragma once

#include <keys.h>
#include <stdint.h>

namespace flutter {

class DisplayEventListener {
 public:
  virtual void OnKeyboardKey(uint32_t evdev_keycode,
                             uint32_t xkb_keycode,
                             uint32_t utf32,
                             bool pressed,
                             SimpleKeyboardModifiers& mods) = 0;
};

}  // namespace flutter
