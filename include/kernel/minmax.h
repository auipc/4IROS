#pragma once 

template <class T> constexpr T min(const T &a, const T &b) {
	return a < b ? a : b;
}

template <class T> constexpr T max(const T &a, const T &b) {
	return a > b ? a : b;
}
