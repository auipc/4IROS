#include <kernel/assert.h>
#include <kernel/util/NBitmap.h>
#include <string.h>

NBitmap::NBitmap(size_t elems) : m_elems(elems) {
	// FIXME: uint32_t containers WILL be better
	m_data = new nbitmap_container_t[elems];
	memset(reinterpret_cast<char *>(m_data), 0, elems);
	m_size = elems / (4 * sizeof(nbitmap_container_t));
}

NBitmap::~NBitmap() { delete[] m_data; }

void NBitmap::set(size_t i, uint8_t val) {
	if (i >= m_elems) {
		return;
	}
	int byteIndex = i / 4;
	int bitIndex = i % 4;
	unsigned char mask = (val & 3) << (bitIndex * 2);
	m_data[byteIndex] |= mask;
}

void NBitmap::unset(size_t i) {
	if (i >= m_elems) {
		return;
	}
	int byteIndex = i / 4;
	int bitIndex = i % 4;
	unsigned char mask = 3 << (bitIndex * 2);
	m_data[byteIndex] &= ~mask; // Set the bit to 0
}

uint8_t NBitmap::get(size_t i) const {
	if (i >= m_elems) {
		assert(false);
	}
	int byteIndex = i / 4;
	int bitIndex = i % 4;
	return (m_data[byteIndex] >> (bitIndex * 2)) & 3;
}

void NBitmap::group_free(uint32_t start) {
#if 0
	for (size_t i = 0; i < m_size; i++) {
		printk("%d", get(i));
	}
	printk("\n");
#endif
	uint8_t nib = get(start);
	if (nib == 1) {
		unset(start);
	} else if (nib == 2) {
		unset(start);
		uint32_t idx = start;
		do {
			nib = get(idx);
			unset(idx);
			idx++;
		} while ((nib != 2));

#if 0
		for (size_t i = 0; i < m_size; i++) {
			printk("%d", get(i));
		}
		printk("\n");
#endif

		printk("Free'd block span: %x-%x\n", start, idx);
		/*
		do {
			if (nib == 2 || nib == 1) {
				printk("%d\n", nib);
			}
			unset(idx);
			nib = get(idx++);
		} while (nib != 2);*/
	}
}

uint32_t NBitmap::scan(uint32_t nr) {
	uint32_t streak = 0;
	for (size_t i = 0; i < m_elems; i++) {
		if (!get(i)) {
			streak++;
		} else {
			streak = 0;
		}

		if (streak == nr) {
			if (nr == 1) {
				set(i, 1);
				return i;
			}

			for (size_t j = 0; j < streak; j++) {
				if (j == 0 || j == streak - 1)
					set(i - j, 2);
				else
					set(i - j, 1);
			}

			return i;
		}
	}

	printk("%x\n", m_elems);
	assert(false);
}

uint32_t NBitmap::scan_no_set(uint32_t nr) {
	uint32_t streak = 0;
	for (size_t i = 0; i < m_size; i++) {
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

uint32_t NBitmap::count() {
	uint32_t c = 0;
	for (size_t i = 0; i < m_size; i++)
		if (i)
			c++;
	return c;
}
