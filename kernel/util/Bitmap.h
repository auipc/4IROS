#pragma once

#include <kernel/stdint.h>
#include <kernel/mem/malloc.h>

class Bitmap {
  public:
	Bitmap(size_t elems);
	~Bitmap();
	void set(size_t i);
	void unset(size_t i);
	uint8_t get(size_t i) const;
	uint32_t scan(uint32_t nr);
	uint32_t count();
  private:
	uint8_t* data;
	size_t size;
};