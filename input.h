/*
	nsboot/input.h
	header for input functionality for nsboot

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

#ifndef NSBOOT_TOUCH_H

#define in_box(x, y, w, h) ((ts_x >= x) && (ts_x <= x+w) && (ts_y >= y) && (ts_y <= y+h))

int ts_open(char *);
void ts_read(int *, int *);
void ts_close(void);
void wait_touch(void);
void vibrate(int);

#endif
