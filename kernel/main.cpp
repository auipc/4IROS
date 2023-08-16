#include <kernel/stdint.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>

extern "C" void __cxa_pure_virtual()
{
    // Do nothing or print an error message.
}

extern "C" void kernel_main() {
	kmalloc_init();
	printk_use_interface(new VGAInterface());
	//char* lol = reinterpret_cast<char*>(kmalloc(1));
	printk("Hello, World\n");
	/*short c = 'a' | (0xF << 8);
	for (uint32_t i = 0xC03FF000; i < 0xC03FF000 + 0x80; i += 2) {
		*((uint16_t*)i) = c++;
	}*/
	//*((uint16_t*)0xC03FF000) = 41;
	//*((uint16_t*)0xC03FF002) = 42;
	while(1);
}
