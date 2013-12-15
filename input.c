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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <linux/input.h>
#include <linux/limits.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "input.h"
#include "lib.h"
#include "log.h"

int ts_fd;

int vibrator_fd;
int vibrator_usable = 0;

int input_open(void) {
	char dev_path[PATH_MAX];
	char *dev_name = NULL;
	int attempt;

	for (attempt = 0; attempt < 5; ++attempt) {
		usleep(333333);
		logprintf("0locating touch screen (attempt %d)", attempt + 1);
		dev_name = name_to_event_dev("HPTouchpad");
		if (dev_name != NULL)
			break;
	}
	if (attempt == 5) {
		logprintf("3unable to detect touch screen!");
		return 1;
	}

	snprintf(dev_path, sizeof(dev_path), "/dev/%s", dev_name);

	free(dev_name);

	logprintf("0trying to open touch screen at %s", dev_path);
	if (ts_open(dev_path))
		return 1;

	return 0;
}

void input_close(void) {
	ts_close();

	qfprintf("/sys/devices/platform/cy8ctma395/vdd", "0");
}

char * name_to_event_dev(char *match_name) {
	DIR *sysfs_dir;
	struct dirent *sysfs_dir_entry;

	errno = 0;
	sysfs_dir = opendir("/sys/class/input");
	if (sysfs_dir == NULL) {
		logperror("error opening sysfs input class dir");
		return NULL;
	}

	sysfs_dir_entry = (DIR *)99;
	while (sysfs_dir_entry != NULL) {
		char name_file_path[PATH_MAX];
		char cur_name[256];
			
		errno = 0;
		sysfs_dir_entry = readdir(sysfs_dir);
		if (errno) {
			logperror("error reading sysfs dir entry");
			closedir(sysfs_dir);
			return NULL;
		}

		if (strncmp("event", sysfs_dir_entry->d_name, 5))
			continue;	

		snprintf(name_file_path, sizeof(name_file_path),
			"/sys/class/input/%s/device/name",
			sysfs_dir_entry->d_name);

		if (test_file(name_file_path))
			qfscanf(name_file_path, "%s", cur_name);

		if (!strcmp(cur_name, match_name)) {
			closedir(sysfs_dir);

			return strdup(sysfs_dir_entry->d_name);
		}
	}

	closedir(sysfs_dir);
	return NULL;
}

int ts_open(char *dev_file) {
	ts_fd = open(dev_file, O_RDONLY);
	if (ts_fd < 0) {
		logperror("error opening touchscreen event device");
		return 1;
	}

	return 0;
}

void vibrator_open(char *dev_file) {
	vibrator_fd = open(dev_file, O_WRONLY);
	if (vibrator_fd < 0)
		logperror("error opening vibrator sysfs file");
	else
		vibrator_usable = 1;
}

void ts_close(void) {
	close(ts_fd);

}

void vibrator_close(void) {
	if (vibrator_usable) {
		close(vibrator_fd);
		vibrator_usable = 0;
	}
}


void ts_read(int *x, int *y) {
	int ts_x = -1, ts_y = -1;
	int ts_touch = 0;
	int ts_syn = 0;
	int nbytes;
	struct input_event ev;

	while ((!ts_syn) || (ts_x == -1) || (ts_y == -1) || (!ts_touch)) {
		nbytes = read(ts_fd, &ev, sizeof(struct input_event));
		if (nbytes != sizeof(struct input_event)) {
			perror("error reading touchscreen (will retry)");
			continue;
		}

		switch (ev.type) {
		case EV_SYN:
			switch (ev.code) {
			case SYN_REPORT:
				ts_syn = 1;
				break;
			default:
				logprintf("0unknown code in EV_SYN event on touchscreen");
			}
			break;
		
		case EV_ABS:
			switch (ev.code) {
			case ABS_X:
				ts_x = ev.value;
				break;
			case ABS_Y:
				ts_y = ev.value;
				break;
			case ABS_PRESSURE:
				break;
			default:
				logprintf("0unknown code in EV_ABS event on touchscreen");
			}
			break;

		case EV_KEY:
			switch (ev.code) {
			case BTN_TOUCH:
				ts_touch = ev.value;
				break;
			default:
				logprintf("0unknown code in EV_KEY event on touchscreen");
			}
			break;

		default:
			logprintf("0unknown type in event on touchscreen");
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
	char str_ms[8];

	if (!vibrator_usable)
		return;

	snprintf(str_ms, sizeof(str_ms), "%d\n", ms);

	if (write(vibrator_fd, str_ms, 8) < 2)
		logperror("can't write to vibrator");
}
