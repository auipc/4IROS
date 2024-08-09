#pragma once

#include <kernel/mem/malloc.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t bitmap_container_t;

class Bitmap {
  public:
	Bitmap(size_t elems);
	~Bitmap();
	void set(size_t i);
	void unset(size_t i);
	uint8_t get(size_t i) const;
	uint32_t scan(uint32_t nr);
	uint32_t scan_no_set(uint32_t nr);
	uint32_t count();

	static size_t alloc_size(size_t elems);

  private:
	size_t m_elems;
	bitmap_container_t *data;
	size_t size;
};
