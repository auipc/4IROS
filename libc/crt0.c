int main();

void _start() {
	int r = main();
#ifdef __i386__
	asm volatile("int $0x80" ::"a"(0x01), "b"(r));
#endif
}
