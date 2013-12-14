/*
	nsboot/log.c
	nsboot logger subsystem
	* dynamically allocated
	* text based
	* 4 levels of priority [0-3]
	* formatted output (printf) capable - logprintf
	* perror clone available - logperror
	* display in text or graphics mode or dump to file

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

struct str_list *loglist;

const uint32_t logpalette[4] = {
	0xFF404040,
	0xFFFFFF00,
	0xFFFF8000,
	0xFFFF0000 };

void init_log(void) {
	loglist = new_str_list(8);
}

void logprintf(const char *fmt, ...) {
	va_list args;
	char buf[128];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	append_to_str_list(loglist, buf);

	display_logbar();
}

void display_logbar(void) {
	char *str;
	int loglevel = 0, logindex = 0;

	if (is_fb_available()) for (int i = 0; i < 16; ++i) {
		logindex = loglist->used - 16 + i;
		if (logindex < 0) continue;
		loglevel = (int)strtol(loglist->data[logindex], &str, 10);
		if (str == NULL) continue;
		if (loglevel < 0) loglevel = 0;
		else if (loglevel > 3) loglevel = 3;
		fill_rect(0, FBHEIGHT - 256 + i*16, FBWIDTH, 16, logpalette[loglevel]);
		text(str, 0, FBHEIGHT - 256 + i*16, 1, 1, 0xFFFFFFFF, logpalette[loglevel]);
	} else
		fprintf(stderr, "%s\n", loglist->data[loglist->used-1]+1);
}

void display_wholelog(void) {
	char *str;
	int loglevel = 0, logindex = 0;
	int start = 0;
	if (is_fb_available()) while (start < loglist->used) {
		clear_screen();

		for (int i = 0; i < FBHEIGHT / 16; ++i) {
			logindex = start + i;
			if (logindex >= loglist->used) break;
			if (loglist->data[logindex] == NULL) continue;

			loglevel = (int)strtol(loglist->data[logindex], &str, 10);
			if (str == NULL) continue;
			if (loglevel < 0) loglevel = 0;
			else if (loglevel > 3) loglevel = 3;
			fill_rect(0, i*16, FBWIDTH, 16, logpalette[loglevel]);
			text(str, 0, i*16, 1, 1, 0xFFFFFFFF, logpalette[loglevel]);
		}
		start += FBHEIGHT / 16;

		update_screen();
		wait_touch();
	} else for (int i = 0; i < loglist->used; ++i)
		fprintf(stderr, "%s\n", loglist->data[i]+1);
}

void dump_log_to_file(const char *path) {
	FILE *log_fp;
	int status = 0;

	log_fp = fopen(path, "a");
	if (log_fp == NULL) {
		logperror("can't open log file for writing");
		return;
	}

	for (int i = 0; i < loglist->used; ++i) {
		if (loglist->data[i] != '\0')
			status = fprintf(log_fp, "%s", loglist->data[i]);
		if (status == EOF) {
			logperror("error writing to log file");
			fclose(log_fp);
			return;
		}
	}
	fclose(log_fp);
}
