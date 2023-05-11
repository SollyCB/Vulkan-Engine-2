#pragma once 

#include "Allocator.hpp"
#include "Vec.hpp"

namespace Sol {

struct StringView {
  const char* string;
  size_t length = 0;

  void init(const char* _string, size_t _length);
};

struct StringBuffer {
  size_t capacity = 0;
  size_t length = 0;
  char* data = nullptr;
  Allocator* allocator;

  void init(size_t _capacity, Allocator* _allocator);
  void kill();
  void resize(size_t size);
  void copy_here(const char* _data, size_t size);
  void copy_here(StringView *view);
  // if _length == 0, whole buffer is returned
  StringView get_view(size_t start, size_t _length);
};

struct StringArray {
  struct StartEnd {
    size_t start = 0;
    size_t length = 0;
  };
  StringBuffer buffer;
  Vec<StartEnd> indices;
  
  void init(size_t string_count, size_t _capacity, Allocator* _allocator);
  void kill();
  size_t get_str_count();
  void resize(size_t size);
  void push(const char* string, size_t length);
  void push(StringView *view);
  // This will return a view to memory which will be marked for override...
  StringView pop();

  StringView operator[](size_t i);
};

struct String {
  // Have to call with c_string, so use <StringView>.string...
  static void concat(StringBuffer *buffer, uint32_t string_count, ...);
  static size_t count(const char* str);
};

}
