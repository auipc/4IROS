#pragma once
#include <kernel/stdint.h>

struct GDTPointer {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

struct GDTEntry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	struct {
		uint8_t accessed : 1;
		uint8_t rw : 1;
		uint8_t dc : 1;
		uint8_t ex : 1;
		uint8_t type : 1;
		uint8_t priv : 2;
		uint8_t present : 1;
	} __attribute__((packed)) access;
	struct {
		uint8_t limit_high : 4;
		uint8_t zero : 2;
		uint8_t size : 1;
		uint8_t granularity : 1;
	} __attribute__((packed)) flags;
	uint8_t base_high;
} __attribute__((packed));

class GDT {
  public:
	static void setup();
};