#include <kernel/assert.h>
#include <kernel/string.h>
#include <kernel/util/Bitmap.h>

Bitmap::Bitmap(size_t elems) {
	// FIXME: uint32_t containers WILL be better
	data = new bitmap_container_t[elems];
	memset(reinterpret_cast<char *>(data), 0, elems);
	size = elems / (8 * sizeof(bitmap_container_t));
}

Bitmap::~Bitmap() { delete[] data; }

size_t Bitmap::alloc_size(size_t elems) {
	size_t a = sizeof(Bitmap);
	a += sizeof(bitmap_container_t) * elems;
	return a;
}

void Bitmap::set(size_t i) {
	if (i >= size) {
		return;
	}
	int byteIndex = i / 8;
	int bitIndex = i % 8;
	unsigned char mask = 1 << bitIndex;
	data[byteIndex] |= mask; // Set the bit to 1
}

void Bitmap::unset(size_t i) {
	if (i >= size) {
		return;
	}
	int byteIndex = i / 8;
	int bitIndex = i % 8;
	unsigned char mask = 1 << bitIndex;
	data[byteIndex] &= ~mask; // Set the bit to 0
}

uint8_t Bitmap::get(size_t i) const {
	if (i >= size) {
		assert(false);
	}
	int byteIndex = i / 8;
	int bitIndex = i % 8;
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
			for (size_t j = 0; j < streak; j++)
				set(i + j);

			return i;
		}
	}
	assert(false);
}

uint32_t Bitmap::scan_no_set(uint32_t nr) {
	uint32_t streak = 0;
	for (size_t i = 0; i < size; i++) {
		if (!get(i)) {
			streak++;
		} else {
			streak = 0;
		}

		if (streak == nr) {
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
