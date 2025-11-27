#ifdef __unix__

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

#include "shared.h"

#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)(SIZE_MAX>>1))
#endif

/*
 * Shamelessly stolen from: https://github.com/datenwolf/binreloc/blob/master/binreloc.c
 *
 * Original maintainer: Wolfgang 'datenwolf' Draxinger <coding@datenwolf.net>
 * Licensed with WTFPL
 */

char *find_exe(void)
{
	char *path, *path2, *line, *result;
	size_t buf_size;
	ssize_t size;
	struct stat stat_buf;
	FILE *f;

	/* Read from /proc/self/exe (symlink) */
	if (sizeof (path) > SSIZE_MAX)
		buf_size = SSIZE_MAX - 1;
	else
		buf_size = PATH_MAX - 1;
	path = (char *) malloc (buf_size);
	if (path == NULL) {
		return NULL;
	}
	path2 = (char *) malloc (buf_size);
	if (path2 == NULL) {
		free (path);
		return NULL;
	}

	strncpy (path2, "/proc/self/exe", buf_size - 1);

	while (1) {
		int i;

		size = readlink (path2, path, buf_size - 1);
		if (size == -1) {
			/* Error. */
			free (path2);
			break;
		}

		/* readlink() success. */
		path[size] = '\0';

		/* Check whether the symlink's target is also a symlink.
		 * We want to get the final target. */
		i = stat (path, &stat_buf);
		if (i == -1) {
			/* Error. */
			free (path2);
			break;
		}

		/* stat() success. */
		if (!S_ISLNK (stat_buf.st_mode)) {
			/* path is not a symlink. Done. */
			free (path2);
			return path;
		}

		/* path is a symlink. Continue loop and resolve this. */
		strncpy (path, path2, buf_size - 1);
	}


	/* readlink() or stat() failed; this can happen when the program is
	 * running in Valgrind 2.2. Read from /proc/self/maps as fallback. */

	buf_size = PATH_MAX + 128;
	line = (char *) realloc (path, MAX(1, buf_size));
	if (line == NULL) {
		/* Cannot allocate memory. */
		free (path);
		return NULL;
	}

	f = fopen ("/proc/self/maps", "r");
	if (f == NULL) {
		free (line);
		return NULL;
	}

	/* The first entry should be the executable name. */
	result = fgets (line, (int) buf_size, f);
	if (result == NULL) {
		fclose (f);
		free (line);
		return NULL;
	}

	/* Get rid of newline character. */
	buf_size = strlen (line);
	if (buf_size <= 0) {
		/* Huh? An empty string? */
		fclose (f);
		free (line);
		return NULL;
	}
	if (line[buf_size - 1] == 10)
		line[buf_size - 1] = 0;

	/* Extract the filename; it is always an absolute path. */
	path = strchr (line, '/');

	/* Sanity check. */
	if (strstr (line, " r-xp ") == NULL || path == NULL) {
		fclose (f);
		free (line);
		return NULL;
	}

	path = strdup (path);
	free (line);
	fclose (f);
	return path;
}

/* Emulates glibc's strndup() */
static char *
br_strndup (const char *str, size_t size)
{
	char *result = (char *) NULL;
	size_t len;

	if (str == (const char *) NULL)
		return (char *) NULL;

	len = strlen (str);
	if (len == 0)
		return strdup ("");
	if (size > len)
		size = len;

	result = (char *) malloc (len + 1);
	memcpy (result, str, size);
	result[size] = '\0';
	return result;
}

char *
br_dirname (const char *path)
{
	char *end, *result;

	if (path == (const char *) NULL)
		return (char *) NULL;

	end = strrchr (path, '/');
	if (end == (const char *) NULL)
		return strdup (".");

	while (end > path && *end == '/')
		end--;
	result = br_strndup (path, end - path + 1);
	if (result[0] == 0) {
		free (result);
		return strdup ("/");
	} else
		return result;
}

#endif

