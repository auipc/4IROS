#include <unistd.h>
#include <priv/util.h>

int fork() {
	int r = 0;
	SYSCALL(3, r);
	return r;
}
