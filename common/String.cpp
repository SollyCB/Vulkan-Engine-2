#include <cstdarg>

#include "String.hpp"
#include "Allocator.hpp"

namespace Sol {

/* StringView Start */
void StringView::init(const char* _string, size_t _length) {
  string = _string;
  length = _length;
}

/* StringView End */

/* StringBuffer Start */
void StringBuffer::init(size_t _capacity, Allocator* _allocator) {
  capacity = _capacity;
  allocator = _allocator;
  if (capacity != 0)
    data = reinterpret_cast<char*>(mem_alloc(capacity, allocator));
}

void StringBuffer::kill() {
  mem_free(data, allocator);
  capacity = 0;
  length = 0;
}

void StringBuffer::resize(size_t size) {
  data = reinterpret_cast<char*>(mem_realloc(size, data, allocator));
  capacity = size;
  if (length > size)
    length = size;
}

void StringBuffer::copy_here(const char* _data, size_t size) {
  if ((capacity - length) < size) 
    resize(size - (capacity - length));

  mem_cpy(data + length, _data, size);
  length += size;
}
void StringBuffer::copy_here(StringView *view) {
  if ((capacity - length) < view->length) 
    resize(view->length - (capacity - length));

  mem_cpy(data + length, view->string, view->length);
  length += view->length;
}

StringView StringBuffer::get_view(size_t start, size_t _length) {
  if (_length == 0) 
    _length = length;

  StringView view;
  view.length = _length;
  view.string = const_cast<const char*>(data + start);

  return view;
}
/* StringBuffer End */

/* StringArray Start */
void StringArray::init(size_t string_count, size_t _capacity, Allocator* _allocator) {
  buffer.init(_capacity, _allocator); 
  indices.init(string_count, _allocator);
}

void StringArray::kill() {
  buffer.kill();
  indices.kill();
}

size_t StringArray::get_str_count() {
  return indices.length;
}

void StringArray::push(const char* string, size_t length) {
  if ((buffer.capacity - buffer.length) < length)
    buffer.resize(buffer.capacity * 2 + length);

  StartEnd index;
  index.start = buffer.length;
  buffer.copy_here(string, length);
  index.length = buffer.length - index.start;
  indices.push(index);
}

StringView StringArray::pop() {
  StringView view;
  if (buffer.length == 0)
    return view;

  PopResult<StartEnd> pop = indices.pop();
  if (!pop.some) {
    buffer.length = 0;
    return view;
  }

  view = buffer.get_view(pop.item.start, pop.item.length);
  size_t cut_len = pop.item.length;
  buffer.length -= cut_len;

  return view;
}

void StringArray::resize(size_t size) {
  buffer.resize(size);
  indices.resize(size);
}

StringView StringArray::operator[](size_t i) {
  StartEnd index = indices[i];
  StringView view;
  view.string = buffer.data + index.start;
  view.length = index.length;
  return view;
}
/* StringArray End */

/* String Start */
void String::concat(StringBuffer *buffer, uint32_t string_count, ...) {
  va_list args;
  va_start(args, string_count);

  uint32_t lengths[string_count];
  const char* strings[string_count];
  size_t size = 1;

  for(uint32_t arg_index = 0; arg_index < string_count; ++arg_index) {
    uint32_t length = 0;
    const char* str = va_arg(args, const char*);
    for(uint32_t str_index = 0; str[str_index] != '\0'; ++str_index) {
      ++length;
      ++size;
    }
    lengths[arg_index] = length;
    strings[arg_index] = str;
  }

  buffer->resize(size);
  for(uint32_t arg_index = 0; arg_index < string_count; ++arg_index) {
    buffer->copy_here(strings[arg_index], lengths[arg_index]);
  } 
  buffer->data[buffer->length] = '\0';

  va_end(args);
}

size_t String::count(const char* str) {
  for(size_t i = 0; true; ++i) {
    if (str[i] == '\0')
      return i + 1;
  }
}

/* String End */

}
