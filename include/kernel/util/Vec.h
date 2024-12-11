#pragma once
#include <kernel/assert.h>
#include <kernel/mem/malloc.h>
#include <string.h>

enum BranchFlags { Continue = 0, Break = 1 };

template <typename T> class Vec {
  public:
	Vec() {}

	~Vec() {
		if (container)
			kfree(container);

		container = nullptr;
		m_size = 0;
	}

	Vec(Vec &c) {
		m_size = c.m_size;
		if (m_size > 0) {
			container = reinterpret_cast<T *>(kmalloc(sizeof(T) * c.m_size));
			memcpy(container, c.container, sizeof(T) * c.m_size);
		}
	}

	// the hard way
	T &operator[](size_t idx) {
		T *t = get(idx);
		assert(t);
		return *t;
	}

	// soft way to access stuff (no)
	T *get(size_t idx) {
		if (idx >= m_size)
			return nullptr;
		return &container[idx];
	}

	T *data() { return container; }

	T *begin() {
		return &container[0];
	}

	T *end() {
		return &container[m_size-1];
	}

	inline size_t size() { return m_size; }

	void push(T entry) {
		m_size++;
		if (container) {
			// container = kmalloc(sizeof(T));
			T *tmp = new T[m_size];
			memcpy(tmp, container, sizeof(T) * m_size);
			if (container)
				kfree(container);
			container = tmp;
		} else {
			container = new T[m_size];
		}

		container[m_size - 1] = entry;
	}

	void remove(size_t pos) {
		assert(container);

		T *tmp = nullptr;

		if (m_size - 1 != 0) {
			tmp = new T[m_size - 1];

			uint32_t new_pos = 0;
			for (uint32_t i = 0; i < m_size; i++) {
				if (i != pos) {
					memcpy(&tmp[new_pos++], &container[i], sizeof(T));
				}
			}
		}

		if (container)
			kfree(container);

		if (m_size - 1 != 0)
			container = tmp;

		m_size--;
	}

	template <typename F> void iterator(F func) {
		for (size_t i = 0; i < m_size; i++) {
			// Isn't this just a for loop with extra steps?
			BranchFlags flag = func(container[i]);
			if (flag == BranchFlags::Break)
				break;
		}
	}

	void clear() {
		m_size = 0;
		delete container;
	}

  private:
	T *container = nullptr;
	// Correct?
	size_t m_size = 0;
};
