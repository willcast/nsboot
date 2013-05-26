/*
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

#ifndef NSBOOT_FB_H
#define NSBOOT_FB_H

#define FBWIDTH 1024
#define FBHEIGHT 768

#include <stdint.h>
#include <errno.h>

// management functions
int fb_init(void);
void fb_close(void);
int load_font(char *);

// drawing functions
void clear_screen(void);
void put_pixel(int, int, uint32_t);
void hline(int, int, int, uint32_t);
void vline(int, int, int, uint32_t);
void rect(int, int, int, int, uint32_t);
void fill_rect(int, int, int, int, uint32_t);
void text(char *, int, int, int, int, uint32_t, uint32_t);

// UI functions
void text_box(char *, int, int, int, int, int, uint32_t, uint32_t, uint32_t);

void status(char *);
void status_error(char *);
void message(char *);
void stprintf(const char *fmt, ...);
void steprintf(const char *fmt, ...);

#define stperror(str) steprintf("%s: %s", str, strerror(errno))

void set_brightness(int);

#endif
