#pragma once
#include <kernel/assert.h>
#include <kernel/mem/malloc.h>
#include <kernel/string.h>

template <typename T> class Vec {
  public:
	Vec() {}

	~Vec() {
		if (container)
			kfree(container);

		m_size = 0;
	}

	// the hard way
	T &operator[](size_t idx) {
		// ASSERT(idx < size);
		T *t = get(idx);
		assert(t);
		return *t;
	}

	// soft way to access stuff
	T *get(size_t idx) {
		if (idx >= m_size)
			return nullptr;
		return &container[idx];
	}

	inline size_t size() { return m_size; }

	void push(T entry) {
		m_size++;
		if (container) {
			// container = kmalloc(sizeof(T));
			T *tmp = reinterpret_cast<T *>(kmalloc(sizeof(T) * m_size));
			memcpy(tmp, container, sizeof(T) * m_size);
			kfree(container);
			container = tmp;
		} else {
			container = reinterpret_cast<T *>(kmalloc(sizeof(T)));
		}

		container[m_size - 1] = entry;
	}

	void remove(size_t pos) {
		assert(container);

		T *tmp = reinterpret_cast<T *>(kmalloc(sizeof(T) * m_size - 1));

		uint32_t new_pos = 0;
		for (uint32_t i = 0; i < m_size; i++) {
			if (i != pos) {
				memcpy(&tmp[new_pos++], &container[i], sizeof(T));
			}
		}

		kfree(container);
		container = tmp;

		m_size--;
	}

	template <typename F> void iterator(F func) {
		for (size_t i = 0; i < m_size; i++) {
			func(container[i]);
		}
	}

  private:
	T *container = nullptr;
	// Correct?
	size_t m_size = 0;
};
