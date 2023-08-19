#pragma once

#include <kernel/stdint.h>
#include <kernel/mem/malloc.h>

class Bitmap {
  public:
	Bitmap(size_t elems);
	~Bitmap();
	void set(int i);
	void unset(int i);
	uint8_t get(int i) const;
	uint32_t scan(uint32_t nr);
	uint32_t count();
  private:
	uint8_t* data;
	size_t size;
};