#include <kernel/arch/x86_common/IO.h>
#include <kernel/minmax.h>
#include <kernel/printk.h>
#include <kernel/vfs/block/ata.h>

#define DRIVE_SELECT_PORT 0x1F6
#define COMMAND_PORT 0x1F7
#define STATUS_PORT 0x1F7
#define DATA_PORT 0x1F0

ATABlockNode::ATABlockNode(const char *name, DriveConfig config)
	: VFSNode(name), m_config(config) {}

int ATABlockNode::open(Vec<const char *> path) {
	(void)path;
	return 0;
}

int ATABlockNode::read_512(uint8_t *buffer, size_t under) {
	size_t i = 0;
	size_t lba_pos = m_position / 512;
	outb(DRIVE_SELECT_PORT, m_config);
	outb(0x1F2, 0);
	outb(0x1F3, (lba_pos >> 24) & 0xFF);
	outb(0x1F4, 0); //(lba_pos>>32)&0xFF);
	outb(0x1F5, 0); //(lba_pos>>40)&0xFF);
	outb(0x1F2, 1);
	outb(0x1F3, lba_pos & 0xFF);
	outb(0x1F4, (lba_pos >> 8) & 0xFF);
	outb(0x1F5, (lba_pos >> 16) & 0xFF);
	outb(COMMAND_PORT, 0x24);

	while (!(inb(STATUS_PORT) & 0x8))
		;

	int blocks_remaining = 256;

	struct LeftRight {
		uint8_t left;
		uint8_t right;
	} PACKED;

	(void)under;
	size_t begin = 0; // max((size_t)0, m_position%512);
	size_t min_sz = min((size_t)512, (m_position % 512) + under);
	assert(!(begin % 2));
	LeftRight lr = {};
	for (i = begin; i < min_sz; i++) {
		if (!(i % 2)) {
			uint16_t word = inw(DATA_PORT);
			lr = *(LeftRight *)&word;
			buffer[i] = lr.left;
			blocks_remaining--;
			continue;
		}
		buffer[i] = lr.right;
	}

	while (blocks_remaining--) {
		(void)inw(DATA_PORT);
	}

	m_position += 512;
	return 0;
}

int ATABlockNode::read_512_temp(uint8_t *buffer, size_t under) {
	size_t i = 0;
	size_t lba_pos = m_position / 512;
	outb(DRIVE_SELECT_PORT, m_config);
	outb(0x1F2, 0);
	outb(0x1F3, (lba_pos >> 24) & 0xFF);
	outb(0x1F4, 0); //(lba_pos>>32)&0xFF);
	outb(0x1F5, 0); //(lba_pos>>40)&0xFF);
	outb(0x1F2, 1);
	outb(0x1F3, lba_pos & 0xFF);
	outb(0x1F4, (lba_pos >> 8) & 0xFF);
	outb(0x1F5, (lba_pos >> 16) & 0xFF);
	outb(COMMAND_PORT, 0x24);

	while (!(inb(STATUS_PORT) & 0x8))
		;

	int blocks_remaining = 256;

	struct LeftRight {
		uint8_t left;
		uint8_t right;
	} PACKED;

	(void)under;
	size_t begin = max((size_t)0, m_position % 512);
	size_t min_sz = min((size_t)512, begin + under);
	assert(!(begin % 2));
	LeftRight lr = {};

	for (i = 0; i < begin / 2; i++) {
		(void)inw(DATA_PORT);
		blocks_remaining--;
	}

	for (i = begin; i < min_sz; i++) {
		if (!(i % 2)) {
			uint16_t word = inw(DATA_PORT);
			lr = *(LeftRight *)&word;
			buffer[i - begin] = lr.left;
			blocks_remaining--;
			continue;
		}
		buffer[i - begin] = lr.right;
	}

	while (blocks_remaining--) {
		(void)inw(DATA_PORT);
	}

	m_position += 512;
	return 0;
}

int ATABlockNode::read(void *buffer, size_t size) {
	if (!size)
		return -1;

	// Disable interrupts when reading the disk for speed
	// very 70s
	bool disable_int = interrupts_enabled();
	if (disable_int)
		asm volatile("cli");

	size_t lba_sector_count = (size + 511) / 512;
	if (lba_sector_count == 1) {
		size_t old_pos = m_position;
		read_512_temp((uint8_t *)buffer, size);
		m_position = old_pos;
		m_position += size;
	} else {
		uint8_t *b = new uint8_t[512 * lba_sector_count];
		size_t old_pos = m_position;

		for (size_t i = 0; i < lba_sector_count; i++) {
			read_512(b + (512 * i), size);
		}

		m_position = old_pos;
		memcpy((char *)buffer, &b[m_position % (512)], size);
		m_position += size;
		delete[] b;
	}

	if (disable_int)
		asm volatile("sti");
	return 0;
}

void ATAManager::setup(VFSNode *parent) {
	outb(DRIVE_SELECT_PORT, 0xA0);
	for (int i = 0; i < 3; i++) {
		outb(0x1F2 + i, 0);
	}

	outb(COMMAND_PORT, 0xEC);
	uint8_t status = inb(STATUS_PORT);
	if (!status) {
		printk("No drives present.\n");
		return;
	}
	while (status & 0x80)
		status = inb(STATUS_PORT);

	uint16_t ident[256] = {};
	for (int i = 0; i < 256; i++) {
		ident[i] = inw(DATA_PORT);
	}

	if (!(ident[83] & 0x400)) {
		printk("Drive does not support mode LBA48");
		return;
	}

	auto b = new ATABlockNode("hd0", DriveConfig::Master);
	parent->push(b);
}
