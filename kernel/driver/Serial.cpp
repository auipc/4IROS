#include <kernel/arch/x86_common/IO.h>
#include <kernel/driver/Serial.h>

#define PORT 0x3f8
// Yoinked from OSDev.org
static int init_serial() {
	outb(PORT + 1, 0x00); // Disable all interrupts
	outb(PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
	outb(PORT + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
	outb(PORT + 1, 0x00); //                  (hi byte)
	outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
	outb(PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
	outb(PORT + 4, 0x1E); // Set in loopback mode, test the serial chip
	outb(PORT + 0, 0xAE); // Test serial chip (send byte 0xAE and check if
						  // serial returns same byte)

	// Check if serial is faulty (i.e: not same byte as sent)
	if (inb(PORT + 0) != 0xAE) {
		return 1;
	}

	// If serial is not faulty set it in normal operation mode
	// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
	outb(PORT + 4, 0x0F);
	return 0;
}

static int is_transmit_empty() { return inb(PORT + 5) & 0x20; }
static int is_receive_empty() { return inb(PORT + 5) & 1; }

static void write_serial(char a) { outb(PORT, a); }

Serial::Serial() : VFSNode("serial") {}

void Serial::init() {
	// init_serial();
}

bool Serial::check_blocked() { return !is_receive_empty(); }

int Serial::read(void *buffer, size_t size) {
	assert(size == 1);
	for (size_t i = 0; i < size; i++)
		((char *)buffer)[i] = inb(PORT);

	return size;
}

bool Serial::check_blocked_write(size_t) { return !is_transmit_empty(); }

int Serial::write(void *buffer, size_t size) {
	// assert(size == 1);
	for (size_t i = 0; i < size; i++) {
		if (((char *)buffer)[i] == '\r')
			((char *)buffer)[i] = '\n';
		while (!is_transmit_empty())
			;
		write_serial(((char *)buffer)[i]);
	}

	return size;
}
