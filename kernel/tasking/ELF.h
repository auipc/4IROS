#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/mem/Paging.h>
#include <kernel/util/Vec.h>
#include <kernel/vfs/vfs.h>
#include <stdint.h>

enum ELFArch : uint16_t { Unspecified = 0, x86 = 0x03, Arm = 0x28 };

struct ELFHeader32 {
	char magic[4];
	uint8_t subarch;
	uint8_t endian;
	uint8_t version;
	uint8_t abi;
	char unused[8];
	uint16_t bintype;
	ELFArch isa;
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
	ELF(VFSNode *exec);
	~ELF();

	inline uintptr_t program_entry() { return m_elf_header.entry; }
	int load_sections(PageDirectory *pd);

  private:
	enum SegmentFlags { EXEC = 1, WRITE = 2, READ = 4 };

	void parse();
	char *m_buffer = nullptr;
	ElfHeader m_elf_header;
	VFSNode *m_exec_fd = nullptr;
	ELFSectionHeader *m_headers = nullptr;
};
