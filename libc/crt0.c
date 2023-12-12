extern int main();

void _start() {
	int r = main();
	asm volatile("int $0x80"::"a"(0x01), "b"(r));
}
