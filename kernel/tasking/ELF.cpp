#include <kernel/tasking/ELF.h>
#include <kernel/string.h>
#include <kernel/printk.h>

ELF::ELF(char* buffer, size_t buffer_length) 
	: m_buffer(buffer)
	, m_buffer_length(buffer_length)
{
	parse();
}

ELF::~ELF() {
}

uintptr_t ELF::program_entry() {
	return m_elf_header.entry;
}

void ELF::load_sections(PageDirectory* pd) {
	m_headers.iterator([&](ELFSectionHeader header) {
		if (header.seg != 1) return;
		//pd->map_range(header.vaddr, header.memsz, true);
		//auto current_page_directory = Paging::current_page_directory();
		printk("vaddr %x off %x memsz %x\n", header.vaddr, header.off, header.memsz);
		auto physical_pages = pd->map_range(header.vaddr, header.memsz, true);
		for (size_t i = 0; i < physical_pages.size(); i++) {
			auto address = physical_pages[i];
			Paging::current_page_directory()->map_page(address, address, false);
			memcpy(reinterpret_cast<void*>(address), m_buffer + header.off, header.memsz);
		}
	});
}

void ELF::parse() {
	constexpr char elf_magic[4] = {0x7F, 'E', 'L', 'F'};
	memcpy(&m_elf_header, m_buffer, sizeof(ElfHeader));
	if(strncmp(m_elf_header.magic, elf_magic, 4) != 0) {
		printk("Bad header\n");
	}

	ELFSectionHeader* headers = reinterpret_cast<ELFSectionHeader*>(m_buffer + m_elf_header.phtable);
	for (int i = 0; i < m_elf_header.phnum; i++, headers++) {
		ELFSectionHeader header;
		memcpy(&header, headers, sizeof(ELFSectionHeader));
		m_headers.push(header);
	}
}
