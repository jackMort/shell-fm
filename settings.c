/*
	vim:syntax=c tabstop=2 shiftwidth=2 noexpandtab

	Shell.FM - settings.c
	Copyright (C) 2006 by Jonas Kramer
	Published under the terms of the GNU General Public License (GPL).
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "hash.h"

struct hash rc; /* settings read from ~/.shell-fm.rc */

extern unsigned getln(char **, unsigned *, FILE *);
const char * rcpath(const char *);

int settings(const char * path, int first) {
	int retval = !0;
	FILE * fd = fopen(path, "r");
	unsigned nline = 0;
	char * line = NULL;
	unsigned size = 0;

	if(first)
		memset(& rc, 0, sizeof(struct hash));
	
	if(!fd)
		return 0;
		
	while(!feof(fd)) {
		char * ptr;
		
		if(!getln(& line, & size, fd))
			continue;

		++nline;

		if(strlen(line) < 2)
			continue;
	
		ptr = line;
		while((ptr = strchr(ptr, '#')) != NULL)
			if(ptr == line || ptr[-1] != '\\')
				* ptr = (char) 0;
			else
				if(ptr[-1] == '\\') {
					unsigned restlen = strlen(ptr);
					ptr[-1] = '#';
					if(restlen > 1)
						memmove(ptr, ptr + 1, restlen - 1);
					ptr[restlen] = (char) 0;
				}
		
		if(strlen(line) > 1) {
			char key[64] = { 0 }, value[256] = { 0 };

			if(sscanf(line, "%63[^= \t] = %255[^\r\n]", key, value) == 2)
				set(& rc, key, value);
			else {
				fprintf(stderr, "%s, line %d invalid.\n", path, nline);
				retval = 0;
			}
		}
	}

	fclose(fd);
	if(size)
		free(line);

	return retval;
}

const char * rcpath(const char * file) {
	static char path[4096] = { 0 };
	memset(path, 0, sizeof(path));
	snprintf(path, sizeof(path), "%s/.shell-fm/%s", getenv("HOME"), file);
	return path;
}
