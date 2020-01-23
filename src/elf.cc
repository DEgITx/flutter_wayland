// Copyright (c) 2019-2020 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
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
//.
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//

#include <memory>
#include <cstring>
#include <dlfcn.h>
#include "elf.h"
#include "macros.h"

namespace flutter {

class LoadedElf {
public:
  explicit LoadedElf(const char *const filename, const uint64_t elf_data_offset)
      : filename(strdup(filename), std::free)
      , elf_data_offset(elf_data_offset)
      , error_(nullptr)
      , fd(NULL)
      , vm_snapshot_data_(NULL)
      , vm_snapshot_instructions_(NULL)
      , vm_isolate_snapshot_data_(NULL)
      , vm_isolate_snapshot_instructions_(NULL) {
  }

  ~LoadedElf() {
    if (fd) {
      dlclose(fd);
      fd = NULL;
    }
  }

  bool Load() {
    if (elf_data_offset) {
      return false; // not supported
    }

    fd = dlopen(filename.get(), RTLD_LOCAL | RTLD_NOW);

    if (fd == NULL) {
      return false;
    }

    return true;
  }

  bool ResolveSymbols(const uint8_t **vm_snapshot_data, const uint8_t **vm_snapshot_instructions, const uint8_t **vm_isolate_snapshot_data, const uint8_t **vm_isolate_snapshot_instructions) {
    if (error_ != nullptr) {
      return false;
    }

    *vm_snapshot_data = *vm_snapshot_instructions = *vm_isolate_snapshot_data = *vm_isolate_snapshot_instructions = nullptr;

    do {

      vm_snapshot_instructions_ = dlsym(fd, "_kDartVmSnapshotInstructions");

      if (vm_snapshot_instructions_ == NULL) {
        error_ = strerror(errno);
        break;
      }

      vm_isolate_snapshot_instructions_ = dlsym(fd, "_kDartIsolateSnapshotInstructions");

      if (vm_isolate_snapshot_instructions_ == NULL) {
        error_ = strerror(errno);
        break;
      }

      vm_snapshot_data_ = dlsym(fd, "_kDartVmSnapshotData");

      if (vm_snapshot_data_ == NULL) {
        error_ = strerror(errno);
        break;
      }

      vm_isolate_snapshot_data_ = dlsym(fd, "_kDartIsolateSnapshotData");

      if (vm_isolate_snapshot_data_ == NULL) {
        error_ = strerror(errno);
        break;
      }
    } while (0);

    if (vm_snapshot_data_ == NULL || vm_snapshot_instructions_ == NULL || vm_isolate_snapshot_data_ == NULL || vm_isolate_snapshot_instructions_ == NULL) {
      return false;
    }

    *vm_snapshot_data                 = reinterpret_cast<const uint8_t *>(vm_snapshot_data_);
    *vm_snapshot_instructions         = reinterpret_cast<const uint8_t *>(vm_snapshot_instructions_);
    *vm_isolate_snapshot_data         = reinterpret_cast<const uint8_t *>(vm_isolate_snapshot_data_);
    *vm_isolate_snapshot_instructions = reinterpret_cast<const uint8_t *>(vm_isolate_snapshot_instructions_);

    return true;
  }

  const char *error() {
    return error_;
  }

protected:
  std::unique_ptr<char, decltype(std::free) *> filename;
  const uint64_t elf_data_offset;
  const char *error_;
  void *fd;
  void *vm_snapshot_data_, *vm_snapshot_instructions_, *vm_isolate_snapshot_data_, *vm_isolate_snapshot_instructions_;

private:
  FLWAY_DISALLOW_COPY_AND_ASSIGN(LoadedElf);
};

Aot_LoadedElf *Aot_LoadELF(const char *filename, const uint64_t file_offset, const char **error, const uint8_t **vm_snapshot_data, const uint8_t **vm_snapshot_instrs, const uint8_t **vm_isolate_snapshot_data,
                           const uint8_t **vm_isolate_snapshot_instructions) {

  auto elf = std::make_unique<LoadedElf>(filename, file_offset);

  if (!elf->Load() || !elf->ResolveSymbols(vm_snapshot_data, vm_snapshot_instrs, vm_isolate_snapshot_data, vm_isolate_snapshot_instructions)) {
    *error = elf->error();
    return nullptr;
  }

  return reinterpret_cast<Aot_LoadedElf *>(elf.release());
}

} // namespace flutter
