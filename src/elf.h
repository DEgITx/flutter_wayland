// Copyrithg (c) 2019-2020 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
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

namespace flutter {

typedef struct {
} Aot_LoadedElf;

// Modeled after Dart_LoadELF() however, it is based on the dlopen()/dlsym() instead of relying on any piece of the code from the Dart Engine.
Aot_LoadedElf *Aot_LoadELF(const char *filename, const uint64_t file_offset, const char **error, const uint8_t **vm_snapshot_data, const uint8_t **vm_snapshot_instrs, const uint8_t **vm_isolate_data, const uint8_t **vm_isolate_instrs);

void Aot_UnloadELF(Aot_LoadedElf *loaded);

} // namespace flutter
