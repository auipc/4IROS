#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

typedef void (*builtin_ptr_t)(int, char **);

typedef struct {
	const char *name;
	builtin_ptr_t func;
} builtin_t;

void builtin_echo(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (i > 1)
			printf(" ");
		printf("%s", argv[i]);
	}
	printf("\n");
}

static const builtin_t s_builtins[] = {{"echo", &builtin_echo}};

void handle_cmd(const char *buf, size_t buf_sz) {
	pid_t pid = 0;
	int status = 0;
	if (buf_sz <= 1)
		return;
	// parse args
	int argc = 0;
	char **argv = NULL;

	size_t last_bound = 0;
	char *s = NULL;
	for (size_t i = 0; i < buf_sz; i++) {
		if (buf[i] == ' ' || buf[i] == '\0') {
			s = (char *)malloc(sizeof(char) * ((i - last_bound) + 1));
			memset(s, 0, sizeof(char) * ((i - last_bound) + 1));

			memcpy(s, buf + last_bound, i - last_bound);

			char **tmp = (char **)malloc(sizeof(char **) * ++argc);
			memcpy(tmp, argv, sizeof(char **) * (argc - 1));
			argv = tmp;
			argv[argc - 1] = s;

			last_bound = i + 1;
		}
	}

	for (int i = 0; i < sizeof(s_builtins) / sizeof(s_builtins[0]); i++) {
		if (!strcmp(s_builtins[i].name, argv[0])) {
			s_builtins[i].func(argc, argv);
			return;
		}
	}

	if (!(pid = fork())) {
		exit(execvp(argv[0], argv));
	}
	printf("CMD FORK  PID%x\n", pid);

	waitpid(pid, &status, 0);
	if (status) {
		printf("Not found\n");
	}

	// FIXME: free args
}

void parse_autorun() {
	int autorun_fd = open("autorun", 0);
	int sz = lseek(autorun_fd, 0, SEEK_END);
	lseek(autorun_fd, 0, SEEK_SET);
	char *autorun_buf = (char *)malloc(sz);
	read(autorun_fd, autorun_buf, sz);
	for (int i = 0; i < sz; i++) {
		if (autorun_buf[i] == '\n') {
			handle_cmd(autorun_buf, i - 1);
		}
	}
}

int main() {
#if 0
	if (fork()) {
		while(1) {
		}
	}
#endif
	printf("Shell\n");
	//parse_autorun();
	char *buf = (char *)malloc(4096);
	size_t buf_sz = 0;
	printf(">");
	while (1) {
		buf_sz += read(0, buf + buf_sz, 1);
		for (int i = 0; i < buf_sz; i++) {
			if (buf[i] == '\b') {
				if (buf_sz > 1) {
					buf[buf_sz] = '\0';
					buf[buf_sz - 1] = '\0';
					buf_sz -= 2;
				}
			}
			if (buf[i] == '\n') {
				buf[i] = '\0';
				handle_cmd(buf, buf_sz);
				buf_sz = 0;
				printf(">");
			}
		}
	}
}
