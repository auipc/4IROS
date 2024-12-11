#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if 0
typedef struct _BTreeNode {
	struct BTreeNode* left;
	struct BTreeNode* right;
} BTreeNode;

typedef struct _BTree {
	BTreeNode root;
} BTree;

static uintptr_t addr_base = 0;
static uintptr_t addr = 0;
static size_t addr_sz = 0x10000;

void *malloc(size_t size) {
	if (!addr) {
		addr_base = addr = (uintptr_t)mmap((void *)0x590000, 0x10000);
		printf("MMAP\n");
	}

	while ((addr+size) > (addr_base+addr_sz)) {
		printf("Allocating more mem\n");
		(void)mmap((void*)(addr_base+addr_sz), 0x10000);
		addr_sz += 0x10000;
	}

	void *p = (void *)addr;
	addr += size;
	return p;
}

void free(void* addr) {
	(void)addr;
}
#endif
