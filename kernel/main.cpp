#include <kernel/Debug.h>
#include <kernel/arch/amd64/kernel.h>
#include <kernel/arch/amd64/x64_sched_help.h>
#include <kernel/arch/x86_common/IO.h>
#include <kernel/assert.h>
#include <kernel/driver/BochsFramebuffer.h>
#include <kernel/driver/PS2Keyboard.h>
#include <kernel/driver/VGAFramebuffer.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/shedule/proc.h>
#include <kernel/util/HashTable.h>
#include <kernel/util/NBitmap.h>
#include <kernel/util/Spinlock.h>
#include <kernel/util/Vec.h>
#include <kernel/vfs/vfs.h>
//#define STB_TRUETYPE_IMPLEMENTATION
//#include <stb_truetype.h>

#if 0
extern "C" void syscall_interrupt_handler();

extern "C" void syscall_interrupt(InterruptRegisters &regs) {
	Syscall::handler(regs);
}
#endif

void test_fs() {
	Vec<const char *> zero_path;
	zero_path.push("dev");
	zero_path.push("zero");
	VFSNode *zero_file = VFS::the().open(zero_path);
	assert(zero_file);
	uint8_t zero_value = 0xFF;
	zero_file->read(&zero_value, sizeof(uint8_t) * 1);
	assert(zero_file == 0);
}

template <class T> constexpr T min(const T &a, const T &b) {
	return a < b ? a : b;
}

template <class T> constexpr T max(const T &a, const T &b) {
	return a > b ? a : b;
}

void kidle() {
	while (1)
		;
}

#if 0
void kidle2() {
	asm volatile("cli");
	auto pjth = VFS::the().parse_path("/dev/bochs");
	auto bochs_file = VFS::the().open(pjth);
	auto pth = VFS::the().parse_path("jetbrains.ttf");
	auto font_file = VFS::the().open(pth);

	uint8_t* ttf_buffer = new uint8_t[font_file->size()];
	font_file->read(ttf_buffer, font_file->size());

	stbtt_fontinfo font;
	stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0));

	const char text[] = "Note: the grandmothers are growing restless. Do not encourage them.";

	uint16_t xres = *(uint16_t*)(0xfebf8000+0x500+2); 
	uint16_t yres = *(uint16_t*)(0xfebf8000+0x500+4); 
	int width_off = 0;
	int py_offset = 0;
	static const int SPACING = 10;
	for (size_t i = 0; i < strlen(text); i++) {
		int width=0, height=0, xoff=0, yoff=0;
		int x0, x1, y0, y1;

		stbtt_GetCodepointBox(&font, text[i], &x0, &y0, &x1, &y1);
		int center_x = x0+((x1-x0)/2);
		int center_y = ((y1-y0)/2);
		int pix_scale = 48;
		float scale = stbtt_ScaleForPixelHeight(&font, pix_scale);
		unsigned char* lol = stbtt_GetCodepointBitmap(&font, scale, scale, text[i], &width, &height, 0,0);
		int ascent, descent, lineGap;
		stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

		if (i == 0) {
			for (int px = 0; px < xres; px++) {
				bochs_file->seek(((py_offset+(pix_scale-height)+height)*xres+width_off+px)*4);
				uint32_t col = 0xFF0000;
				bochs_file->write(&col, sizeof(uint32_t));
			}
		}

		int ascent_pix = ascent*scale;
		for (int py = 0; py < height; py++) {
			for (int px = 0; px < width; px++) {
				if (!lol[py*width+px]) continue;
				bochs_file->seek(((py_offset+(pix_scale-height)+ascent+1+py)*xres+width_off+px)*4);
				uint32_t col = (lol[py*width+px]<<16)|(lol[py*width+px]<<8)|(lol[py*width+px]);
				bochs_file->write(&col, sizeof(uint32_t));
			}
		}
		width_off += width+SPACING;
		if (width_off > xres) {
			width_off = 0;
			py_offset += (ascent*scale)-(descent*scale)+(lineGap*scale);
		}
		STBTT_free(lol, font.userdata);
	}
	asm volatile("sti");

	while(1);
}
#endif

