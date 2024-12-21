#include <ctype.h>

int toupper(int ch) {
	if ((ch >= 'a') && (ch <= 'z')) {
		ch -= 32;
	}
	return ch;
}

int tolower(int ch) {
	if ((ch >= 'a') && (ch <= 'z')) {
		return ch;
	}
	return ch - 32;
}

int islower(int ch) {
	if ((ch >= 'a') && (ch <= 'z')) {
		return 1;
	}
	return 0;
}
