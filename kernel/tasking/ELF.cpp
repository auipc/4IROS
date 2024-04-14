#include <kernel/arch/i386/kernel.h>
#include <kernel/printk.h>
#include <kernel/tasking/ELF.h>
#include <string.h>

ELF::ELF(VFSNode *exec) : m_exec_fd(exec) { parse(); }

ELF::~ELF() {
	if (m_headers)
		delete[] m_headers;
}

int ELF::load_sections(PageDirectory *pd) {
	int error = 0;
	for (int i = 0; i < m_elf_header.phnum; i++) {
		auto header = m_headers[i];
		if (header.seg != 1)
			continue;

		// Refuse to map LOAD segments that have an alignment less than our
		// current page size. Apparently overlapping segments "should" be
		// supported, but you'd be hard pressed to find a case where a regular C
		// compiler or linker emits an ELF like that.
		if (header.alignment < PAGE_SIZE) {
			error = 1;
			break;
		}

		bool is_writable = (header.flags & ELF::SegmentFlags::WRITE) != 0;

		auto current_page_directory = Paging::current_page_directory();

		pd->map_range(header.vaddr, header.memsz + PAGE_SIZE, PageFlags::USER);

		asm volatile(
			"mov %%eax, %%cr3" ::"a"(Paging::get_physical_address(pd)));
		memset(reinterpret_cast<char *>(header.vaddr), 0, header.memsz);

		if (header.filesz > 0) {
			memcpy(reinterpret_cast<void *>(header.vaddr),
				   (m_buffer + header.off), header.filesz);
		}

		asm volatile("mov %%eax, %%cr3" ::"a"(
			Paging::get_physical_address(current_page_directory)));

		// Remap with the appropriate page flags requested by the ELF
		// executable.
		int flags = PageFlags::USER;

		if (!is_writable)
			flags |= PageFlags::READONLY;

		pd->map_range(header.vaddr, header.filesz, flags);
	}
	return error;
}

static constexpr char elf_magic[4] = {0x7F, 'E', 'L', 'F'};
void ELF::parse() {
	// Read entire ELF file
	m_buffer = new char[m_exec_fd->size()];
	m_exec_fd->read(m_buffer, m_exec_fd->size());

	memcpy(&m_elf_header, m_buffer, sizeof(ElfHeader));

	if (strncmp(m_elf_header.magic, elf_magic, 4) != 0) {
		printk("Bad header\n");
		return;
	}

#if defined(__i386__)
	if (m_elf_header.isa != ELFArch::x86) {
		printk("Bad arch\n");
		return;
	}

	if (m_elf_header.subarch != 1) {
		printk("Bad subarch\n");
		return;
	}
#endif

	m_headers = new ELFSectionHeader[m_elf_header.phnum];
	m_exec_fd->seek(m_elf_header.phtable);
	m_exec_fd->read(m_headers, sizeof(ELFSectionHeader) * m_elf_header.phnum);
}
