#include <kernel/gdt.h>
#include <kernel/string.h>

static GDTEntry gdt[6];
static GDTPointer gdt_ptr;
TSS tss_entry;

void GDT::setup() {
	/* FIXME This relies on identity mapping being set up. So a page fault
	 * involving a bad page directory will make it so we are unable to diagnose
	 * the problem. So we could either just use the physical addresses (which
	 * would require us to switch page maps during scheduling which might be
	 * better because it avoids certain page privilege escalation attacks) Or we
	 * could just make our page fault handler use the boot page directory. For
	 * the idiots (me) that don't understand what I'm talking about: When
	 * gdt_ptr is loaded, the CPU will try to read the GDT from a virtual
	 * address like 0xc01050f0 which resolves to 0x1050f0. But if the page
	 * directory is invalid, then the CPU will fault.
	 *
	 * Now that I think about it, the IDT is also reliant on this. So the
	 * hardcoded page directory is probably the best solution.
	 */

	memset(reinterpret_cast<char *>(gdt), 0, sizeof(gdt));

	// null descriptor
	gdt[0].limit_low = 0;
	gdt[0].base_low = 0;
	gdt[0].access.accessed = 0;
	gdt[0].access.rw = 0;
	gdt[0].access.dc = 0;
	gdt[0].access.ex = 0;
	gdt[0].access.type = 0;
	gdt[0].access.priv = 0;
	gdt[0].access.present = 0;
	gdt[0].flags.limit_high = 0;
	gdt[0].flags.zero = 0;

	// kernel code descriptor
	gdt[1].limit_low = 0xFFFF;
	gdt[1].base_low = 0;
	gdt[1].access.accessed = 0;
	gdt[1].access.rw = 1;
	gdt[1].access.dc = 0;
	gdt[1].access.ex = 1;
	gdt[1].access.type = 1;
	gdt[1].access.priv = 0;
	gdt[1].access.present = 1;
	gdt[1].flags.limit_high = 0xF;
	gdt[1].flags.zero = 0;
	gdt[1].flags.size = 1;
	gdt[1].flags.granularity = 1;
	gdt[1].base_high = 0;

	// kernel data descriptor
	gdt[2].limit_low = 0xFFFF;
	gdt[2].base_low = 0;
	gdt[2].access.accessed = 0;
	gdt[2].access.rw = 1;
	gdt[2].access.dc = 0;
	gdt[2].access.ex = 0;
	gdt[2].access.type = 1;
	gdt[2].access.priv = 0;
	gdt[2].access.present = 1;
	gdt[2].flags.limit_high = 0xF;
	gdt[2].flags.zero = 0;
	gdt[2].flags.size = 1;
	gdt[2].flags.granularity = 1;
	gdt[2].base_high = 0;

	// user code descriptor
	gdt[3].limit_low = 0xFFFF;
	gdt[3].base_low = 0;
	gdt[3].access.accessed = 0;
	gdt[3].access.rw = 1;
	gdt[3].access.dc = 0;
	gdt[3].access.ex = 1;
	gdt[3].access.type = 1;
	gdt[3].access.priv = 3;
	gdt[3].access.present = 1;
	gdt[3].flags.limit_high = 0xF;
	gdt[3].flags.zero = 0;
	gdt[3].flags.size = 1;
	gdt[3].flags.granularity = 1;
	gdt[3].base_high = 0;

	// user data descriptor
	gdt[4].limit_low = 0xFFFF;
	gdt[4].base_low = 0;
	gdt[4].access.accessed = 0;
	gdt[4].access.rw = 1;
	gdt[4].access.dc = 0;
	gdt[4].access.ex = 0;
	gdt[4].access.type = 1;
	gdt[4].access.priv = 3;
	gdt[4].access.present = 1;
	gdt[4].flags.limit_high = 0xF;
	gdt[4].flags.zero = 0;
	gdt[4].flags.size = 1;
	gdt[4].flags.granularity = 1;
	gdt[4].base_high = 0;

	uintptr_t tss_base = reinterpret_cast<uintptr_t>(&tss_entry);
	uint32_t tss_limit = sizeof(TSS);
	// tss descriptor
	gdt[5].limit_low = tss_limit;
	gdt[5].base_low = tss_base;
	gdt[5].access.accessed = 1;
	gdt[5].access.rw = 0;
	gdt[5].access.dc = 0;
	gdt[5].access.ex = 1;
	gdt[5].access.type = 0;
	gdt[5].access.priv = 0;
	gdt[5].access.present = 1;
	gdt[5].flags.limit_high = (tss_limit & (0xf << 16)) >> 16;
	gdt[5].flags.zero = 0;
	gdt[5].flags.size = 1;
	gdt[5].flags.granularity = 1;
	gdt[5].base_high = ((tss_base & (0xff << 24)) >> 24);

	memset(reinterpret_cast<char *>(&tss_entry), 0, sizeof(TSS));
	tss_entry.ss0 = 0x10;

	gdt_ptr.limit = 6 * sizeof(GDTEntry);
	gdt_ptr.base = reinterpret_cast<uintptr_t>(&gdt);
	// gdt_ptr = {.limit = 6 * sizeof(GDTEntry) - 1, .base = &gdt};

	asm volatile("lgdt %0" : : "m"(gdt_ptr));
	asm volatile("ltr %w0" : : "q"(5 * 8));
}
