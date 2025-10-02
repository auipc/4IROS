#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

int main() {
	struct timeval tv1;
	gettimeofday(&tv1, NULL);
	int pid = 0;
	if (!(pid = fork())) {
		execvp("ls", NULL);
		exit(127);
	}
	waitpid(pid, NULL, 0);
	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	uint64_t tf1 = tv1.tv_sec * 1000000;
	uint64_t tf2 = tv2.tv_sec * 1000000;
	tf1 += tv1.tv_usec;
	tf2 += tv2.tv_usec;
	// not correct
	printf("time delta %f\n", ((tf2 - tf1) / 1000000.0));
	// printf("tf2 %d tf1 %d\n", tv2.tv_usec, tv1.tv_usec);
	return 0;
}
