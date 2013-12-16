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

#ifndef NSBOOT_INPUT_H
#define NSBOOT_INPUT_H

#define is_in(x, y, w, h) ((cur_input.ts_x >= x) && (cur_input.ts_x <= x+w) \
        && (cur_input.ts_y >= y) && (cur_input.ts_y <= y+h) \
	&& (last_input.ts_x >= x) && (last_input.ts_x <= x+w) \
	&& (last_input.ts_y >= y) && (last_input.ts_y <= y+h))

#define is_held(x, y, w, h) ((is_in(x, y, w, h)) && cur_input.touch)
#define was_lifted(x, y, w, h) ((is_in(x, y, w, h)) && !diff_input.touch)
#define was_clicked(x, y, w, h) ((is_in(x, y, w, h)) && diff_input.touch)

typedef struct {
	int ts_x;
	int ts_y;
	int touch;
	int vol_up;
	int vol_down;
	int center;
	int power;

	int timed_out;
} input_state;

extern input_state last_input;
extern input_state cur_input;
extern input_state diff_input;

int input_open(void);
void input_close(void);
char * name_to_event_dev(char *);

int ts_open(char *);
void ts_read(int *, int *, int *);
void ts_close(void);
extern int ts_usable;

int gpio_keys_open(char *);
void gpio_keys_read(int *, int *, int *);
void gpio_keys_close(void);
extern int gpio_keys_usable;

int pmic_keys_open(char *);
void pmic_keys_read(int *);
void pmic_keys_close(void);
extern int pmic_keys_usable;

void vibrator_open(char *);
void vibrator_close(void);
void vibrate(int);
extern int vibrator_usable;

#endif
