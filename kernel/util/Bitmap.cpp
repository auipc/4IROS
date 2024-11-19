#include <kernel/assert.h>
#include <kernel/util/Bitmap.h>
#include <string.h>

Bitmap::Bitmap(size_t bits) : m_containers((bits + 63) / s_bits_per_container) {
	m_data = new bitmap_container_t[m_containers];
}

Bitmap::~Bitmap() { delete[] m_data; }

void Bitmap::set(size_t i) { m_data[i / s_bits_per_container] |= 1ull << (i%s_bits_per_container); }

void Bitmap::unset(size_t i) {
	m_data[i / s_bits_per_container] &= ~(1ull << i);
}

uint8_t Bitmap::get(size_t i) const {
	return !!(m_data[i / s_bits_per_container] &
			  1ull << (i % s_bits_per_container));
}

size_t Bitmap::count_unset() const {
	size_t unset = 0;
	for (size_t i = 0; i < m_containers; i++) {
		unset += count_unset_container(i);
	}
	return unset;
}

// FIXME cache last location
size_t Bitmap::scan(const size_t span) {
	size_t streak = 0;
	for (size_t i = 0; i < m_containers; i++) {
		if (count_unset_container(i) < span)
			continue;
		for (size_t j = 0; j < s_bits_per_container; j++) {
			// slow
			if (!get(i * s_bits_per_container + j)) {
				streak++;
			}

			if (streak == span) {
				size_t base = i * s_bits_per_container + j - (streak - 1);
				for (size_t k = base; k < base + streak; k++) {
					set(k);
				}
				return base;
			}
		}
	}
	assert(false);
}

size_t Bitmap::scan_no_set(const size_t span) const {
	size_t streak = 0;
	for (size_t i = 0; i < m_containers; i++) {
		if (count_unset_container(i) < span)
			continue;
		for (size_t j = 0; j < s_bits_per_container; j++) {
			// slow
			if (!get(i * s_bits_per_container + j)) {
				streak++;
			}

			if (streak == span) {
				size_t base = i * s_bits_per_container + j - (streak - 1);
				return base;
			}
		}
	}
	assert(false);
}

/*
size_t Bitmap::scan_no_set(size_t span) const {
	(void)span;
	for (size_t i = 0; i < m_containers; i++) {
	}
	assert(false);
}*/

size_t Bitmap::count_unset_container(const size_t container) const {
	size_t unset = 0;
#ifdef OS_SSE4
	unset += 64ull - __builtin_popcountll(m_data[container]);
#else
	for (size_t i = 0; i < s_bits_per_container; i++) {
		unset += !(m_data[container] & 1ull << i);
	}
#endif
	return unset;
}
