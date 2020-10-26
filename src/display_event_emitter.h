#pragma once

#include <vector>

#include "display_event_listener.h"

namespace flutter {

class DisplayEventEmitter {
 protected:
  std::vector<DisplayEventListener*> kEventListeners;

 public:
  void addListener(DisplayEventListener* l) { kEventListeners.push_back(l); }

  void removeListener(DisplayEventListener* l) {
    std::remove(kEventListeners.begin(), kEventListeners.end(), l);
  }
};

}  // namespace flutter