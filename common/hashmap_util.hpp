#pragma once

#include <emmintrin.h>
#include <math.h>

#include "wyhash.h"

template <typename T>
inline uint64_t calculateHash(const T &value, size_t seed = 0) {
  return wyhash(&value, sizeof(T), seed, _wyp);
}

template <size_t N>
inline uint64_t calculateHash(const char (&value)[N], size_t seed = 0) {
  return wyhash(value, strlen(value), seed, _wyp);
}

// template <> TODO: idk if this will compile with this commented...
inline uint64_t calculateHash(const char *value, size_t seed) {
  return wyhash(value, strlen(value), seed, _wyp);
}

inline uint64_t hashBytes(void *data, size_t len, size_t seed = 0) {
  return wyhash(data, len, seed, _wyp);
}

inline bool checked_mul(size_t &res, size_t mul) {
  if (UINT64_MAX / res < mul)
    return false;
  res = res * mul;
  return true;
}

inline void next_pow_2(size_t &cap) {
  if (cap <= 16)
    cap = 16;

  size_t log = log2(cap);
  if (pow(2, log) == cap) {
  } else
    cap = pow(2, log + 1);
}

struct BitMask {
  uint16_t mask;

  // methods for checking bits...
  inline uint32_t countTrailingZeros() { return __builtin_ctzl(mask); }
};

// sse2 - Groups are 16 byte wide.
struct Group {
  static const uint8_t EMPTY = 0b1111'1111;
  static const uint8_t DEL = 0b1000'0000;
  __m128i ctrl;

  inline BitMask isEmpty() {
    __m128i empty = _mm_set1_epi8(EMPTY);
    __m128i res = _mm_cmpeq_epi8(ctrl, empty);
    uint16_t mask = _mm_movemask_epi8(res);
    return BitMask{mask};
  }
  inline BitMask isSpecial() {
    uint16_t mask = _mm_movemask_epi8(ctrl);
    return BitMask{mask};
  }
  inline BitMask isFull() {
    BitMask mask = isSpecial();
    uint16_t invert = ~mask.mask;
    return BitMask{invert};
  }
  inline void prepareToMakeFull(uint8_t *bytes) {
    _mm_store_si128(reinterpret_cast<__m128i *>(bytes), ctrl);
  }
  inline BitMask matchByte(uint8_t byte) {
    __m128i to_match = _mm_set1_epi8(byte);
    __m128i match = _mm_cmpeq_epi8(ctrl, to_match);
    BitMask mask;
    mask.mask = (uint16_t)_mm_movemask_epi8(match);
    return mask;
  }
};
