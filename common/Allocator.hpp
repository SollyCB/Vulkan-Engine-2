#pragma once
// clang-format off

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>

#include "tlsf.h"

#define MEM_STATS

namespace Sol {

inline size_t memory_align(size_t size, size_t alignment) {
  const size_t alignment_mask = alignment - 1;
  return (size + alignment_mask) & ~alignment_mask;
}

struct MemoryStats {
  size_t allocated_bytes;
  size_t total_bytes;
  uint32_t allocation_count = 0;
  std::vector<void*> active_allocations;
  void add(std::size_t a) {
    if (a) {
      allocated_bytes += a;
      ++allocation_count;
    }
  }
};
static std::vector<void*> Global_Allocation_Tracker;

struct Allocator {
  virtual ~Allocator() {}
  virtual void *allocate(size_t size, size_t alignment) = 0;
  virtual void *reallocate(size_t size, void* ptr) = 0;
  virtual void deallocate(void* ptr) = 0; 
};

struct HeapAllocator : public Allocator {
  ~HeapAllocator() override;

  void *handle;
  void *memory;
  size_t allocated = 0;
  size_t limit = 0;
#if defined MEM_STATS
  std::vector<void*>* active_allocations = &Global_Allocation_Tracker;
#endif

  /* Initialize/Kill service */
  void init(size_t size);
  void shutdown();

  /* General API */
  void *allocate(size_t size, size_t alignment) override;
  void *reallocate(size_t size, void* ptr) override;
  void deallocate(void* ptr) override; 
};

struct MemoryConfig {
  size_t default_size = 32 * 1024 * 1024;
};

struct MemoryService {
  HeapAllocator system_allocator;
  // return a pointer to an instance of a static MemoryService
  static MemoryService* instance();
  void init(MemoryConfig* config);
  // free all memory associated with the service
  void shutdown();
};

inline void mem_cpy(void* to, void* from, size_t size);

#if 1
#define mem_cpy(to, from, size) (memcpy(to, from, size))
#define mem_alloca(size, alignment, allocator) ((allocator)->allocate(size, alignment))
#define mem_alloc(size, allocator) ((allocator)->allocate(size, 1))
#define mem_realloc(size, ptr, allocator) ((allocator)->reallocate(size, ptr))
#define mem_free(ptr, allocator) ((allocator)->deallocate(ptr))
#endif

} // Sol
