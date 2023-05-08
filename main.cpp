// clang-format on

#include <GLFW/glfw3.h>
#include <iostream>

#include "Engine.hpp"
#include "common/Allocator.hpp"
#include "common/Vec.hpp"

static Sol::Engine engine;

int main() {
  Sol::MemoryConfig mem_config;
  Sol::MemoryService::instance()->init(&mem_config);
  Sol::Allocator* allocator = &Sol::MemoryService::instance()->system_allocator;

  engine.run(allocator);
  engine.kill();

  Sol::MemoryService::instance()->shutdown();
  return 0;
}

