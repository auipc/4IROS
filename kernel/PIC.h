#pragma once

#include <stdint.h>

class PIC {
  public:
	enum {
		PIC1_COMMAND = 0x20,
		PIC1_DATA = 0x21,
		PIC2_COMMAND = 0xA0,
		PIC2_DATA = 0xA1,
		PIC_EOI = 0x20,
	};

	enum {
		ICW1_ICW4 = 0x01,
		ICW1_SINGLE = 0x02,
		ICW1_INTERVAL4 = 0x04,
		ICW1_LEVEL = 0x08,
		ICW1_INIT = 0x10,
		ICW4_8086 = 0x01,
		ICW4_AUTO = 0x02,
		ICW4_BUF_SLAVE = 0x08,
		ICW4_BUF_MASTER = 0x0C,
		ICW4_SFNM = 0x10,

	};

	static void enable(uint8_t irq);
	static void setup();
	static void eoi(uint8_t irq);
};
