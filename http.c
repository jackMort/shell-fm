/*
	vim:syntax=c tabstop=2 shiftwidth=2 noexpandtab

	Shell.FM - http.c
	Copyright (C) 2006 by Jonas Kramer
	Published under the terms of the GNU General Public License (GPL).
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>

#include <sys/socket.h>

#include "version.h"
#include "hash.h"

extern FILE * ropen(const char *, unsigned short);
extern void fshutdown(FILE *);
extern unsigned getln(char **, unsigned *, FILE *);

void freeln(char **, unsigned *);

unsigned encode(const char *, char **);

char ** fetch(char * const url, FILE ** pHandle, const char * data, const char * addhead) {
	char ** resp = NULL, * host, * file, * port, * status = NULL, * line = NULL;
	char urlcpy[512 + 1] = { 0 };
	unsigned short nport = 80;
	unsigned nline = 0, nstatus = 0, size = 0;
	signed validHead = 0;
	FILE * fd;
	const char * headFormat =
		"%s /%s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: Shell.FM " VERSION "\r\n";

	if(pHandle)
		* pHandle = NULL;
	
	strncpy(urlcpy, url, sizeof(urlcpy) - 1);

	host = & urlcpy[strncmp(urlcpy, "http://", 7) ? 0 : 7];
	port = strchr(host, 0x3A);
	file = strchr(port ? port : host, 0x2F);
	fflush(stderr);
	* file = (char) 0;
	++file;

	if(port) {
		char * ptr = NULL;
		nport = strtol(port, & ptr, 10);
		(ptr == port) && (nport = 80);
		* port = (char) 0;
	}

	if(!(fd = ropen(host, nport)))
		return NULL;

	fprintf(fd, headFormat, data ? "POST" : "GET", file ? file : "", host);

	if(addhead)
		fprintf(fd, "%s\r\n", addhead);

	if(data)
		fprintf(fd, "Content-Length: %ld\r\n\r\n%s\r\n", (long) strlen(data), data);

	fputs("\r\n", fd);
	fflush(fd);
	
	if(getln(& status, & size, fd) >= 12)
		validHead = sscanf(status, "HTTP/%*f %u", & nstatus);

	if(nstatus != 200 && nstatus != 301) {
		fshutdown(fd);
		if(size) {
			if(validHead != 2)
				fprintf(stderr, "Invalid HTTP: %s\n", status);
			else
				fprintf(stderr, "HTTP Response: %s", status);
			free(status);
		}
		return NULL;
	}

	freeln(& status, & size);
	
	while(!0) {
		if(!getln(& line, & size, fd) < 3)
			break;

		if(nstatus == 301 && !strncasecmp(line, "Location: ", 10)) {
			char newurl[512 + 1] = { 0 };
			sscanf(line, "Location: %512[^\r\n]", newurl);
			fprintf(stderr, "NEW: %s\n", newurl);
			fshutdown(fd);
			return fetch(newurl, pHandle, data, addhead);
		}
	}

	freeln(& line, & size);

	if(pHandle) {
		* pHandle = fd;
		return NULL;
	}
	
	while(!feof(fd)) {
		line = NULL;
		size = 0;
		
		resp = realloc(resp, (nline + 2) * sizeof(char *));
		assert(resp != NULL);

		if(getln(& line, & size, fd)) {
			char * ptr = strchr(line, 10);
			ptr && (* ptr = (char) 0);
			resp[nline] = line;
		} else if(size)
			free(line);

		resp[++nline] = NULL;
	}
	
	fshutdown(fd);
	return resp;
}

unsigned encode(const char * orig, char ** encoded) {
	register unsigned i = 0, x = 0;
	* encoded = calloc((strlen(orig) * 3) + 1, sizeof(char));
	while(i < strlen(orig)) {
		if(isalnum(orig[i]))
			(* encoded)[x++] = orig[i];
		else {
			snprintf(
					(* encoded) + x,
					strlen(orig) * 3 - strlen(* encoded) + 1,
					"%%%02x",
					(uint8_t) orig[i]
			);
			x += 3;
		}
		++i;
	}
	return x;
}

unsigned decode(const char * orig, char ** decoded) {
	register unsigned i = 0, x = 0;
	const unsigned len = strlen(orig);
	* decoded = calloc(len + 1, sizeof(char));
	while(i < len) {
		if(orig[i] != '%')
			(* decoded)[x] = orig[i];
		else {
			unsigned hex;
			if(sscanf(orig + i, "%%%02x", & hex) != 1)
				(* decoded)[x] = orig[i];
			else {
				(* decoded)[x] = (char) hex;
				i += 2;
			}
		}

		++i;
		++x;
	}

	* decoded = realloc(* decoded, (x + 1) * sizeof(char));
	return x;
}

void freeln(char ** line, unsigned * size) {
	if(* size) {
		free(* line);
		* line = NULL;
		* size = 0;
	}
}
