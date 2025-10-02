#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

typedef void (*builtin_ptr_t)(int, char **);

typedef struct {
	const char *name;
	builtin_ptr_t func;
} builtin_t;

void builtin_cd(int argc, char **argv) {
	if (argc > 1) {
		if (chdir(argv[1]) < 0) {
			fprintf(stderr, "Invalid dir %d", errno);
		}
	}
}

void builtin_echo(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (i > 1)
			printf(" ");
		printf("%s", argv[i]);
	}
	printf("\n");
}

void builtin_exit(int argc, char **argv) { exit(0); }

static const builtin_t s_builtins[] = {
	{"echo", &builtin_echo}, {"exit", &builtin_exit}, {"cd", &builtin_cd}};

int foreground_pid = 0;
void handle_cmd(char *buf, size_t buf_sz) {
	pid_t pid = 0;
	int status = 0;
	if (buf_sz <= 1)
		return;
	if (buf[0] == '#')
		return;
	char **argv = malloc(sizeof(char *) * 2);

	size_t argc = 1, last_bound = 0;

	for (size_t i = 0; i < buf_sz; i++) {
		if (buf[i] == ' ') {
			buf[i] = '\0';
			argv[argc - 1] = &buf[last_bound];
			argv = realloc(argv, sizeof(char *) * (2 + ++argc));
			if ((i + 1) < buf_sz) {
				last_bound = i + 1;
			}
		}
	}

	argv[argc - 1] = &buf[last_bound];
	// Last char* pointer is NULL to indicate the end
	argv[argc] = 0;

	for (int i = 0; i < sizeof(s_builtins) / sizeof(s_builtins[0]); i++) {
		if (!strcmp(s_builtins[i].name, argv[0])) {
			s_builtins[i].func(argc, argv);
			return;
		}
	}

	if (!(pid = fork())) {
		execvp(argv[0], argv);
		printf("Not found\n");
		exit(127);
	}
	foreground_pid = pid;
	printf("Waitpid waiting on %d\n", pid);
	waitpid(pid, &status, 0);
	printf("Waitpid end %d\n", status);
end:
	free(argv);
}

void parse_autorun() {
	int autorun_fd = open(".shell", O_RDONLY);
	int sz = lseek(autorun_fd, 0, SEEK_END);
	lseek(autorun_fd, 0, SEEK_SET);
	char *autorun_buf = (char *)malloc(sz);
	read(autorun_fd, autorun_buf, sz);
	int last = 0;
	int i = 0;
	for (i = 0; i < sz; i++) {
		if (autorun_buf[i] == '\n') {
			autorun_buf[i] = '\0';
			handle_cmd(&autorun_buf[last], i);
			last = i + 1;
		}
	}

	if (last < i) {
		handle_cmd(&autorun_buf[last], i);
	}
}

size_t buf_sz = 0;
void handle_signal(int) {
	puts("^C");
	if (foreground_pid) {
		kill(foreground_pid, SIGKILL);
		foreground_pid = 0;
	} else {
		printf("?");
		buf_sz = 0;
	}
}

int main() {
	signal(SIGINT, &handle_signal);
	//parse_autorun();
	char *buf = (char *)malloc(0x1000);
	printf("?");
	fflush(stdout);
	while (1) {
		if ((buf_sz + 1) >= 0x1000)
			continue;
		int sz = read(0, buf + buf_sz, 1);
		if (!sz)
			continue;
		buf_sz++;
#if 0
		if (buf[buf_sz - 1] == '\b') {
			if (buf_sz > 1) {
				buf[buf_sz] = '\0';
				buf[buf_sz - 1] = '\0';
				buf_sz -= 2;
			}
			continue;
		}
#endif
		if (buf[buf_sz - 1] == '\n') {
			buf[buf_sz - 1] = '\0';
			handle_cmd(buf, buf_sz);
			buf_sz = 0;
			printf("?");
			fflush(stdout);
			continue;
		}
	}
}
