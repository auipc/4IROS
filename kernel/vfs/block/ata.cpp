#include <kernel/arch/x86_common/IO.h>
#include <kernel/minmax.h>
#include <kernel/printk.h>
#include <kernel/vfs/block/ata.h>
#include <kernel/minmax.h>

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
	//assert(!(begin % 2));
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

int ATABlockNode::read_block(uint16_t *buffer) {
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
	while (blocks_remaining--) {
		*(buffer++) = inw(DATA_PORT);
	}
	return 0;
}

int ATABlockNode::write_512(uint8_t* buffer, size_t) {
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
	outb(COMMAND_PORT, 0x30);

	for (int i = 0; i < 256; i++) {
		outw(DATA_PORT, (buffer[(i*2)+1]<<8) | buffer[i*2]);
		outb(COMMAND_PORT, 0xE7);
		uint8_t status;
		do {
			status = inb(STATUS_PORT);
		} while (status & 0x80);
	}
	m_position += 512;
	return 0;
}

int ATABlockNode::write(void *buffer, size_t size) {
	uint8_t overwrite_buf[512];
	size_t old_pos = m_position;
	size_t size_rem = size;
	size_t sub_pos = m_position%512;
	size_t output_pos = 0;

	while (size_rem) {
		size_t read_sz = min(min((size_t)512, size_rem), (size_t)(512-sub_pos));
		read_block((uint16_t*)overwrite_buf);
		memcpy(&overwrite_buf[sub_pos], ((uint8_t*)buffer)+output_pos, read_sz);

		write_512(overwrite_buf, 512);
		
		output_pos += read_sz;
		size_rem -= read_sz;
		sub_pos = 0;
	}

	m_position = old_pos+size;
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

	size_t old_pos = m_position;
	size_t left = size;
	size_t off = 0;
	while(left) {
		read_512_temp((uint8_t *)((size_t)buffer+off), min(left,(size_t)512));
		off += 512;
		left -= min(left, (size_t)512);
	}
	m_position = old_pos;
	m_position += size;

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
