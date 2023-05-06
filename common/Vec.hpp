#pragma once

// clang-format off

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>

#include "Allocator.hpp"

namespace Sol {

template <typename T> 
struct PopResult {
	bool some = false;
	T item;
	bool is_some() {
		return some;
	}
	T get() { // moves the value from item, I do not really know if this matters...
		return std::move(item);
	}
};

template <typename T>
struct Vec {
  size_t length = 0;
  size_t capacity = 0;
  T* data = nullptr;
  Allocator* allocator = nullptr;

  /* General API */
  void init(size_t cap, Allocator* alloc) {
    capacity = cap;
    allocator = alloc;
    data = (T*)mem_alloca(cap * sizeof(T), 8, alloc);
  }

  void kill() {
    mem_free(data, allocator);
		length = 0;
		capacity = 0;
  }
  void print() {
    for(size_t i = 0; i < length; ++i) {
      std::cout << data[i] << '\n';
    }
  }
  void zero(T t) {
    for(size_t i = 0; i < capacity; ++i) {
      push(t);
    }
  }
  size_t len() {
    return length;
  }
  size_t cap() {
    return capacity;
  }
  PopResult<T> pop() {
    if (!length) {
			PopResult<T> none;
			none.some = false;
			return none;
		}
    --length; // decrement length
		return PopResult<T> { true, data[length] };
  }
  void push(T t) {
    if (capacity == length)
      grow();
    data[length] = t;
    ++length;
  }
  void push_ref(T& t) {
    if (capacity == length)
      grow();
    data[length] = t;
    ++length;
  }
  void push_ptr(T* t) {
    if (capacity == length)
      grow();
    data[length] = t;
    ++length;
  }
  void resize(size_t size) {
    data = reinterpret_cast<T*>(mem_realloc(size * sizeof(T), data, allocator));
  }
  void grow(size_t size) {
    capacity *= size;
    T* new_data = (T*)mem_alloca(capacity * sizeof(T), 8, allocator);
    mem_cpy(new_data, data, length * sizeof(T));
    mem_free(data, allocator);
    data = new_data;
  }
  void grow() {
    capacity *= 2;
    T* new_data = (T*)mem_alloca(capacity * sizeof(T), 8, allocator);
    mem_cpy(new_data, data, length * sizeof(T));
    mem_free(data, allocator);
    data = new_data;
  }
  T& operator[](size_t i) {
    if (i >= length) {
      std::cerr << "OUT OF BOUNDS ACCESS ON VEC " << data << "\n";
      exit(-1);
    }
    return data[i];
  }
};

template <typename T>
struct TempVec {
  Vec<T> vec;
  TempVec(size_t cap, Allocator* allocator) {
    Vec<T> vec_;
    vec_.init(cap, allocator);
    vec = vec_;
  }
  ~TempVec() {
    vec.kill();
  }
};

} // namespace Sol
