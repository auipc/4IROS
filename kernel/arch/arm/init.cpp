#include <kernel/arch/arm/arm.h>
#include <stddef.h>
#include <stdint.h>

extern "C" char _kernel_start;
extern "C" char _kernel_end;

void set_translation_table_base(uint64_t addr) {
	uint64_t ttbr0 = addr;

	// ARM still expects the address to be full, despite also requiring
	// alignment. So, just mask off the bits that are important.
	ttbr0 &= ((1ull << 42ull) - 1ull) << 5ull;
	asm volatile("msr TTBR0_EL1, x0" ::"r"(ttbr0));
	asm volatile("mov x0, xzr\n"
				 "orr x0, x0, #(1<<4)\n" // 48 bit addr enable
				 //"orr x0, x0, #(1<<5)\n" // 32 bit address width
				 "msr TCR_EL1, x0\n"
				 "isb\n"
				 "mrs x0, SCTLR_EL1\n"
				 "orr x0, x0, #1\n"
				 "msr SCTLR_EL1, x0\n"
				 "isb\n");
}

struct TranslationBlockEntry {
	uint8_t valid : 1;
	uint8_t type : 1;

	uint8_t indx : 3;
	uint8_t secure : 1;	  // NS
	uint8_t access : 2;	  // AP
	uint8_t sharable : 2; // SH
	uint8_t accessed : 1; // AF

	uint8_t res0_low : 4;
	uint64_t addr : 32;
	uint8_t res0_high : 4;
	uint16_t upper : 12;
} __attribute__((packed));

// n = 30
struct TranslationBlockEntryLV1 {
	uint8_t valid : 1;
	uint8_t type : 1;

	uint8_t indx : 3;
	uint8_t secure : 1;	  // NS
	uint8_t access : 2;	  // AP
	uint8_t sharable : 2; // SH
	uint8_t accessed : 1; // AF
	uint8_t unused : 1;

	uint32_t res0 : 18;
	uint64_t addr : 18;
	uint16_t res0_ : 4;
	uint16_t upper : 12;
} __attribute__((packed));

// n = 21
struct TranslationBlockEntryLV2 {
	uint8_t valid : 1;
	uint8_t type : 1;

	uint8_t indx : 3;
	uint8_t secure : 1;	  // NS
	uint8_t access : 2;	  // AP
	uint8_t sharable : 2; // SH
	uint8_t accessed : 1; // AF
	uint8_t unused : 1;

	uint32_t res0 : 9;
	uint64_t addr : 27;
	uint16_t res0_ : 4;
	uint16_t upper : 12;
} __attribute__((packed));

// https://armv8-ref.codingbelief.com/en/chapter_d4/d43_1_vmsav8-64_translation_table_descriptor_formats.html
struct TranslationTableEntry {
	uint8_t valid : 1;
	uint8_t type : 1;

	uint32_t ignored : 10;
	uint64_t addr : 36;
	uint8_t res0 : 4;
	uint8_t ignored_2 : 7;
	uint8_t upper : 5;
} __attribute__((packed));

struct TranslationPage {
	uint8_t valid : 1;
	uint8_t type : 1;

	uint8_t indx : 3;
	uint8_t secure : 1;	  // NS
	uint8_t access : 2;	  // AP
	uint8_t sharable : 2; // SH
	uint8_t accessed : 1; // AF
	uint8_t unused : 1;

	uint64_t addr : 36;
	uint8_t res0 : 4;
	uint16_t upper : 12;
} __attribute__((packed));

static_assert(sizeof(TranslationBlockEntry) == sizeof(uint64_t));
static_assert(sizeof(TranslationTableEntry) == sizeof(uint64_t));
static_assert(sizeof(TranslationBlockEntryLV1) == sizeof(uint64_t));
static_assert(sizeof(TranslationBlockEntryLV2) == sizeof(uint64_t));
static_assert(sizeof(TranslationPage) == sizeof(uint64_t));

uint64_t pt_lv_0[512] __attribute__((aligned(512 * sizeof(uint64_t)))) = {};
uint64_t pt_lv_1[512] __attribute__((aligned(512 * sizeof(uint64_t)))) = {};
uint64_t pt_lv_2[512] __attribute__((aligned(512 * sizeof(uint64_t)))) = {};
TranslationPage pt_lv_3[512]
	__attribute__((aligned(512 * sizeof(TranslationPage)))) = {};

uint64_t configure_as_page(uint64_t addr) {
	addr |= (1 << 0);
	addr |= (TRANSLATION_BLOCK << 1);
	addr |= (1 << 10);
	return addr;
}

