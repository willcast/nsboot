/*
	nsboot/lib.h
	header of globally useful functions/macros for nsboot

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

#ifndef NSBOOT_LIB_H
#define NSBOOT_LIB_H

void qfprintf(const char *, const char *, ...);
void qfscanf(const char *, const char *, ...);
void qpscanf(const char *, const char *, ...);
int hexval(char);
int test_file(const char *);

#define array_size(x) (sizeof(x) / sizeof(*(x)))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define bool_inv(b) ((b) = !(b))

#endif
