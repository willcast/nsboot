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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "touch.h"
#include "fb.h"


int fb_fd;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
char *fbp;
int stat_page = 0;
long screen_size = 0;
uint8_t font[8][16][256];

int fb_init(void) {
	fb_fd = open("/dev/fb0", O_RDWR);
	if (fb_fd < 0) {
		perror("can't open framebuffer");
		return 1;
	}
	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo)) {
		perror("can't get fixed framebuffer info");
		close(fb_fd);
		return 1;
	}
	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
		perror("can't get variable framebuffer info");
		close(fb_fd);
		return 1;
	}

	screen_size = vinfo.xres * vinfo.yres << 2;
	fbp = (char*)mmap(0, screen_size, PROT_READ | PROT_WRITE,
		MAP_SHARED, fb_fd, 0);
	if (fbp < (char*)1) {
		perror("couldn't mmap framebuffer");
		close(fb_fd);
		return 1;
	}
	return 0;
}

void fb_close(void) {
	clear_screen();
	munmap(fbp, screen_size);
	close(fb_fd);
}

void put_pixel(int x, int y, uint32_t color) {
	uint32_t *pp = (uint32_t *)(fbp + (x + vinfo.xoffset) *
		(vinfo.bits_per_pixel >> 3) +
		(y + vinfo.yoffset) * finfo.line_length);
	*pp = color;
}

void hline(int x, int y, int w, uint32_t color) {
	for (int i = 0 ; i < w; ++i) put_pixel(x+i, y, color);
}

void vline(int x, int y, int h, uint32_t color) {
	for (int i = 0; i < h; ++i) put_pixel(x, y+i, color);
}

void rect(int x, int y, int w, int h, uint32_t color) {
	hline(x, y, w, color);
	hline(x, y+h, w, color);
	vline(x, y, h, color);
	vline(x+w, y, h, color);
}

int load_font(char *fname) {
	int font_fd;
	uint8_t byte;

	font_fd = open(fname, O_RDONLY);
	if (font_fd < 0) {
		perror("can't open font file");
		return 1;
	}

	for (int c = 0; c < 256; ++c)
		for (int y = 0; y < 16; ++y) {
			if (read(font_fd, &byte, 1) != 1) return 1;
			font[7][y][c] = (byte & 0x01);
			font[6][y][c] = (byte & 0x02) >> 1;
			font[5][y][c] = (byte & 0x04) >> 2;
			font[4][y][c] = (byte & 0x08) >> 3;
			font[3][y][c] = (byte & 0x10) >> 4;
			font[2][y][c] = (byte & 0x20) >> 5;
			font[1][y][c] = (byte & 0x40) >> 6;
			font[0][y][c] = (byte & 0x80) >> 7;
		}

	close(font_fd);
	return 0;
}

void text(const char *str, int x, int y, int zx, int zy, uint32_t fgc, uint32_t bgc) {
	int len = strlen(str);
	int ox = 0, oy = 0;

	for (int ch = 0; ch < len; ++ch) {
	  if (str[ch] == '\n') {
	    oy += 18 * zx;
	    ox = 0;
	  } else {
            for (int cy = 0; cy < 16; ++cy)
	      for (int cx = 0; cx < 8; ++cx) {
	       uint32_t colr = font[cx][cy][str[ch]] ? fgc : bgc;
               for (int px = 0; px < zx; ++px)
                 for (int py = 0; py < zy; ++py)
                 put_pixel(x+ox+cx*zx+px,y+oy+cy*zy+py, colr);
	      }
	    ox += 9 * zx;
	  }
	}
}

void clear_screen(void) {
	for (int y = 0; y < FBHEIGHT; ++y)
		for (int x = 0; x < FBWIDTH; ++x)
			put_pixel(x, y, 0xFF000000);
}

void fill_rect(int x, int y, int w, int h, uint32_t color) {
	for (int iy = 0; iy < h; ++iy)
		for (int ix = 0; ix < w; ++ix)
			put_pixel(x + ix, y + iy, color);
}

// string, X coord, Y coord, width, height, text size, text color, fill color, border color
void text_box(const char *s, int x, int y, int w, int h, int z, uint32_t tc, uint32_t fc, uint32_t bc) {
	rect(x, y, w, h, bc);
	fill_rect(x, y, w, h, fc);
	text(s, x+8, y+8, z, z, tc, fc);
}

void status(char *str) {
	++stat_page;
	stat_page %= 16;

	fill_rect(0,FBHEIGHT - 288 + stat_page*18, FBWIDTH,18, 0xFF4040C0);
	text(str, 8,FBHEIGHT - 288 + stat_page*18, 1, 1, 0xFFC0C0C0,0xFF4040C0);
}

void status_error(char *str) {
	++stat_page;
	stat_page %= 16;

	fill_rect(0,FBHEIGHT - 288 + stat_page*18, FBWIDTH,18, 0xFFFF0000);
	text(str, 8,FBHEIGHT - 288 + stat_page*18, 1, 1, 0xFFFFFFFF,0xFFFF0000);

	vibrate(160);
	usleep(240000);
	vibrate(80);
	usleep(120000);
	vibrate(80);
	usleep(120000);
	vibrate(80);
	usleep(1520000);
}

void message(char *str) {
	status(str);
	wait_touch();
}

void stprintf(const char *fmt, ...) {
	va_list args;
	char buf[128];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	status(buf);
}

void steprintf(const char *fmt, ...) {
	va_list args;
	char buf[128];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	status_error(buf);
}

void set_brightness(int bright) {
	int bright_fd, nchars;
	char bright_str[4];

	if (bright < 4) {
		status_error("can't set brightness < 4");
		return;
	}

	bright_fd = open("/sys/class/leds/lcd-backlight/brightness", O_WRONLY);
	if (bright_fd < 0) {
		stperror("error opening brightness");
		return;
	}
	nchars = snprintf(bright_str, sizeof(bright_str), "%d", bright);

	if (write(bright_fd, bright_str, nchars) != nchars)
		stperror("can't set brightness");

	close(bright_fd);
}

