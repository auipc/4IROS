#include <kernel/util/Bitmap.h>
#include <kernel/assert.h>
#include <kernel/printk.h>
#include <kernel/stdint.h>
#include <kernel/mem/malloc.h>
#include <kernel/string.h>

Bitmap::Bitmap(size_t elems)
{
	// uint32_t containers might be better
	data = new uint8_t[elems];
	memset(reinterpret_cast<char*>(data), 0, elems);
	size = elems / 8;
}

Bitmap::~Bitmap() { delete[] data; }

void Bitmap::set(int i) {
	if (i < 0 || i >= size) {
		assert(false);
		return;
	}
	int byteIndex = i / 8;
	int bitIndex = i % 8;
	unsigned char mask = 1 << bitIndex;
	data[byteIndex] |= mask; // Set the bit to 1
}

void Bitmap::unset(int i) {
	if (i < 0 || i >= size) {
		assert(false);
		return;
	}
	int byteIndex = i / 8;
	int bitIndex = i % 8;
	unsigned char mask = 1 << bitIndex;
	data[byteIndex] &= ~mask; // Set the bit to 0
}

uint8_t Bitmap::get(int i) const {
	if (i < 0 || i >= size) {
		assert(false);
	}
	int byteIndex = i / 8;
	int bitIndex = i % 8;
	return (data[byteIndex] >> bitIndex) & 1;
}

uint32_t Bitmap::scan(uint32_t nr) {
	uint32_t streak = 0;
	for (int i = 0; i < size; i++) {
		if (!get(i)) {
			streak++;
		} else {
			streak = 0;
		}

		if (streak == nr) {
			// FIXME does this work?
			for (int j = i; j > i - streak; j--)
				set(j);

			return i;
		}
	}
	assert(false);
}

uint32_t Bitmap::count() {
	uint32_t c = 0;
	for (int i = 0; i < size; i++)
		if (i)
			c++;
	return c;
}
