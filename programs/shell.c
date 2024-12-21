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

void builtin_exit(int argc, char **argv) { exit(0); }

void builtin_echo(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (i > 1)
			printf(" ");
		printf("%s", argv[i]);
	}
	printf("\n");
}

static const builtin_t s_builtins[] = {{"echo", &builtin_echo},
									   {"exit", &builtin_exit}};

void handle_cmd(char *buf, size_t buf_sz) {
	pid_t pid = 0;
	int status = 0;
	if (buf_sz <= 1)
		return;
	char **argv = malloc(sizeof(char *) * 1);

	size_t argc = 0, i = 0, last_bound = 0;
	for (; i < buf_sz; i++) {
		if (buf[i] == ' ') {
			buf[i] = '\0';
			argv[argc++] = &buf[last_bound];
			argv = realloc(argv, sizeof(char *) * (argc + 1));
			last_bound = i + 1;
		}
	}
	buf[i] = '\0';
	argv[argc++] = &buf[last_bound];
	last_bound = i + 1;
	argv[argc] = 0;
	for (int i = 0; i < sizeof(s_builtins) / sizeof(s_builtins[0]); i++) {
		if (!strcmp(s_builtins[i].name, argv[0])) {
			s_builtins[i].func(argc, argv);
			return;
		}
	}

	if (!(pid = fork())) {
		exit(execvp(argv[0], argv));
	}

	waitpid(pid, &status, 0);
	if (status) {
		printf("Not found\n");
	}

	free(argv);
}

void parse_autorun() {
	int autorun_fd = open("autorun", 0);
	int sz = lseek(autorun_fd, 0, SEEK_END);
	lseek(autorun_fd, 0, SEEK_SET);
	char *autorun_buf = (char *)malloc(sz);
	read(autorun_fd, autorun_buf, sz);
	fwrite(autorun_buf, sz, 1, stdout);
#if 0
	for (int i = 0; i < sz; i++) {
		if (autorun_buf[i] == '\n') {
			handle_cmd(autorun_buf, i - 1);
		}
	}
#endif
}

int main() {
	char *buf = (char *)malloc(0x1000);
	size_t buf_sz = 0;
	printf("~>");
	while (1) {
		if ((buf_sz + 1) >= 0x1000)
			continue;
		int sz = read(0, buf + buf_sz, 1);
		if (!sz)
			continue;
		buf_sz++;
		if (buf[buf_sz - 1] == '\b') {
			if (buf_sz > 1) {
				buf[buf_sz] = '\0';
				buf[buf_sz - 1] = '\0';
				buf_sz -= 2;
			}
			continue;
		}
		if (buf[buf_sz - 1] == '\n') {
			buf[buf_sz - 1] = '\0';
			handle_cmd(buf, buf_sz);
			buf_sz = 0;
			printf("~>");
			continue;
		}
	}
}
