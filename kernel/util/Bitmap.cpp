#include <kernel/assert.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/stdint.h>
#include <kernel/string.h>
#include <kernel/util/Bitmap.h>

Bitmap::Bitmap(size_t elems) {
	// uint32_t containers might be better
	data = new uint32_t[elems / 32];
	memset(reinterpret_cast<char *>(data), 0, elems / 32);
	size = elems / 32;
}

Bitmap::~Bitmap() { delete[] data; }

void Bitmap::set(size_t i) {
	if (i >= size) {
		assert(false);
		return;
	}
	int byteIndex = i / 32;
	int bitIndex = i % 32;
	unsigned char mask = 1 << bitIndex;
	data[byteIndex] |= mask; // Set the bit to 1
}

void Bitmap::unset(size_t i) {
	if (i >= size) {
		assert(false);
		return;
	}
	int byteIndex = i / 32;
	int bitIndex = i % 32;
	unsigned char mask = 1 << bitIndex;
	data[byteIndex] &= ~mask; // Set the bit to 0
}

uint8_t Bitmap::get(size_t i) const {
	if (i >= size) {
		assert(false);
	}
	int byteIndex = i / 32;
	int bitIndex = i % 32;
	return (data[byteIndex] >> bitIndex) & 1;
}

uint32_t Bitmap::scan(uint32_t nr) {
	uint32_t streak = 0;
	for (size_t i = 0; i < size; i++) {
		if (!get(i)) {
			streak++;
		} else {
			streak = 0;
		}

		if (streak == nr) {
			// FIXME does this work?
			for (size_t j = i; j > i - streak; j--)
				set(j);

			return i;
		}
	}
	assert(false);
}

uint32_t Bitmap::count() {
	uint32_t c = 0;
	for (size_t i = 0; i < size; i++)
		if (i)
			c++;
	return c;
}
