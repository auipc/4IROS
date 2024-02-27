#pragma once

class String {
public:
	String(const char* str) {
		m_data = str;
	}

	~String() {
		delete m_data;
	}

	const char* raw() {
		return m_data;
	}
private:
	const char* m_data;
	size_t m_size;
};
