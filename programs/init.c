#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
	printf("===Fork===\n");
	int pid = 0;
	if (!(pid = fork())) {
		exec("term");
		while (1)
			;
	}

	waitpid(pid, NULL, 0);
	printf("Exit PID %d\n", pid);
	while(1);

	/*
	printf("pid: %d\n", pid);
	waitpid(pid);

	printf("scancode_ascii sizeof %d\n", sizeof(scancode_ascii));
	printf("Hello, World\n");
	char *addr = (char *)mmap((void *)0x60000, 0x4000);
	addr[0] = 1;
	printf("%x\n", *addr);
	char *addr1 = (char *)mmap((void *)0x50000, 0x2000);
	addr1[0] = 1;
	printf("%x\n", *addr1);

	int fd = open("/dev/keyboard", 0);
	printf("fd %d\n", fd);
	uint8_t use_upper = 0;
	uint8_t caps_lock = 0;

	while (1) {
		char buf[2 * 20];
		size_t size_read = read(fd, buf, 2 * 20);
		for (int i = 0; i < size_read; i += 2) {
			if (buf[i + 1] == 42)
				use_upper = buf[i] ^ !caps_lock;

			if (buf[i + 1] == 58 && !buf[i]) {
				caps_lock = !caps_lock;
				use_upper = caps_lock;
			}

			if (!buf[i] && sizeof(scancode_ascii) > buf[i + 1]) {
				if (!scancode_ascii[buf[i + 1]])
					continue;

				if (!use_upper)
					printf("%c", scancode_ascii[buf[i + 1]]);
				else
					printf("%c", scancode_ascii_upper[buf[i + 1]]);
			}
		}
	}*/
}
