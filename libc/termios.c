#include <termios.h>
int tcsetattr(int fildes, int optional_actions,
			  const struct termios *termios_p) {}

int tcgetattr(int fildes, struct termios *termios_p) { return 0; }
