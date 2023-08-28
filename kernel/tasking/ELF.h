#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/stdint.h>
#include <kernel/util/Vec.h>
#include <kernel/mem/Paging.h>

struct ELFHeader32 {
	char magic[4];
	uint8_t subarch;
	uint8_t endian;
	uint8_t version;
	uint8_t abi;
	char unused[8];
	uint16_t bintype;
	uint16_t isa;
	uint32_t ver;
	uint32_t entry;
	uint32_t phtable;
	uint32_t shtable;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
} PACKED;

struct ELFSectionHeader32 {
	uint32_t seg;
	uint32_t off;
	uint32_t vaddr;
	uint32_t abimisc;
	uint32_t filesz;
	uint32_t memsz;
	uint32_t flags;
	uint32_t alignment;
};

#if defined(__i386__)
typedef ELFHeader32 ElfHeader;
typedef ELFSectionHeader32 ELFSectionHeader;
#else
#error Unsupported architecture
#endif

class ELF {
public:
	ELF(char* buffer, size_t buffer_length);
	~ELF();

	uintptr_t program_entry();
	void load_sections(PageDirectory* pd);
private:
	void parse();
	ElfHeader m_elf_header;
	char* m_buffer;
	size_t m_buffer_length;
	Vec<ELFSectionHeader> m_headers;
};
