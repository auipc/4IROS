#pragma once
#include <stddef.h>

// Shared smart pointer class
template <class T>
class Smrt {
public:
	Smrt() {
		m_ptr = new T();
	}

	template <class... Args>
	Smrt(Args... a) {
		m_ptr = new T(a...);
	}

	~Smrt() {
		m_refs--;
		if (!m_refs) {
			delete m_ptr;
		}
	}
private:
	T* m_ptr;
	size_t m_refs;
};
