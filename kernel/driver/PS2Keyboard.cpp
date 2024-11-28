#include <kernel/arch/x86_common/IO.h>
#include <kernel/driver/PS2Keyboard.h>

char *PS2Keyboard::s_buffer = nullptr;
size_t PS2Keyboard::s_buffer_pos = 0;
size_t PS2Keyboard::s_writer_pos = 0;
bool PS2Keyboard::s_initialized = false;

PS2Keyboard::PS2Keyboard() : VFSNode("keyboard") {
	s_buffer = new char[KEYBD_BUFFER_SZ];
}

PS2Keyboard::~PS2Keyboard() { delete[] s_buffer; }

[[maybe_unused]] static void write_ps2_port(uint8_t port, uint8_t data) {
	if (port == 0) {
		outb(0x64, 0xD4);
		outb(0x60, data);
		while (inb(0x64) & (1 << 1))
			;
		outb(0x60, data);
	} else {
		while (inb(0x64) & (1 << 1))
			;
		outb(0x60, data);
	}
}

#if 0
static bool is_port_keyboard(uint8_t port) {
	// Disable scanning
	write_ps2_port(port, 0xF5);
	// There's no way to reliably check which PS/2 device is sending the bytes,
	// but it shouldn't matter.
	while (inb(0x64) != 0xFA)
		;
	write_ps2_port(port, 0xF2);
	printk("%x\n", inb(0x64));
	while (inb(0x64) != 0xFA)
		;

	// FIXME: Devices can send 1 byte but I'm too lazy to add a timeout.
	uint8_t b1 = inb(0x60);
	uint8_t b2 = inb(0x60);

	printk("%x %x\n", b1, b2);
	if (b1 == 0xAB && b2 == 0x41) {
		return true;
	}

	// Enable scanning
	write_ps2_port(port, 0xF4);
	return false;
}
#endif

// FIXME UHHHHHHHHHHHHHHHHH
extern "C" void write_eoi();

extern "C" void ps2kbd_interrupt() {
	uint8_t scan = inb(0x60);
	if (!PS2Keyboard::s_initialized) {
		printk("ps2 b4 init\n");
		write_eoi();
		return;
	}
	bool released = scan & 0x80;
	PS2Keyboard::s_buffer[PS2Keyboard::s_writer_pos++] = released;
	PS2Keyboard::s_buffer[PS2Keyboard::s_writer_pos %
						  PS2Keyboard::KEYBD_BUFFER_SZ] = scan & ~0x80;
	PS2Keyboard::s_writer_pos++;
	write_eoi();
}

void PS2Keyboard::init() {
	// Clear any lingering bytes.
	uint32_t timeout = 0;
	do {
		(void)inb(0x60);
	} while (timeout++ < 300);

	// FIXME hmmm
	s_initialized = true;
	VFSNode::init();
}

bool PS2Keyboard::check_blocked() { return (m_position) >= s_writer_pos; }

int PS2Keyboard::read(void *buffer, size_t size) {
	if (!s_initialized)
		return -1;

	size_t size_left = s_writer_pos - m_position;
	size_t size_to_read = (size > size_left) ? size_left : size;

	size_t read_at = (m_position % KEYBD_BUFFER_SZ) + size_to_read;

	// There's a better way to do this
	size_t circular_remainder =
		read_at >= KEYBD_BUFFER_SZ ? (read_at) % KEYBD_BUFFER_SZ : 0;
	memcpy(buffer, s_buffer + m_position, size_to_read - circular_remainder);
	if (circular_remainder)
		memcpy(buffer, s_buffer + m_position, circular_remainder);

	m_position += size_to_read;
	return size_to_read;
}
