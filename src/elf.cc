// Copyrithg (c) 2019 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
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
#include <elfio/elfio.hpp>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "elf.h"
#include "macros.h"

namespace elfio = ::ELFIO;

namespace flutter {

class LoadedElf {
public:
  explicit LoadedElf(const char *const filename, const uint64_t elf_data_offset)
      : filename(strdup(filename), std::free)
      , elf_data_offset(elf_data_offset)
      , error_(nullptr)
      , fd(-1)
      , vm_snapshot_data_(MAP_FAILED)
      , vm_snapshot_instructions_(MAP_FAILED)
      , vm_isolate_snapshot_data_(MAP_FAILED)
      , vm_isolate_snapshot_instructions_(MAP_FAILED)
      , vm_snapshot_data_size(0)
      , vm_snapshot_instructions_size(0)
      , vm_isolate_snapshot_data_size(0)
      , vm_isolate_snapshot_instructions_size(0) {
  }

  ~LoadedElf() {
    unmap_all();

    if (fd != -1) {
      close(fd);
      fd = -1;
    }
  }

  bool Load() {
    if (elf_data_offset) {
      return false; // not supported
    }

    if (!reader.load(filename.get())) {
      return false;
    }

    fd = open(filename.get(), 0, O_RDONLY);

    if (fd == -1) {
      return false;
    }

    return true;
  }

  bool ResolveSymbols(const uint8_t **vm_snapshot_data, const uint8_t **vm_snapshot_instructions, const uint8_t **vm_isolate_snapshot_data, const uint8_t **vm_isolate_snapshot_instructions) {
    if (error_ != nullptr) {
      return false;
    }

    *vm_snapshot_data = *vm_snapshot_instructions = *vm_isolate_snapshot_data = *vm_isolate_snapshot_instructions = nullptr;

    const elfio::Elf_Half sec_num = reader.sections.size();

    for (auto i = 0; i < sec_num; i++) {
      elfio::section *const psec = reader.sections[i];

      if (psec->get_type() == SHT_DYNSYM) {
        const elfio::symbol_section_accessor symbols(reader, psec);

        FLWAY_LOG << " [" << i << "] " << psec->get_name() << "\t" << psec->get_size() << std::endl;

        for (auto j = 0; j < symbols.get_symbols_num(); j++) {
          std::string name;
          elfio::Elf64_Addr addr;
          elfio::Elf_Xword size;
          unsigned char bind;
          unsigned char type;
          elfio::Elf_Half section_index;
          unsigned char other;

          symbols.get_symbol(j, name, addr, size, bind, type, section_index, other);

          FLWAY_LOG << j << " " << name << " addr: 0x" << std::hex << addr << " size: 0x" << size << " index: " << std::dec << section_index << std::endl;

          if (std::string("_kDartVmSnapshotInstructions").compare(name) == 0) {
            vm_snapshot_instructions_ = reinterpret_cast<uint8_t *>(mmap(nullptr, vm_snapshot_instructions_size = size, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_POPULATE, fd, addr));

            if (vm_snapshot_instructions_ == MAP_FAILED) {
              error_ = strerror(errno);
              break;
            }
          } else if (std::string("_kDartIsolateSnapshotInstructions").compare(name) == 0) {
            vm_isolate_snapshot_instructions_ = reinterpret_cast<uint8_t *>(mmap(nullptr, vm_isolate_snapshot_instructions_size = size, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_POPULATE, fd, addr));

            if (vm_isolate_snapshot_instructions_ == MAP_FAILED) {
              error_ = strerror(errno);
              break;
            }
          } else if (std::string("_kDartVmSnapshotData").compare(name) == 0) {
            vm_snapshot_data_ = reinterpret_cast<uint8_t *>(mmap(nullptr, vm_snapshot_data_size = size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, addr));

            if (vm_snapshot_data_ == MAP_FAILED) {
              error_ = strerror(errno);
              break;
            }
          } else if (std::string("_kDartIsolateSnapshotData").compare(name) == 0) {
            vm_isolate_snapshot_data_ = reinterpret_cast<uint8_t *>(mmap(nullptr, vm_isolate_snapshot_data_size = size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, addr));

            if (vm_isolate_snapshot_data_ == MAP_FAILED) {
              error_ = strerror(errno);
              break;
            }
          }
        }

        break;
      }
    }

    if (vm_snapshot_data_ == MAP_FAILED || vm_snapshot_instructions_ == MAP_FAILED || vm_isolate_snapshot_data_ == MAP_FAILED || vm_isolate_snapshot_instructions_ == MAP_FAILED) {
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
  int fd;
  void *vm_snapshot_data_, *vm_snapshot_instructions_, *vm_isolate_snapshot_data_, *vm_isolate_snapshot_instructions_;
  size_t vm_snapshot_data_size, vm_snapshot_instructions_size, vm_isolate_snapshot_data_size, vm_isolate_snapshot_instructions_size;

  elfio::elfio reader;

private:
  FLWAY_DISALLOW_COPY_AND_ASSIGN(LoadedElf);

  void unmap_all() {
    if (vm_snapshot_data_ != MAP_FAILED) {
      munmap(vm_snapshot_data_, vm_snapshot_data_size);
      vm_snapshot_data_ = MAP_FAILED;
    }

    if (vm_snapshot_instructions_ != MAP_FAILED) {
      munmap(vm_snapshot_instructions_, vm_snapshot_instructions_size);
      vm_snapshot_instructions_ = MAP_FAILED;
    }

    if (vm_isolate_snapshot_data_ != MAP_FAILED) {
      munmap(vm_isolate_snapshot_data_, vm_isolate_snapshot_data_size);
      vm_isolate_snapshot_data_ = MAP_FAILED;
    }

    if (vm_isolate_snapshot_instructions_ != MAP_FAILED) {
      munmap(vm_isolate_snapshot_instructions_, vm_isolate_snapshot_instructions_size);
      vm_isolate_snapshot_instructions_ = MAP_FAILED;
    }
  }
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
