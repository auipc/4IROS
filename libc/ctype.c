#include <ctype.h>

int isalpha(char c) {
	return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'));
}

int isdigit(char c) { return (c >= '0') && (c <= '9'); }

int isalnum(char c) { return isalpha(c) || isdigit(c); }

int isupper(char c) { return (c >= 'A') && (c <= 'Z'); }

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

int islower(int ch) { return (ch >= 'a') && (ch <= 'z'); }

int isprint(int ch) { return (ch >= ' ') && (ch <= '~'); }

int iscntrl(int ch) { return ch < 32; }

int isspace(int ch) {
	return (ch == ' ') || (ch == '\f') || (ch == '\n') || (ch == '\t') ||
		   (ch == '\v');
}

int ispunct(int ch) {
	// slow
	return (isprint(ch) && !isalnum(ch)) || !isspace(ch);
}

int isblank(int ch) { return (ch == ' ') || (ch == '\t'); }
