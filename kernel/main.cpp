#include <kernel/stdint.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/gdt.h>

extern "C" void __cxa_pure_virtual() {
	// Do nothing or print an error message.
}

extern "C" void kernel_main() {
	kmalloc_init();
	GDT::setup();
	printk_use_interface(new VGAInterface());
	printk("We're running! %d\n", 100);
	printk("We're running!\n");
	printk("We're running!\n");
	printk("We're running!\n");
	// char* lol = reinterpret_cast<char*>(kmalloc(1));
	/*short c = 'a' | (0xF << 8);
	for (uint32_t i = 0xC03FF000; i < 0xC03FF000 + 0x80; i += 2) {
		*((uint16_t*)i) = c++;
	}*/
	//*((uint16_t*)0xC03FF000) = 41;
	//*((uint16_t*)0xC03FF002) = 42;
	while (1)
		;
}
