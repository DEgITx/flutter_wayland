// Copyright (c) 2019 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//

#pragma once

#include <fmt/format.h>
#include <stdint.h>

#include "GLFW/glfw3.h"

namespace flutter {

class SimpleKeyboardModifiers {
 private:
  bool shift_ = false;
  bool ctrl_ = false;
  bool alt_ = false;
  bool super_ = false;
  bool caps_ = false;
  bool num_ = false;

 public:
  SimpleKeyboardModifiers() {}
  SimpleKeyboardModifiers(bool shift,
                     bool ctrl,
                     bool alt,
                     bool super,
                     bool caps,
                     bool num)
      : shift_(shift),
        ctrl_(ctrl),
        alt_(alt),
        super_(super),
        caps_(caps),
        num_(num) {}

  bool getShift() { return shift_; }

  bool getCtrl() { return ctrl_; }

  bool getAlt() { return alt_; }

  bool getSuper() { return super_; }

  bool getCaps() { return caps_; }

  bool getNum() { return num_; }

  std::string ToString() {
    return fmt::format("[shift {} ctrl {} alt {} super {} caps {} num {}]",
                       shift_, ctrl_, alt_, super_, caps_, num_);
  }
};

int toGLFWKeyCode(const uint32_t key);
int toGLFWModifiers(SimpleKeyboardModifiers& mods);

}  // namespace flutter
