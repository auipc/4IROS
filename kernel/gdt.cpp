#include <kernel/gdt.h>

static GDTEntry gdt[6];
static GDTPointer gdt_ptr;

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

	// null descriptor
	gdt[0].limit_low = 0;
	gdt[0].base_low = 0;
	gdt[0].base_middle = 0;
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
	gdt[1].base_middle = 0;
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
	gdt[2].base_middle = 0;
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

	gdt_ptr = {.limit = 3 * sizeof(GDTEntry) - 1, .base = (uint32_t)&gdt};

	asm volatile("lgdt %0" : : "m"(gdt_ptr));
}