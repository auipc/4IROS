#pragma once

#include <kernel/mem/malloc.h>
#include <stddef.h>
#include <stdint.h>

#ifndef BITMAP_AVX
typedef uint64_t bitmap_container_t;
#else
struct bitmap_container_t {
	uint64_t bitz[4];
} PACKED;
#endif

class Bitmap {
  public:
	Bitmap(size_t bits);
	~Bitmap();
	void set(size_t i);
	void unset(size_t i);
	uint8_t get(size_t i) const;
	size_t scan(const size_t span);
	size_t scan_no_set(const size_t span) const;
	size_t count_unset() const;

  private:
	size_t count_unset_container(const size_t container) const;
	bitmap_container_t *m_data;
	static constexpr size_t s_bits_per_container =
		8 * sizeof(bitmap_container_t);
	size_t m_containers;
	size_t m_last_position = 0;
};
