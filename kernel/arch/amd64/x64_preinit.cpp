//#define TEST_TIMER
#include <kernel/mem/malloc.h>
#include <kernel/arch/amd64/kernel.h>
#include <string.h>
#include <kernel/arch/x86_common/IO.h>
#include <kernel/arch/amd64/x86_64.h>
#include <kernel/arch/amd64/x64_sched_help.h>

struct IDTPTR {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

IDTPTR idtptr;

extern "C" void get_idt(IDTPTR*);

struct IDTGate {
	union {
		struct {
			uint16_t offset_lo;
			uint16_t segment;
			uint8_t ist : 3;
			uint8_t blank : 5;
			uint8_t type : 4;
			uint8_t blank_2 : 1;
			uint8_t dpl : 2;
			uint8_t present : 1;
			uint16_t offset_mid;
			uint32_t offset_hi;
			uint32_t reserved;
		};
		struct {
			uint32_t a;
			uint32_t b;
			uint32_t c;
			uint32_t d;
		};
	};
} __attribute__((packed));

static_assert(sizeof(IDTGate) == 16);

// The base addresses of the IDT should be aligned on an 8-byte boundary to maximize performance of cache line fills.
IDTGate idtgate[256] __attribute__((aligned(8)));

#define INTEXT(no) \
	extern "C" void interrupt_##no##_begin(); \

#define INT(no) \
	assert((uint64_t)&interrupt_##no##_begin <= BIT48_MAX); \
	idtgate[no].present = 1; \
	idtgate[no].segment = 0x8; \
	idtgate[no].type = (no < 0x10) ? 0xe : 0xf; \
	idtgate[no].offset_lo = (uint64_t)&interrupt_##no##_begin & 0xFFFF; \
	idtgate[no].offset_mid = ((uint64_t)&interrupt_##no##_begin>>16) & 0xFFFF; \
	idtgate[no].offset_hi = ((uint64_t)&interrupt_##no##_begin>>32) & 0xFFFF; \

INTEXT(0);
INTEXT(5);
INTEXT(6);
INTEXT(7);
INTEXT(8);
INTEXT(10);
INTEXT(11);
INTEXT(12);
INTEXT(13);
INTEXT(14);
INTEXT(16);
INTEXT(65);
INTEXT(85);

void interrupts_init() {
	//get_idt(&idtptr);
	idtptr.limit = 256*8;
	idtptr.base = (uint64_t)idtgate;

	/*uint64_t entry = (uint64_t)&interrupt_begin;
	for (int i = 0; i < 16; i++) {
			idtgate[i].present = 1;
			idtgate[i].segment = 0x8;
			idtgate[i].type = (i < 0x10) ? 0xe : 0xf;
			idtgate[i].offset_lo = entry & 0xFFFF;
			idtgate[i].offset_mid = (entry>>16) & 0xFFFF;
			idtgate[i].offset_hi = (entry>>32) & 0xFFFF;
	}*/

	INT(0);
	INT(5);
	INT(6);
	INT(7);
	INT(8);
	INT(10);
	INT(11);
	INT(12);
	INT(13);
	INT(14);
	INT(16);
	INT(65);
	INT(85);

	uint64_t ptr = (uint64_t)&idtptr;
	asm volatile("lidt (%%rax)"::"a"(ptr));
	//idta[0x40].present = 1;
	//idta[0x40].segment = 0x08;
	//idta[0x40].type = 0xe;

	//idta[0].offset_lo = 0xFF;
	//idta[0].offset_hi = 0xAAA;

	//idta[1] = (1ull<<46ull) | (0x08ull<<16ull) | ((0xFEFEFEFEull)&0xFFFFull);

	//idta[0] = 0xFEFEFEFEull & ~0xFFFFFFFFull;
}

static const char RSDP_MAGIC[] = "RSD PTR ";

struct RSDP {
	char signature[8];
	uint8_t checksum;
	char oem[6];
	uint8_t acpi_rev;
	uint32_t rsdt_addr;
	uint32_t length;
	uint64_t xsdt_addr;
	uint8_t ex_checksum;
	uint8_t reserved[3];
} __attribute__((packed));

struct RSDT {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem[6];
	char oem_id[8];
	char oem_rev[4];
	char creator_id[4];
	char creator_rev[4];
	uint32_t entries;
} __attribute__((packed));

struct DESCRIPTION_HEADER {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem[6];
	char oem_id[8];
	char oem_rev[4];
	char creator_id[4];
	char creator_rev[4];
} __attribute__((packed));

struct MADT {
	DESCRIPTION_HEADER header;
	uint32_t local_intc_addr;
	uint32_t flags;
} __attribute__((packed));

struct LocalAPIC {
	uint8_t acpi_processor_uid;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__((packed));

struct IOAPIC {
	uint8_t apic_id;
	uint8_t reserved;
	uint32_t apic_address;
	uint32_t int_base;
} __attribute__((packed));

struct GenericAddressStructure {
	uint8_t address_space_id;
	uint8_t register_bit_width;
	uint8_t register_bit_offset;
	uint8_t reserved;
	uint64_t address;
} __attribute__((packed));

static_assert(sizeof(GenericAddressStructure) == 12);

struct HPET {
	DESCRIPTION_HEADER header;
	uint32_t event_timer_id;
	GenericAddressStructure address;
	uint8_t hpet_number;
	uint16_t counter_min;
} __attribute__((packed));

uint64_t to_phys(uint64_t addr) {
	return (addr-0x280103000)+(0x20000+0x4000);
}

uint64_t to_virt(uint64_t addr) {
	return (addr+0x280103000)-(0x20000+0x4000);
}

#define VIRT_MASK(x,y) ((*(x + ((virt>>y)&0x1ffull)))&~4095ull)
#define VIRT_MASK_ADDR(x,y) (x + (((virt>>y))&0x1ffull))
//#define VIRT_MASK_ADDR(x,y) (x + (((virt>>y))&0x1ffull))

void simp_map_page(uint64_t virt, uint64_t phys) {
	printk("Map %x\n", virt);
	uint64_t* pml4 = 0;
	asm volatile("mov %%cr3, %%rax":"=a"(pml4):);
	uint64_t* pdpte = (uint64_t*)VIRT_MASK(pml4, 39);//(*(pml4 + ((virt>>39)&0x1ff))&~4095);
	if (!pdpte) {
		panic("PDPTE is null");
	}

	uint64_t* pd = (uint64_t*)VIRT_MASK(pdpte, 30);
	if (!pd) {
		panic("PD is null");
	}

	uint64_t* pt = (uint64_t*)VIRT_MASK_ADDR(pd, 21);
	if (!*pt) {
		uint64_t pt_addr = (uint64_t)kmalloc(sizeof(uint64_t)*512);
		// offset into pd
		*pt = to_phys(pt_addr);
		*pt |= 3;
	}

	uint64_t* pte = (uint64_t*)VIRT_MASK_ADDR((uint64_t*)to_virt(*pt & ~4095ull), 12);

	if (*pte != *((uint64_t*)VIRT_MASK_ADDR((*pt & ~4095ull), 12))) {
		panic("Mismatch");
	}


	*pte = phys&~4095;
	*pte |= 3;
	asm volatile("invlpg %0"::"m"(virt));
}

// https://wiki.osdev.org/RSDP#Detecting_the_RSDP
RSDP* get_rsdp_ptr() {
	RSDP* rsdp_ptr = nullptr;

	// Skip over 0x0
	for (uintptr_t addr = 16; addr < KB; addr+=16) {
		char* magic = (char*)addr;
		if(strncmp(magic, RSDP_MAGIC, 8) == 0) {
			rsdp_ptr = (RSDP*)addr;
		}
	}

	for (uintptr_t addr = 0x000E0000; addr < 0x000FFFFF; addr+=16) {
		char* magic = (char*)addr;
		if(strncmp(magic, RSDP_MAGIC, 8) == 0) {
			rsdp_ptr = (RSDP*)addr;
		}
	}
	
	if (!rsdp_ptr) panic("RSDP is null");
	return rsdp_ptr;
}

HPET* scan_acpi_hpet(RSDP* rsdp) {
	RSDT* rsdt = (RSDT*)(uint64_t)rsdp->rsdt_addr;
	uint32_t* rsdt_entries = (uint32_t*)(((uint8_t*)rsdt)+sizeof(RSDT));
	size_t n_entries = (rsdt->length - 40)/4;
	for (size_t i = 0; i < n_entries; i++) {
		DESCRIPTION_HEADER* header = (DESCRIPTION_HEADER*)(uint64_t)rsdt_entries[i];
		if (!strncmp(header->signature, "HPET", 4)) {
			printk("found HPET\n");
			HPET* hpet = (HPET*)header;
			return hpet;
		}
	}
	return nullptr;
}

uint64_t scan_acpi_apic(RSDP* rsdp) {
	RSDT* rsdt = (RSDT*)(uint64_t)rsdp->rsdt_addr;
	uint32_t* rsdt_entries = (uint32_t*)(((uint8_t*)rsdt)+sizeof(RSDT));
	size_t n_entries = (rsdt->length - 40)/4;
	for (size_t i = 0; i < n_entries; i++) {
		DESCRIPTION_HEADER* header = (DESCRIPTION_HEADER*)(uint64_t)rsdt_entries[i];
		if (!strncmp(header->signature, "APIC", 4)) {
			MADT* madt = (MADT*)header;
			uint8_t* madt_entries = ((uint8_t*)madt)+sizeof(MADT);
			size_t madt_n_entries = (rsdt->length - 44)/4;
			simp_map_page((uint64_t)madt->local_intc_addr, (uint64_t)madt->local_intc_addr);

			(void)madt_entries;
			(void)madt_n_entries;
			for (size_t i = 0; i < madt_n_entries; i++) {
				uint8_t type = *madt_entries;
				uint8_t length = *(madt_entries+1);
				switch (type) {
					case 0: {
						LocalAPIC* localapic = (LocalAPIC*)madt_entries+2;
						printk("kkkk %x\n", localapic->apic_id);
					} break;
					case 1: {
						//IOAPIC* ioapic = (IOAPIC*)madt_entries+2;
						printk("%x\n", length);
						/*simp_map_page((uint64_t)ioapic->apic_address, (uint64_t)ioapic->apic_address);

						uint32_t* lol = (uint32_t*)(uint64_t)ioapic->apic_address;
						*lol = 0x10;
						printk("%x\n", *(lol+0x10));
						printk("%x\n", *(lol+0x10+4));

						printk("apic ib %x\n", ioapic->int_base);*/
					} break;
					default:
						panic("Unknown MADT type");
						break;
				}
				madt_entries += length;
			}
			return (uint64_t)madt->local_intc_addr;
		}
		printk("%s\n", header->signature);
	}

	return 0;
}

// TODO This address is constant in Intel documentation, but it would be nice to get it from ACPI. 
static constexpr uint64_t IOAPIC_ADDR = 0xFEC00000ull;
static constexpr uint8_t IOAPIC_REGISTER_WINDOW = 0x10;
static constexpr uint8_t IOREDIR_BASE = 0x10;
[[maybe_unused]] static constexpr uint64_t IOREDIR_INTMASK_FLAG = 1ull<<16ull;

static uint8_t* local_apic = 0;

void ioapic_set_ioredir_val(uint8_t index, const uint64_t value) {
	index *= 2;
	*(uint32_t*)(IOAPIC_ADDR) = index+IOREDIR_BASE;
	*(uint32_t*)(IOAPIC_ADDR+IOAPIC_REGISTER_WINDOW) = value & (1ull<<32ull)-1ull;
	*(uint32_t*)(IOAPIC_ADDR) = index+1ull+IOREDIR_BASE;
	*(uint32_t*)(IOAPIC_ADDR+IOAPIC_REGISTER_WINDOW) = value>>32ull;
}

extern "C" void write_eoi() {
	uint32_t* eoi_register = (uint32_t*)(local_apic + 0xB0);
	*eoi_register = 0;
}

extern "C" void unhandled_interrupt() {
	panic("Unhandled Interrupt\n");
}

extern "C" void div_interrupt() {
	panic("Division Fault\n");
}

extern "C" void gpf_interrupt() {
	panic("General Protection Fault\n");
}

extern "C" void df_interrupt() {
	panic("Double Fault");
}

extern "C" void tss_interrupt() {
	panic("Invalid TSS");
}

extern "C" void pf_interrupt() {
	uint32_t fault_addr = get_cr2();
	printk("Unrecoverable kernel page fault while accessing address 0x%x\n", fault_addr);
	panic("Page Fault");
}

static uint64_t ms_elapsed = 0;
void sleep(const uint64_t ms_time) {
	uint64_t sleep_comp = 0;
	sleep_comp = ms_elapsed + ms_time;
	while(ms_elapsed < sleep_comp) {
		printk("Sleepwait\n");
	}
	return;
}

static uint64_t fp_per_tick = 0;
uint64_t tick_counter = 0;

extern "C" void keyboard_interrupt() {
	(void)inb(0x60);
#ifdef TEST_TIMER
	// Read comparator
	ms_elapsed = (tick_counter*0xB00B15*fp_per_tick)/1000000000000;

	tick_counter++;
	printk("Timer %x %x %x %x\n", tick_counter, fp_per_tick, tick_counter*fp_per_tick, ms_elapsed);
	uint64_t cr3;
	asm volatile("movq %%cr3, %0":"=a"(cr3));

	uint64_t* stupidstack = (uint64_t*)kmalloc_aligned(sizeof(uint64_t)*50, 8);
	uint64_t* stp = x64_create_task_stack(stupidstack+50);
	x64_switch((void*)(stp),cr3);
#else
	panic("Keyboard\n");
#endif
	write_eoi();
}

extern "C" void timer_interrupt(InterruptReg regs) {
	(void)regs;
#ifndef TEST_TIMER
	// Read comparator
	ms_elapsed = (tick_counter*0xB00B15*fp_per_tick)/1000000000000;

	tick_counter++;
	printk("Timer %x %x %x %x\n", tick_counter, fp_per_tick, tick_counter*fp_per_tick, ms_elapsed);
	uint64_t cr3;
	asm volatile("movq %%cr3, %0":"=a"(cr3));

	uint64_t* stupidstack = (uint64_t*)kmalloc_aligned(sizeof(uint64_t)*50, 8);
	uint64_t* stp = x64_create_task_stack(stupidstack+50);
	x64_switch((void*)(stp),cr3);
#endif 
	write_eoi();
}

[[noreturn]] extern "C" void kx86_64_preinit() {
	// Setup FPU
	uint64_t cr0, cr4;
	asm volatile("mov %%cr0, %%rax":"=a"(cr0):);
	cr0 |= (1<<1);
	//cr0 |= (1<<2);
	cr0 |= (1<<5);
	asm volatile("mov %%rax, %%cr0"::"a"(cr0));

	// Setup SSE
	asm volatile("mov %%cr4, %%rax":"=a"(cr4):);
	cr4 |= (1<<9);
	cr4 |= (1<<10);
	asm volatile("mov %%rax, %%cr4"::"a"(cr4));

	// enable the Local APIC
	// useless in qemu...
	asm volatile("mov $0x1B, %%ecx\n"
		     "rdmsr\n"
		     "orl $0x800, %%eax\n"
		     "wrmsr\n" ::: "eax", "ecx");

	screen_init();
	kmalloc_temp_init();
	interrupts_init();

	// Scan ACPI tables for relevant structures.
	RSDP* rsdp = get_rsdp_ptr();
	simp_map_page((uint64_t)rsdp->rsdt_addr, (uint64_t)rsdp->rsdt_addr);
	local_apic = (uint8_t*)scan_acpi_apic(rsdp);
	HPET* hpet = scan_acpi_hpet(rsdp);
	simp_map_page((uint64_t)hpet->address.address, (uint64_t)hpet->address.address);
	simp_map_page((uint64_t)IOAPIC_ADDR, (uint64_t)IOAPIC_ADDR);

	uint8_t* ioapic = (uint8_t*)IOAPIC_ADDR;
	
	// Disable all the entries in the LVT.
	// CMCI
	*(uint32_t*)(local_apic+0x2F0) |= 1<<12 | 1<<16 | 70;
	// LINT0
	*(uint32_t*)(local_apic+0x350) |= 1<<12 | 1<<16 | 70;
	// LINT1
	*(uint32_t*)(local_apic+0x360) |= 1<<12 | 1<<16 | 70;
	// Error
	*(uint32_t*)(local_apic+0x370) |= 1<<12 | 1<<16 | 70;
	// Perf counter 
	*(uint32_t*)(local_apic+0x340) |= 1<<12 | 1<<16 | 70;
	// Timer
	*(uint32_t*)(local_apic+0x320) |= 1<<12 | 1<<16 | 70;

	printk("? %x\n", *(uint32_t*)(ioapic+0x10));

	for (int i = 0; i < 24; i++) {
		if (i == 2) {
			ioapic_set_ioredir_val(i, 1<<16);
			continue;
		}

		ioapic_set_ioredir_val(i, 0x40+i);
	}
	/*ioapic_set_ioredir_val(1, 65);
	ioapic_set_ioredir_val(21, 85);*/

	printk("%d\n", (*(uint64_t*)(hpet->address.address)>>32));

	// Enable all HPETs
	*(uint64_t*)(hpet->address.address+0x10) |= 1;
	// Setup HPET0
	*(uint64_t*)(hpet->address.address+0x100) |= 21<<9 | 1<<2 | 1<<1 | 1<<3;

	fp_per_tick = *(uint64_t*)(hpet->address.address)>>32;

	// Comparator
	*(uint64_t*)(hpet->address.address+0x108) = 0xB00B15;

	asm volatile("sti");

	//sleep(5000);
	printk("Sleep 5s\n");

	while(1);
}
