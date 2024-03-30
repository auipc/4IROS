#include <kernel/Debug.h>
#include <kernel/printk.h>
#include <kernel/util/ism.h>
#include <stdint.h>

void Debug::stack_trace() {
	uint32_t *ebp = 0;
	READ_REGISTER("ebp", ebp);

	printk("==BEGIN STACK TRACE==\n");
	for (int i = 0; i < 10; i++) {
		if (!ebp)
			break;

		uint32_t eip = *(ebp + 1);

		printk("EIP: %x\n", eip);
		ebp = (uint32_t *)*(ebp);
	}
	printk("==END STACK TRACE==\n");
}

void print_type(char c) { printk("%c", c); }

void Debug::print_type(const char *str) { printk("%s", str); }

void Debug::print_type(int n) { printk("%d", n); }

void Debug::print_type_hex(int n) { printk("%x", n); }

void Debug::print_type(unsigned int n) { printk("%d", n); }
void Debug::print_type(long long unsigned int n) { printk("%d", n); }
