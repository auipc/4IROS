#pragma once
#include <kernel/util/Bitmap.h>

template <typename T> class Pool {
  public:
	Pool(size_t sz) {
		m_bitmap = new Bitmap(sz);
		m_values = new T[sz];
	}

	~Pool() {
		delete[] m_bitmap;
		delete[] m_values;
	}

	T *get() {
		int pos = m_bitmap->scan(1);
		printk("%x\n", m_bitmap->count_unset());
		return &m_values[pos];
	}

	void release(T *ptr) {
		m_bitmap->unset(((uintptr_t)(ptr - m_values)) / sizeof(T));
	}

  private:
	Bitmap *m_bitmap;
	T *m_values = nullptr;
};
