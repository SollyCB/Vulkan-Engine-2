// clang-format on
#pragma once

#include "Allocator.hpp"
#include "hashmap_util.hpp"

namespace Sol {

struct Probe {
  size_t pos;
  size_t stride = 0;
  size_t stride_limit; // capacity - 1

  inline Probe(size_t start, size_t limit) {
    pos = start;
    stride_limit = limit;
  }
  inline bool increment() {
    pos += stride;
    pos &= stride_limit;

    if (stride >= stride_limit)
      return false;
    stride += 16;

    return true;
  }
}; // ProbeSequence

template <typename K, typename V> struct HashMap {
  struct KeyValue {
    K key;
    V value;
  };

  // pointers
  uint8_t *raw_bytes;
  Group *base_group;
  KeyValue *base_kv;

  // sizes
  size_t capacity = 0;
  size_t buckets_full = 0;
  size_t growth = 0;

  Allocator *allocator;

  /* Inner methods */
  inline size_t capacity_to_buckets() {
    size_t growth = capacity;
    if (!checked_mul(growth, 7))
      return UINT64_MAX;
    return (growth / 8) - buckets_full;
  }

  inline bool rehash_and_grow() {
    if ((UINT64_MAX >> 1) < capacity)
      return false;

    Group *old_group = base_group;
    KeyValue *old_kv = base_kv;

    capacity *= 2;
    buckets_full = 0;
    growth = capacity_to_buckets();
    raw_bytes = (uint8_t *)mem_alloca(capacity + (sizeof(KeyValue) * capacity),
                                      16, allocator);
    base_kv = (KeyValue *)(raw_bytes + capacity);
    base_group = (Group *)raw_bytes;
    initEmpty();

    // This algorithm may need testing for linear access optimisations
    for (size_t i = 0; i < capacity / 2; i += 16) {
      BitMask mask = old_group[i >> 4].isFull();
      while (mask.mask) {
        auto offset = mask.countTrailingZeros();
        if (insert(old_kv[i + offset].key, old_kv[i + offset].value) ==
            UINT64_MAX)
          return false;

        mask.mask ^= (1 << offset);
      }
    }

    mem_free(old_group, allocator);
    return true;
  }
  inline void update_sizes_on_insert() {
    --growth;
    ++buckets_full;
  }

  /* General API */
  inline void initEmpty() {
    for (size_t i = 0; i < (capacity >> 4); ++i) {
      base_group[i].ctrl = _mm_set1_epi8(Group::EMPTY);
    }
  }

  inline HashMap(size_t cap, Allocator *cator) {
    allocator = cator;
    next_pow_2(cap);
    capacity = cap;
    growth = capacity_to_buckets();

    // TODO: reinterpret_cast
    raw_bytes =
        (uint8_t *)mem_alloca(cap + (sizeof(KeyValue) * cap), 16, allocator);
    base_group = (Group *)raw_bytes;
    base_kv = (KeyValue *)(raw_bytes + cap);
    initEmpty();
  }

  inline void shutdown() {
    mem_free(raw_bytes, allocator);
    capacity = 0;
    buckets_full = 0;
    growth = 0;
  }

  inline size_t insert(K &key, V &value) {
    if (growth == 0)
      if (!rehash_and_grow())
        return UINT64_MAX;

    auto hash = calculateHash(key);
    auto primary_index = hash % capacity;
    auto offset_into_group = primary_index % 16;
    uint8_t top7 = (hash >> 57) & 0x7f;

    BitMask mask = (base_group + (primary_index >> 4))->isEmpty();
    if (mask.mask & (1 << offset_into_group)) {
      raw_bytes[primary_index] &= top7;

      KeyValue *kv = base_kv + primary_index;
      kv->key = key;
      kv->value = value;

      update_sizes_on_insert();
      return primary_index;
    }
    auto adjusted_index = primary_index - offset_into_group;
    Probe probe(adjusted_index, capacity - 1);
    while (probe.increment()) {
      mask = (base_group + (probe.pos >> 4))->isEmpty();
      if (!mask.mask)
        continue;

      auto offset = mask.countTrailingZeros();
      raw_bytes[probe.pos + offset] &= top7;

      KeyValue *kv = base_kv + probe.pos + offset;
      kv->key = key;
      kv->value = value;

      update_sizes_on_insert();
      return (probe.pos + offset);
    }

    return UINT64_MAX;
  } // fn insert

  inline KeyValue *get(K &key) {
    auto hash = calculateHash(key);
    auto primary_index = hash % capacity;
    if (base_kv[primary_index].key == key)
      return base_kv + primary_index;

    uint8_t top7 = (hash >> 57) & 0x7f;
    BitMask mask = (base_group + (primary_index >> 4))->matchByte(top7);
    auto adjusted_index = primary_index - (primary_index % 16);

    Probe probe(adjusted_index, capacity - 1);
    while (probe.increment()) {
      mask = base_group[probe.pos >> 4].isFull();
      while (mask.mask) {
        auto offset = mask.countTrailingZeros();
        if (key == base_kv[probe.pos + offset].key)
          return base_kv + probe.pos + offset;

        mask.mask ^= (1 << offset);
      }
    }
    return nullptr;
  } // fn get

  inline bool rehash_and_grow(size_t new_cap) {
    if ((UINT64_MAX >> 1) < new_cap || new_cap < capacity)
      return false;
    next_pow_2(new_cap);

    Group *old_group = base_group;
    KeyValue *old_kv = base_kv;

    capacity = new_cap;
    buckets_full = 0;
    growth = capacity_to_buckets();
    raw_bytes = (uint8_t *)mem_alloca(capacity + (sizeof(KeyValue) * capacity),
                                      16, allocator);
    base_kv = (KeyValue *)(raw_bytes + capacity);
    base_group = (Group *)raw_bytes;
    initEmpty();

    // This algorithm may need testing for linear access optimisations
    for (size_t i = 0; i < capacity / 2; i += 16) {
      BitMask mask = old_group[i >> 4].isFull();
      while (mask.mask) {
        auto offset = mask.countTrailingZeros();
        if (insert(old_kv[i + offset].key, old_kv[i + offset].value) ==
            UINT64_MAX)
          return false;

        mask.mask ^= (1 << offset);
      }
    }

    mem_free(old_group, allocator);
    return true;
  }
};

} // namespace Sol
