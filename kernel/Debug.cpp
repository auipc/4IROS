#include <kernel/Debug.h>
#include <kernel/printk.h>
#include <kernel/util/Vec.h>
#include <kernel/util/ism.h>
#include <stdint.h>

Vec<SymTab> debug_symtabs;

void Debug::parse_symtab(const char *buffer, size_t length) {
	for (size_t idx = 0; idx < length; idx++) {
		size_t i = 0;
		uintptr_t addr = 0;
		size_t sz = 0;
		SymTab tab = {};

		for (i = idx; i < length; i++) {
			if (buffer[i] == ',')
				break;

			addr *= 16;
			uint8_t val = buffer[i] - '0';
			if (val > 9)
				val = buffer[i] - 'W';
			addr += val;
		}
		idx += i - idx + 1;
		for (i = idx; i < length; i++) {
			if (buffer[i] == ',')
				break;
			sz *= 10;
			uint8_t val = buffer[i] - '0';
			sz += val;
		}
		idx += i - idx + 1;
		for (i = idx; i < length; i++) {
			if (buffer[i] == '\n')
				break;
		}
		char *func_name = new char[i - idx + 1];
		memcpy(func_name, &buffer[idx], i - idx);
		func_name[i - idx] = '\0';
		idx += i - idx;

		tab.addr = addr;
		tab.size = sz;
		tab.func_name = func_name;
		debug_symtabs.push(tab);
	}
}

SymTab *Debug::resolve_symbol(uintptr_t addr) {
	if (!debug_symtabs.size())
		return nullptr;
	for (size_t i = 0; i < debug_symtabs.size(); i++) {
		auto &debug_symbol = debug_symtabs[i];
		if (debug_symbol.addr <= addr &&
			(debug_symbol.addr + debug_symbol.size) >= addr)
			return &debug_symbol;
	}
	return nullptr;
}

void Debug::stack_trace(uint64_t *rbp) {
	if (!rbp)
		READ_REGISTER("rbp", rbp);

	printk("==BEGIN STACK TRACE==\n");
	for (int i = 0; i < 10; i++) {
		if (!rbp)
			break;

		uint64_t rip = *(rbp + 1);
		if (!rip)
			break;

		printk("RIP: %x", rip);
		SymTab *symbol = resolve_symbol(rip);
		if (symbol)
			printk(" Function: %s", symbol->func_name);
		printk("\n");
		rbp = (uint64_t *)*(rbp);
	}
	printk("==END STACK TRACE==\n");
}

void Debug::stack_trace_depth(int depth) {
	uint64_t* rbp;
	READ_REGISTER("rbp", rbp);

	printk("==BEGIN STACK TRACE==\n");
	for (int i = 0; i < depth; i++) {
		if (!rbp)
			break;

		uint64_t rip = *(rbp + 1);
		if (!rip)
			break;

		printk("RIP: %x", rip);
		SymTab *symbol = resolve_symbol(rip);
		if (symbol)
			printk(" Function: %s", symbol->func_name);
		printk("\n");
		rbp = (uint64_t *)*(rbp);
	}
	printk("==END STACK TRACE==\n");
}


void print_type(char c) { printk("%c", c); }

void Debug::print_type(const char *str) { printk("%s", str); }

void Debug::print_type(int n) { printk("%d", n); }

void Debug::print_type_hex(int n) { printk("%x", n); }

void Debug::print_type(unsigned int n) { printk("%d", n); }
void Debug::print_type(long long unsigned int n) { printk("%d", n); }
