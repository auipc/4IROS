#ifndef _LIBC_SIGNAL
#define _LIBC_SIGNAL
#include <sys/types.h>
#define SIGKILL 1
#define SIGINT 2

typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);
int kill(pid_t pid, int sig);
#endif