void kproc() {
#if 0
	auto pjth = VFS::the().parse_path("/dev/bochs");
	auto bochs_file = VFS::the().open(pjth);

	uint16_t xres = *(uint16_t*)(0xfebf8000+0x500+2); 
	uint16_t yres = *(uint16_t*)(0xfebf8000+0x500+4); 
	asm volatile("sti");
	size_t index = 0;
	while (true) {
		size_t i = 0;
		Vec<VFSNode*>& sub = VFS::the().get_root_fs()->mounted_filesystem()->nodes();
		printk("sub %d\n", sub.size());
		for (i = index; i < sub.size(); i++) {
			auto name = sub[sub.size()-i-1]->get_name();
			if (name[strlen(name)-1] == 'f') {
				break;
			}
		}
		
		// Read farbfeld image
		auto image_file = sub[sub.size()-index-1];
		image_file->open(Vec<const char*>());

		if (!image_file->size()) {
			index++;
			continue;
		}
		uint64_t magic;
		image_file->read(&magic, 8);
		// "farbfeld" in hex
		if (magic != 0x646C656662726166) {
			printk("Bad farbfeld header\n");
			index++;
			continue;
		}
		uint32_t width = 0;
		uint32_t height = 0;
		image_file->read(&width, 4);
		image_file->read(&height, 4);
		width = endswap32(width);
		height = endswap32(height);

		printk("Width %d Height %d\n", width, height);
		printk("Begins at %x\n", image_file->position());
		uint16_t* image_buffer = new uint16_t[width*height*4];
		printk("sz %d\n", width*height*4*2);
		image_file->read(image_buffer, width*height*4*sizeof(uint16_t));
		printk("%d %x\n", (width*height*4)-1, image_buffer[(width*height*4)-1]);
		memset((char*)0xfd000000, 25, xres*yres*4);

		for (size_t py = 0; py < height; py++) {
			for (size_t px = 0; px < width; px++) {
				uint16_t r = image_buffer[(py*width+px)*4];
				r = endswap16(r);
				uint16_t g = image_buffer[(py*width+px)*4+1];
				g = endswap16(g);
				uint16_t b = image_buffer[(py*width+px)*4+2];
				b = endswap16(b);
				uint16_t a = image_buffer[(py*width+px)*4+3];
				a = endswap16(a);

				bochs_file->seek((py*xres+px)*4);
				uint32_t col =  ((b&0xff)) | ((g&0xff)<<8) | ((r&0xff)<<16);
				bochs_file->write(&col, sizeof(uint32_t));
			}
		}

		//auto kbd_size = kbd->size();
		//while (kbd->size() == kbd_size);
		index++;
		delete[] image_buffer;
	}
#endif
	while (1)
		;
}
// FIXME this should be the kernel stack

extern bool s_use_actual_allocator;
extern TSS tss;

extern "C" [[noreturn]] void kernel_main() {
	printk("VFS init\n");
	VFS::init();

	printk("ps2\n");
	auto kbd = new PS2Keyboard();
	VFS::the().get_dev_fs()->push(kbd);
	kbd->init();

	auto bochs = new BochsFramebuffer();
	VFS::the().get_dev_fs()->push(bochs);
	bochs->init();

	// Read generated symtab file
	auto pth = VFS::the().parse_path("asymtab");
	auto symtab_file = VFS::the().open(pth);
	if (!symtab_file) {
		printk("Warning!!! Symtab file missing\n");
	} else {
		auto symtab_sz = symtab_file->size();
		char* sym_buf = new char[symtab_sz];
		symtab_file->read(sym_buf, symtab_sz);
		Debug::parse_symtab(sym_buf, symtab_sz);
		delete[] sym_buf;
	}

	Process::init();
	panic("Task switch failed somehow???\n");
	while (1)
		;
}
