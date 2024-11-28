#define STUPID_BAD_UGLY_BAD_DESIGN_TURN_OFF_STRING_H
#define BOOT_STUB
#include <kernel/arch/amd64/kernel.h>
#include <kernel/arch/x86_common/IO.h>
#include <kernel/multiboot.h>
#include <kernel/unix/ELF.h>
#include <stddef.h>
#include <stdint.h>

#define VGA_REGISTER_1 0x3C4
#define VGA_REGISTER_2 0x3CE
#define VGA_REGISTER_3 0x3D4

void vga_init() {
	// Reading from port 0x3DA will set port 0x3C0 to it's index state.
	inb(0x3DA);
}

void vga_write_misc_port(uint8_t byte) { outb(0x3C2, byte); }

void vga_write_fixed_port(uint8_t index, uint8_t byte) {
	outb(0x3C0, index);
	outb(0x3C0, byte);
}

void vga_write_index_port(uint16_t port, uint8_t index, uint8_t byte) {
	outb(port, index);
	outb(port + 1, byte);
}

extern const char _binary_4IROS_start[];
extern size_t _binary_4IROS_size;

extern "C" void memset(char *dest, int value, size_t size) {
	for (size_t i = 0; i < size; i++) {
		((char *)dest)[i] = value;
	}
}

extern "C" void *memcpy(void *dest, const void *src, size_t size) {
	for (size_t i = 0; i < size; i++) {
		((char *)dest)[i] = ((const char *)src)[i];
	}
	return nullptr;
}

extern "C" uint64_t pdpte0[512];
extern "C" uint64_t pdbad[512];
extern "C" uint64_t ptbad[512];

[[noreturn]] extern "C" void kx86_64_start(uint32_t multiboot_header,
										   multiboot_info *boot_head) {
	(void)multiboot_header;

	const ELFHeader64 *ehead =
		reinterpret_cast<const ELFHeader64 *>(_binary_4IROS_start);
	const ELFSectionHeader64 *esecheads =
		reinterpret_cast<const ELFSectionHeader64 *>(_binary_4IROS_start +
													 ehead->phtable);
	size_t kend = 0;

	for (int i = 0; i < ehead->phnum; i++) {
		const auto sechead = esecheads[i];
		if (sechead.seg != 1)
			continue;
		uint64_t off = 0x20000 + sechead.off;
		uint64_t flags = 1;
		// Disable execution if no ELF executable flag
		flags |= ((uint64_t) !(sechead.flags & 1ull)) << 63ull;
		// Writable? Ideally, panic if both eXecute and Writable are enabled.
		flags |= (!!(sechead.flags & 2ull)) << 1ull;

		for (uint64_t base = off; base < off + sechead.memsz; base += 0x1000) {
			uint64_t vaddr = (base - off) + sechead.vaddr;
			pdpte0[(vaddr >> 30) & 0x1ff] = ((uint64_t)pdbad) | 1;
			pdbad[(vaddr >> 21) & 0x1ff] = ((uint64_t)ptbad) | 1;
			ptbad[(vaddr >> 12) & 0x1ff] = base | flags;
		}

		uint64_t totalsz = sechead.off + sechead.memsz;
		kend = totalsz > kend ? totalsz : kend;
		memset((char *)off, 0, sechead.memsz);
		memcpy((char *)off, (char *)_binary_4IROS_start + sechead.off,
			   sechead.filesz);
		// off += sechead.memsz;
	}

	uint64_t cr3;
	asm volatile("mov %%cr3, %%rax" : "=a"(cr3));
	asm volatile("mov %%rax, %%cr3" ::"a"(cr3));
	typedef void (*pentryt)(const KernelBootInfo &, multiboot_info *);
	KernelBootInfo kbootinfo = {0x20000, kend};

	pentryt pentry = (pentryt)ehead->entry;

	pentry(kbootinfo, boot_head);
	/*
	vga_init();
	// Mode Control
	vga_write_fixed_port(0x10, 0x41);
	// Horizontal Panning
	vga_write_fixed_port(0x13, 0x0);
	// Misc output
	// Clock Mode
	vga_write_index_port(VGA_REGISTER_1, 0x01, 0x01);
	// Memory Mode
	vga_write_index_port(VGA_REGISTER_1, 0x04, 0x0E);
	// Mode Register
	vga_write_index_port(VGA_REGISTER_2, 0x05, 0x40);
	// Misc Register
	vga_write_index_port(VGA_REGISTER_2, 0x06, 0x05);
	vga_write_index_port(VGA_REGISTER_3, 0x0, 0x5F);

	vga_write_index_port(VGA_REGISTER_3, 0x01, 0x4F);
	vga_write_index_port(VGA_REGISTER_3, 0x02, 0x50);
	vga_write_index_port(VGA_REGISTER_3, 0x03, 0x82);
	vga_write_index_port(VGA_REGISTER_3, 0x04, 0x54);
	vga_write_index_port(VGA_REGISTER_3, 0x05, 0x80);
	vga_write_index_port(VGA_REGISTER_3, 0x06, 0xBF);
	vga_write_index_port(VGA_REGISTER_3, 0x07, 0x1F);
	vga_write_index_port(VGA_REGISTER_3, 0x09, 0x41);
	vga_write_index_port(VGA_REGISTER_3, 0x14, 0x40);
	vga_write_index_port(VGA_REGISTER_3, 0x17, 0xA3);
	while(1) {
		for (uint8_t* p = (uint8_t*)0x000A0000; p < (uint8_t*)(0x000BFFFF); p++)
	{ uint8_t lol = 255;
			//asm volatile("rdrand %%eax":"=a"(lol));
			*p = lol;
		}
	}
	*/
	while (1)
		;
}
