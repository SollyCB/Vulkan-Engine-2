// clang-format off
#include <iostream>
#include <cstring>

#include "../include/tlsf.h"
#include "Allocator.hpp"

//#define NDEBUG
#include <assert.h>

namespace Sol {

/* MemoryService */
static MemoryService GlobalMemoryService;
MemoryService *MemoryService::instance() { return &GlobalMemoryService; }
void MemoryService::init(MemoryConfig* config) {
  std::cout << "Initializing memory service, allocating " << config->default_size << " bytes...\n";
  system_allocator.init(config->default_size);
}
void MemoryService::shutdown() { system_allocator.shutdown(); }

/* HeapAllocator */
HeapAllocator::~HeapAllocator() { }

  /* Initialize/Kill service */
void HeapAllocator::init(size_t size) {
  memory = malloc(size);
  limit = size;
  handle = tlsf_create_with_pool(memory, size);
  std::cout << "HeapAllocator, size " << size << " created...\n";
} // init

void HeapAllocator::shutdown() {
  MemoryStats stats = { 0, limit };
  pool_t pool = tlsf_get_pool(handle);
  tlsf_walk_pool(pool, nullptr, (void*)&stats);
  if (stats.allocated_bytes)
    std::cerr << "FAILED TO SHUTDOWN HEAPALLOCATOR! DETECTED ALLOCATED MEMORY!\n"
      << "  Allocated: " << stats.allocated_bytes << '\n'
      << "  Total: " << stats.total_bytes << '\n';
  else 
    std::cout << "HeapAllocator successfully shutdown! All memory free!\n";

  assert(stats.allocated_bytes == 0 && "MEMORY IS STILL ALLOCATED\n");
  tlsf_destroy(handle);
  free(memory);
} // shutdown

  /* General API */
void *HeapAllocator::allocate(size_t size, size_t alignment) { 
  void *allocated_mem = alignment == 1 ? tlsf_malloc(handle, size) : tlsf_memalign(handle, alignment, size);
  size_t actual_size = tlsf_block_size(allocated_mem);
  allocated += actual_size;
#if defined MEM_STATS 
  active_allocations->push_back(allocated_mem);
#endif
  return allocated_mem;
}
void *HeapAllocator::reallocate(size_t size, void* ptr) { 
  void *allocated_mem = tlsf_realloc(handle, ptr, size);
  size_t actual_size = tlsf_block_size(allocated_mem);
  allocated += actual_size;
  return allocated_mem;
}
void HeapAllocator::deallocate(void* ptr) {
  size_t actual_size = tlsf_block_size(ptr);
  allocated -= actual_size;
  tlsf_free(handle, ptr);
#if defined MEM_STATS
  // TODO: make this use SIMD to scan the pointers...
  for(size_t i = 0; i < active_allocations->size(); ++i) {
    if ((*active_allocations)[i] == ptr) {
      size_t last_index = active_allocations->size() - 1;
      void* item = (*active_allocations)[last_index];
      (*active_allocations)[i] = item;   
      active_allocations->pop_back();
    }
  }
#endif
} 

} // namespace Sol
