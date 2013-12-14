/*
	nsboot/log.h
	header for the nsboot logger subsystem

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

#ifndef NSBOOT_LOG_H
#define NSBOOT_LOG_H

#include <errno.h>

void logprintf(const char *, ...);
void init_log(void);
void display_wholelog(void);
void display_logbar(void);
void dump_log_to_file(const char *);

#define logperror(str) logprintf("2%s: %s", str, strerror(errno))

#endif
