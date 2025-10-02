#include <errno.h>
#include <priv/common.h>
#include <signal.h>
#include <stddef.h>

sighandler_t s_handler = NULL;

void sigreturn() {
	asm volatile("syscall"
				 :
				 : "a"(SYS_SIGRET)
				 : "rcx", "r11", "memory");
}

void sigtramp(int sig) {
	s_handler(sig);
	sigreturn();
}

sighandler_t signal(int signum, sighandler_t handler) {
	int r = 0;
	s_handler = handler;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_SIGNAL), "b"(signum), "d"(sigtramp)
				 : "rcx", "r11", "memory");
	return (sighandler_t)r;
}

int kill(pid_t pid, int sig) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_KILL), "b"(pid), "d"(sig)
				 : "rcx", "r11", "memory");
	return r;
}
