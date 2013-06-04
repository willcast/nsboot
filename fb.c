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
#include "log.h"
#include "lib.h"

int fb_fd;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
char *fbp = NULL;
int stat_page = 0;
long screen_size = 0;
uint8_t font[8][16][256];

int is_fb_available(void) {
	return !(fbp == NULL);
}

int out_fd, err_fd, save_out, save_err;

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
	ditch_stdouterr();

	return 0;
}

void fb_close(void) {
	clear_screen();
	munmap(fbp, screen_size);
	close(fb_fd);
	fbp = NULL;
	reclaim_stdouterr();
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

void set_brightness(int bright) {
	if (bright < 4) {
		logprintf("1can't set brightness < 4");
		return;
	}
	if (bright > 255) {
		logprintf("1can't set brightness > 255");
		return;
	}

	qfprintf("/sys/class/leds/lcd-backlight/brightness", "%d", bright);
}

void ditch_stdouterr(void) {
    out_fd = open("/var/nsboot.out", O_RDWR|O_CREAT|O_APPEND, 0644);
    if (-1 == out_fd) { logperror("opening nsboot.err"); return; }

    err_fd = open("/var/nsboot.err", O_RDWR|O_CREAT|O_APPEND, 0644);
    if (-1 == err_fd) { logperror("opening nsboot.err"); return; }

    save_out = dup(fileno(stdout));
    save_err = dup(fileno(stderr));

    if (-1 == dup2(out_fd, fileno(stdout))) { logperror("cannot redirect stdout"); return; }
    if (-1 == dup2(err_fd, fileno(stderr))) { logperror("cannot redirect stderr"); return; }
}

void reclaim_stdouterr(void) {
    fflush(stdout); close(out_fd);
    fflush(stderr); close(err_fd);

    dup2(save_out, fileno(stdout));
    dup2(save_err, fileno(stderr));

    close(save_out);
    close(save_err);
}
