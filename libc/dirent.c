#include <dirent.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

// FIXME check if file is directory
DIR *opendir(const char *path) {
	int fd;
	if ((fd = open(path, O_RDONLY | O_DIRECTORY)) < 0)
		return NULL;
	DIR *d = (DIR *)malloc(sizeof(DIR));
	d->fd = fd;
	d->dirlist = NULL;
	d->dircount = 0;
	return d;
}

struct dirent *readdir(DIR *dir) {
	if (!dir)
		return NULL;
	struct dirent *ent = (struct dirent *)malloc(sizeof(struct dirent));
	if (read(dir->fd, ent, 1) < 0) {
		free(ent);
		return NULL;
	}

	dir->dircount++;
	dir->dirlist = (struct dirent **)realloc(
		dir->dirlist, sizeof(struct dirent **) * dir->dircount);
	dir->dirlist[dir->dircount - 1] = ent;
	return ent;
}
