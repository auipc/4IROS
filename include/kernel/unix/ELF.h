#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/mem/Paging.h>
#include <kernel/util/Vec.h>
#ifndef BOOT_STUB
#include <kernel/vfs/vfs.h>
#endif
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
} PACKED;

struct ELFHeader64 {
	char magic[4];
	uint8_t subarch;
	uint8_t endian;
	uint8_t version;
	uint8_t abi;
	char unused[8];
	uint16_t bintype;
	ELFArch isa;
	uint32_t ver;
	uint64_t entry;
	uint64_t phtable;
	uint64_t shtable;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
} PACKED;

struct ELFSectionHeader64 {
	uint32_t seg;
	uint32_t flags;
	uint64_t off;
	uint64_t vaddr;
	uint64_t paddr;
	uint64_t filesz;
	uint64_t memsz;
	uint64_t alignment;
} PACKED;

#if defined(__i386__)
typedef ELFHeader32 ElfHeader;
typedef ELFSectionHeader32 ELFSectionHeader;
#elif defined(__x86_64__)
typedef ELFHeader64 ElfHeader;
typedef ELFSectionHeader64 ELFSectionHeader;
#else
#error Unsupported architecture
#endif