static constexpr uint64_t ADDRESS_MASK = (1ull << 48ull) - 1;
void map_page(uintptr_t addr, int flag) {
	(void)flag;
	size_t lv_0_indx = (addr >> 39) & 0x1ff;
	size_t lv_1_indx = (addr >> 30) & 0x1ff;
	size_t lv_2_indx = (addr >> 21) & 0x1ff;
	size_t lv_3_indx = (addr >> 12) & 0x1ff;
	// FIXME: grab first page level a different way.
	uint64_t *lv1_table =
		(uint64_t *)(((pt_lv_0[lv_0_indx] >> 12) << 12) & ADDRESS_MASK);
	uint64_t *lv2_table =
		(uint64_t *)(((lv1_table[lv_1_indx] >> 12) << 12) & ADDRESS_MASK);
	TranslationPage *lv3_table =
		(TranslationPage *)(((lv2_table[lv_2_indx] >> 12) << 12) &
							ADDRESS_MASK);

	auto lv3 = &lv3_table[lv_3_indx];

	lv3->addr = addr >> 12;
	// FIXME: make these up to flags
	lv3->accessed = 1;
	lv3->valid = 1;
	lv3->access = 0;
	// Lowest level always has type set to 1
	lv3->type = 1;
}

void arm_paging_init() {
	/*
	for (int i = 0; i < 512; i++) {
		pt_lv_1[i].valid = 1;
		pt_lv_1[i].type = TRANSLATION_BLOCK;
		pt_lv_1[i].addr = i * 0x1000;
		pt_lv_1[i].access = 0;
		pt_lv_1[i].accessed = 1;
	}*/

	/*
	for (size_t addr = (size_t)&_kernel_start;
		 addr < ((size_t)&_kernel_end) + PAGE_SIZE - 1; addr += PAGE_SIZE) {
		simple_map_page(addr);
	}*/

	//  auto lvl_3_indx = (address >> 21ull) & 0x1ffull;
	auto lv0 = &pt_lv_0[0];
	auto lv1 = &pt_lv_1[0];
	auto lv2 = &pt_lv_2[0];

	*lv0 = (uint64_t)pt_lv_1;
	*lv0 |= (1 << 0);
	*lv0 |= (TRANSLATION_TABLE << 1);

	*lv1 = (uint64_t)pt_lv_2;
	*lv1 |= (1 << 0);
	*lv1 |= (TRANSLATION_TABLE << 1);

	*lv2 = (uint64_t)pt_lv_3;
	*lv2 |= (1 << 0);
	*lv2 |= (TRANSLATION_TABLE << 1);

	for (int i = 0; i < 512; i++) {
		auto lv3 = &pt_lv_3[i];
		lv3->accessed = 1;
		lv3->access = 0;
		lv3->addr = i;
		lv3->type = TRANSLATION_TABLE;
		lv3->valid = 1;
	}

	for (int i = 0; i < 512; i++) {
		map_page(i * PAGE_SIZE, 0);
	}

	set_translation_table_base((uint64_t)pt_lv_0);
}

#define PERI_BASE 0x20000000
#define MAIL_BASE 0xB880
#define MBOX_REGS (PERI_BASE + MAIL_BASE)
#define MBOX0_READ MBOX_REGS
#define MBOX0_WRITE (MBOX_REGS + 0x20)
#define MBOX0_STATUS (MBOX_REGS + 0x18)
#define MBOX_SUCCESS 0x80000000
#define MBOX_FAIL (MBOX_SUCCESS & 1)

struct Framebuffer {
	uint32_t width;
	uint32_t height;
	uint32_t vwidth;
	uint32_t vheight;
	uint32_t pitch;
	uint32_t depth;
	uint32_t ignorex;
	uint32_t ignorey;
	uint64_t pointer : 32;
	uint32_t size;
};

struct RGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

RGB screen_buffer[1920][1080] = {};

extern "C" void kernel_init() {
	Framebuffer fb __attribute__((aligned(16)));
	fb.depth = sizeof(RGB) * 8;

	fb.vwidth = fb.width = 640;
	fb.vheight = fb.height = 480;
	// truncate to 32bits, bad practice
	fb.pointer = (uint64_t)screen_buffer;

	uint32_t fb_ptr = 0x40000000 + (uint64_t)&fb;

	*reinterpret_cast<uint64_t *>(MBOX0_WRITE) = fb_ptr | 1;

	for (int x = 0; x < 1920; x++) {
		for (int y = 0; y < 1080; y++) {
			screen_buffer[x][y].r = 255;
		}
	}

	arm_paging_init();

	while (1)
		;
}
