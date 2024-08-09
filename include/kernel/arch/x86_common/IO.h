#pragma once
#include <stdint.h>

inline __attribute__((always_inline)) void outb(uint16_t port, uint8_t value) {
	asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

inline __attribute__((always_inline)) void outw(uint16_t port, uint16_t value) {
	asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

inline __attribute__((always_inline)) uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

inline __attribute__((always_inline)) uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

inline void io_wait() { asm volatile("outb %%al, $0x80" : : "a"(0)); }
