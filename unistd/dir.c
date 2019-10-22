/*
 * Phoenix-RTOS
 *
 * libphoenix
 *
 * unistd (POSIX routines for directory operations)
 *
 * Copyright 2018 Phoenix Systems
 * Author: Jan Sikorski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <posix/utils.h>


static struct {
	char *cwd;
} dir_common;


int chdir(const char *path)
{
	struct stat s;
	int len;
	char *canonical;

	if ((canonical = canonicalize_file_name(path)) == NULL)
		return -ENOMEM;

	if (stat(canonical, &s) < 0 || !S_ISDIR(s.st_mode)) {
		free(canonical);
		return -1;
	}

	len = strlen(canonical) + 1;
	if ((dir_common.cwd = realloc(dir_common.cwd, len)) == NULL) {
		free(canonical);
		return -1; /* ENOMEM */
	}

	setenv("PWD", canonical, 1);

	memcpy(dir_common.cwd, canonical, len);
	free(canonical);

	return EOK;
}


static int getcwd_len(void)
{
	int len;
	char *wd;

	if (dir_common.cwd == NULL) {
		if ((wd = getenv("PWD")) == NULL)
			wd = "/";

		len = strlen(wd);

		if ((dir_common.cwd = malloc(len + 1)) == NULL)
			return -1;

		memcpy(dir_common.cwd, wd, len + 1);
		return len;
	}

	return strlen(dir_common.cwd);
}


char *getcwd(char *buf, size_t size)
{
	int len = getcwd_len();

	if (len == -1) {
		errno = ENOMEM;
		return NULL;
	}

	if (size == 0) {
		if (buf == NULL) {
			size = len + 1;
		}
		else {
			errno = EINVAL;
			return NULL;
		}
	}
	else if (size < (len + 1)) {
		errno = ERANGE;
		return NULL;
	}

	if (buf == NULL) {
		if ((buf = malloc(size)) == NULL) {
			errno = ENOMEM;
			return NULL;
		}
	}

	memcpy(buf, dir_common.cwd, len + 1);
	return buf;
}


/* TODO: resolve links, handle '//', '.' and '..' */
char *canonicalize_file_name(const char *path)
{
	char *buf;
	int bufsiz;
	int cwdlen;
	int pathlen;

	if (path == NULL)
		return NULL;

	pathlen = strlen(path);

	if (*path != '/') {
		cwdlen = getcwd_len();
		if (cwdlen < 0)
			return  NULL; /* ENOMEM */

		bufsiz = cwdlen + pathlen + 3;
		if ((buf = malloc(bufsiz)) == NULL)
			return NULL; /* ENOMEM */

		buf = getcwd(buf, bufsiz);
		if (buf == NULL)
			return NULL;

		if (buf[cwdlen - 1] != '/') {
			buf[cwdlen] = '/';
			buf[cwdlen + 1] = 0;
		}
		buf = strcat(buf, path);

		pathlen += cwdlen + 1;
	}
	else {
		bufsiz = pathlen + 2;
		if ((buf = malloc(bufsiz)) == NULL)
			return NULL; /* ENOMEM */

		strcpy(buf, path);
	}

	if (buf[pathlen - 1] == '/') {
		buf[pathlen] = '.';
		buf[pathlen + 1] = '\0';
	}

	return buf;
}


int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
	if (read(dirp->fd, entry, sizeof(struct dirent) + NAME_MAX) != -1) {
		*result = entry;
		return 0;
	}
	else {
		*result = NULL;
		return errno;
	}
}


struct dirent *readdir(DIR *dirp)
{
	static union {
		struct dirent storage;
		char bytes[sizeof(struct dirent) + NAME_MAX];
	} u;

	struct dirent *result;
	readdir_r(dirp, &u.storage, &result);
	return result;
}


DIR *opendir(const char *dirname)
{
	DIR *d;
	int fd;

	if ((fd = open(dirname, O_DIRECTORY | O_RDONLY)) == -1)
		return NULL;

	if ((d = malloc(sizeof(*d) + NAME_MAX)) == NULL) {
		close(fd);
		return NULL;
	}

	d->fd = fd;
	return d;
}


int closedir(DIR *dirp)
{
	close(dirp->fd);
	free(dirp);
	return 0;
}


ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
	return -ENOSYS;
}


int rmdir(const char *path)
{
	return -ENOSYS;
}


char *dirname(char *path)
{
	size_t len;

	if (path == NULL || (len = strlen(path)) == 0)
		return ".";

	char *last = path + len - 1;

	while ((last >= path) && (*last == '/'))
		--last;

	if (last < path)
		return "/";

	while ((last >= path) && (*last != '/'))
		--last;

	if (last < path)
		return ".";

	while ((last >= path) && (*last == '/'))
		--last;

	if (last < path)
		return "/";

	*(last + 1) = '\0';

	return path;
}


char *basename(char *path)
{
	size_t len;

	if (path == NULL || (len = strlen(path)) == 0)
		return ".";

	char *last = path + len - 1;

	while ((last >= path) && (*last == '/'))
		--last;

	if (last < path)
		return "/";

	*(last + 1) = '\0';

	while ((last >= path) && (*last != '/'))
		--last;

	return (last + 1);
}
