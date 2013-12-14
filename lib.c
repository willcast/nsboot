/*
	nsboot/lib.c
	globally useful functions for nsboot

	Copyright (C) 2013 Will Castro

	This file is part of nsboot.

	nsboot is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	nsboot is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with nsboot.  If not, see <http://www.gnu.org/licenses/>
*/

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "types.h"
#include "touch.h"
#include "fb.h"
#include "log.h"

void qfprintf(const char *fname, const char *fmt, ...) {
	va_list args;
	FILE *fp;

	fp = fopen(fname, "w");
	if (fp == NULL) {
		logprintf("2 can't open %s: %s", fname,
			strerror(errno));
		return;
	}

	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

	fclose(fp);
}

void qfscanf(const char *fname, const char *fmt, ...) {
	va_list args;
	FILE *fp;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		logprintf("2 can't open %s: %s", fname,
			strerror(errno));
		return;
	}

	va_start(args, fmt);
	vfscanf(fp, fmt, args);
	va_end(args);

	fclose(fp);
}

void qpscanf(const char *cmd, const char *fmt,  ...) {
	va_list args;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp == NULL) {
		logprintf("2 can't popen %s: %s", cmd,
			strerror(errno));
		return;
	}

	va_start(args, fmt);
	vfscanf(fp, fmt, args);
	va_end(args);

	pclose(fp);	
}

int hexval(char c) {
	if (c <= '9' && c >= '0') {
		return (c - '0');
	}
	if (c <= 'f' && c >= 'a') {
		return (c - 'a' + 10);
	}
	if (c <= 'F' && c >= 'A') {
		return (c - 'A' + 10);
	}
	return -1;
}

