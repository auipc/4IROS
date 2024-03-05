#include <kernel/arch/i386/kernel.h>
#include <kernel/printk.h>
#include <kernel/tasking/ELF.h>
#include <string.h>

ELF::ELF(char *buffer, size_t buffer_length)
	: m_buffer(buffer), m_buffer_length(buffer_length) {
	parse();
}

ELF::~ELF() {}

int ELF::load_sections(PageDirectory *pd) {
	int error = 0;
	m_headers.iterator([&](ELFSectionHeader header) {
		if (header.seg != 1)
			return BranchFlags::Continue;

		// Outright refuse to load executables that have an alignment
		// where 2 sections will share pages. There should be no condition
		// where LOADs are split other than having differing
		// permissions, so this should be a no-brainer!
		if (header.alignment < 0x1000) {
			error = 1;
			return BranchFlags::Break;
		}

		bool is_writable = (header.flags & ELF::SegmentFlags::WRITE) != 0;

		auto current_page_directory = Paging::current_page_directory();

		int map_size = header.memsz;
		pd->map_range(header.vaddr, map_size, PageFlags::USER);

		printk("Loading %x\n", header.vaddr);
		asm volatile(
			"mov %%eax, %%cr3" ::"a"(Paging::get_physical_address(pd)));
		memset(reinterpret_cast<char *>(header.vaddr), 0, header.memsz);
		if (header.filesz > 0)
			memcpy(reinterpret_cast<void *>(header.vaddr),
				   m_buffer + header.off, header.filesz);
		asm volatile("mov %%eax, %%cr3" ::"a"(
			Paging::get_physical_address(current_page_directory)));

		// Remap with permissions specified by ELF and with the page userspace
		// accessible
		int flags = PageFlags::USER;

		if (!is_writable)
			flags |= PageFlags::READONLY;

		pd->map_range(header.vaddr, header.filesz, flags);

		return BranchFlags::Continue;
	});
	return error;
}

void ELF::parse() {
	static constexpr char elf_magic[4] = {0x7F, 'E', 'L', 'F'};
	memcpy(&m_elf_header, m_buffer, sizeof(ElfHeader));
	if (strncmp(m_elf_header.magic, elf_magic, 4) != 0) {
		printk("Bad header\n");
	}

	ELFSectionHeader *headers =
		reinterpret_cast<ELFSectionHeader *>(m_buffer + m_elf_header.phtable);
	for (int i = 0; i < m_elf_header.phnum; i++, headers++) {
		ELFSectionHeader header;
		memcpy(&header, headers, sizeof(ELFSectionHeader));
		m_headers.push(header);
	}
}
