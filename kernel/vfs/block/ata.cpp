#include <kernel/arch/i386/IO.h>
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

int ATABlockNode::read_512(uint16_t *buffer) {
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
	for (size_t i = 0; i < 256; i++) {
		uint16_t w = inw(DATA_PORT);
		buffer[i] = w;
	}
	m_position += 512;
	return 0;
}

int ATABlockNode::read(void *buffer, size_t size) {
	if (!size)
		return -1;

	printk("reading at position %x\n", m_position);
	size_t lba_sector_count = (size + 512 - 1) / 512;
	uint16_t *b = new uint16_t[256 * lba_sector_count];
	size_t old_pos = m_position;

	for (size_t i = 0; i < lba_sector_count; i++) {
		read_512(b + (256 * i));
	}

	m_position = old_pos;

	memcpy((char *)buffer,
		   &((uint8_t *)b)[m_position % (512 * lba_sector_count)], size);

	m_position += size;
	delete[] b;

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
