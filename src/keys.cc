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

#include <iterator>
#include "keys.h"

namespace flutter {

GdkModifierType toGDKModifiers(struct xkb_keymap *xkb_keymap, guint32 mods) {
  static const struct {
    const char *xkb_name;
    guint32 gdk_mask;
  } table[] = {
      {XKB_MOD_NAME_CAPS, GDK_LOCK_MASK},
      {XKB_MOD_NAME_CTRL, GDK_CONTROL_MASK},
      {XKB_MOD_NAME_SHIFT, GDK_SHIFT_MASK},
      {XKB_MOD_NAME_ALT, GDK_MOD1_MASK},
      {XKB_MOD_NAME_NUM, GDK_MOD2_MASK},
      {XKB_MOD_NAME_LOGO, GDK_MOD4_MASK},
      {"Mod3", GDK_MOD3_MASK},
      {"Mod5", GDK_MOD5_MASK},
      {"Super", GDK_SUPER_MASK},
      {"Hyper", GDK_HYPER_MASK},
  };

  guint state = 0;

  for (size_t i = 0; i < std::size(table); i++) {
    if (mods & (1 << xkb_keymap_mod_get_index(xkb_keymap, table[i].xkb_name)))
      state |= table[i].gdk_mask;
  }

  if (mods & (1 << xkb_keymap_mod_get_index(xkb_keymap, "Meta")) && (state & GDK_MOD1_MASK) == 0)
    state |= GDK_META_MASK;

  return static_cast<GdkModifierType>(state);
}

} // namespace flutter
