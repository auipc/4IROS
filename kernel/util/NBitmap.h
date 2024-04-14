#pragma once

#include <kernel/mem/malloc.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t nbitmap_container_t;

class NBitmap {
  public:
	NBitmap(size_t elems);
	~NBitmap();
	void set(size_t i, uint8_t val);
	void unset(size_t i);
	uint8_t get(size_t i) const;
	void group_free(uint32_t start);
	uint32_t scan(uint32_t nr);
	uint32_t scan_no_set(uint32_t nr);
	uint32_t count();

  private:
	nbitmap_container_t *m_data;
	size_t m_size;
	size_t m_elems;
};
