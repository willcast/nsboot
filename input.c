/*
	nsboot/input.c
	input functionality for nsboot

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

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <linux/input.h>

#include <sys/file.h>
#include <sys/stat.h>

#include "input.h"

int ts_fd;

int ts_open(char *dev_file) {
	ts_fd = open(dev_file, O_RDONLY);
	if (ts_fd < 0) return 1;
	flock(ts_fd, LOCK_EX);
	return 0;
}

void ts_close(void) {
	flock(ts_fd, LOCK_UN);
	close(ts_fd);
}

void ts_read(int *x, int *y) {
	int ts_x, ts_y, ts_id, ts_maj, ts_syn = 0, nbytes;
	struct input_event ev;
	static struct timeval ts_time, ts_last_time = {0,0}, ts_diff_time;

	while ((ts_maj != 0) || (ts_id != 1) || (!ts_syn)) {
		nbytes = read(ts_fd, &ev, sizeof(struct input_event));
		if (nbytes != sizeof(struct input_event)) {
			perror("can't read touchscreen");
			continue;
		}

		if (ev.type == EV_SYN) ts_syn = 1;
		else if (ev.type == EV_ABS) {
			switch (ev.code) {
			case ABS_MT_TRACKING_ID:
				ts_id = ev.value;
				break;
			case ABS_MT_TOUCH_MAJOR:
				ts_maj = ev.value;
				break;
			case ABS_MT_POSITION_X:
				ts_x = ev.value;
				break;
			case ABS_MT_POSITION_Y:
				ts_y = ev.value;
				break;
			}
		}
	}
	vibrate(40);
	usleep(50000);
	*x = ts_x;
	*y = ts_y;
}

void wait_touch(void) {
	int dummy;
	ts_read(&dummy, &dummy);
}

void vibrate(int ms) {
	int vibrator_fd;
	char str_ms[8];

	vibrator_fd = open("/sys/devices/virtual/timed_output/vibrator/enable",
		O_WRONLY);
	if (vibrator_fd < 0) {
		perror("can't open vibrator");
		return;
	}

	sprintf(str_ms, "%d", ms);

	if (write(vibrator_fd, str_ms, 8) < 2)
		perror("can't write to vibrator");
	close(vibrator_fd);
}
