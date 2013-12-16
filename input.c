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
#include <stdlib.h>
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
int ts_usable = 0;

int gpio_keys_fd;
int gpio_keys_usable = 0;

int pmic_keys_fd;
int pmic_keys_usable = 0;

int vibrator_fd;
int vibrator_usable = 0;

input_state last_input;
input_state cur_input;
input_state diff_input;

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
	} else {
		snprintf(dev_path, sizeof(dev_path), "/dev/%s", dev_name);
		free(dev_name);
		logprintf("0trying to open touch screen at %s", dev_path);
		if (!ts_open(dev_path)) 
			ts_usable = 1;
	}

	dev_name = name_to_event_dev("gpio-keys");
	if (dev_name == NULL) {
		logprintf("3unable to detect GPIO keys!");
	} else {
		snprintf(dev_path, sizeof(dev_path), "/dev/%s", dev_name);
		free(dev_name);
		logprintf("0trying to open GPIO keys at %s", dev_path);
		if (!gpio_keys_open(dev_path))
			gpio_keys_usable = 1;
	}

	dev_name = name_to_event_dev("pmic8058_pwrkey");
	if (dev_name == NULL) {
		logprintf("3unable to detect PMIC keys!");
	} else {
		snprintf(dev_path, sizeof(dev_path), "/dev/%s", dev_name);
		free(dev_name);
		logprintf("0trying to open PMIC keys at %s", dev_path);
		if (!pmic_keys_open(dev_path))
			pmic_keys_usable = 1;
	}

	if (!ts_usable && !(gpio_keys_usable && pmic_keys_usable)) {
		logprintf("3no usable input devices!");
		input_close();
		return 1;
	}

	memset(&last_input, 0, sizeof(last_input));
	memset(&cur_input, 0, sizeof(last_input));
	memset(&diff_input, 0, sizeof(last_input));

	return 0;
}

void input_read(void) {
	fd_set input_fds;
	struct timeval tv;
	int num_fds = 0;
	int status;
	FD_ZERO(&input_fds);

	last_input = cur_input;

	if (ts_usable) {
		FD_SET(ts_fd, &input_fds);
		num_fds = max(num_fds, ts_fd);
	}
	if (gpio_keys_usable) {
		FD_SET(gpio_keys_fd, &input_fds);
		num_fds = max(num_fds, gpio_keys_fd);
	}
	if (pmic_keys_usable) {
		FD_SET(pmic_keys_fd, &input_fds);
		num_fds = max(num_fds, pmic_keys_fd);
	}

	// these values directly affect the (minimum) frame rate
	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	status = select(num_fds, &input_fds, NULL, NULL, &tv);

	if (!status) {
		cur_input.timed_out = 1;
	} else if (status == -1) {
		logperror("select");
		sleep(1);
		return;
	} else {
		cur_input.timed_out = 0;
	}

	if (FD_ISSET(ts_fd, &input_fds))
		ts_read(&cur_input.ts_x, &cur_input.ts_y, &cur_input.touch);
	if (FD_ISSET(gpio_keys_fd, &input_fds))
		gpio_keys_read(&cur_input.vol_up, &cur_input.vol_down,
			&cur_input.center);
	if (FD_ISSET(pmic_keys_fd, &input_fds))
		pmic_keys_read(&cur_input.power);

	diff_input.touch = last_input.touch && !cur_input.touch;
	diff_input.vol_up = !last_input.vol_up && cur_input.vol_up;
	diff_input.vol_down = !last_input.vol_down && cur_input.vol_down;
	diff_input.center = !last_input.center && cur_input.center;
	diff_input.power = !last_input.power && cur_input.power;

	if (diff_input.touch)
		vibrate(30);
}

void input_close(void) {
	ts_close();
	gpio_keys_close();
	pmic_keys_close();

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

	sysfs_dir_entry = (struct dirent *)1;
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

int gpio_keys_open(char *dev_file) {
	gpio_keys_fd = open(dev_file, O_RDONLY);
	if (gpio_keys_fd < 0) {
		logperror("error opening GPIO key event device");
		return 1;
	}

	return 0;
}

int pmic_keys_open(char *dev_file) {
	pmic_keys_fd = open(dev_file, O_WRONLY);
	if (pmic_keys_fd < 0) {
		logperror("error opening PMIC key event device");
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
	if (ts_usable) {
		close(ts_fd);
		ts_usable = 0;
	}
}

void gpio_keys_close(void) {
	if (gpio_keys_usable) {
		close(gpio_keys_fd);
		gpio_keys_usable = 0;
	}
}

void pmic_keys_close(void) {
	if (pmic_keys_usable) {
		close(pmic_keys_fd);
		pmic_keys_usable = 0;
	}
}

void vibrator_close(void) {
	if (vibrator_usable) {
		close(vibrator_fd);
		vibrator_usable = 0;
	}
}

void ts_read(int *x, int *y, int *touch) {
	int got_syn;
	size_t read_size;
	struct input_event ev;

	if (!ts_usable) {
		*x = 0;
		*y = 0;
		*touch = 0;
		return;
	}

	got_syn = 0;
	while (!got_syn) {
		read_size = read(ts_fd, &ev, sizeof(struct input_event));
		if (read_size == -1) {
			logperror("error reading touchscreen (retrying)");
			continue;
		} else if (read_size < sizeof(struct input_event)) {
			logprintf("2short read on touchscreen (retrying");
			continue;
		}

		switch (ev.type) {
		case EV_SYN:
			switch (ev.code) {
			case SYN_REPORT:
				got_syn = 1;
				break;
			default:
				logprintf("0unknown input event, type EV_SYN "
					"code %x, value %x, from touchscreen",
					ev.code, ev.value);
				break;
			}
			break;
		
		case EV_ABS:
			switch (ev.code) {
			case ABS_X:
				*x = ev.value;
				break;
			case ABS_Y:
				*y = ev.value;
				break;
			case ABS_PRESSURE:
			case ABS_TOOL_WIDTH:
				break;
			default:
				logprintf("0unknown input event, type EV_ABS "
					"code %x, value %x, from touchscreen",
					ev.code, ev.value);
				break;
			}
			break;

		case EV_KEY:
			switch (ev.code) {
			case BTN_TOUCH:
				*touch = ev.value;
				break;
			default:
				logprintf("0unknown input event, type EV_KEY, "
					"code %x, value %x, from touchscreen",
					ev.code, ev.value);
			}
			break;

		default:
			logprintf("0unknown input event, type %x, code %x, "
				"value %x, from touchscreen",
				ev.type, ev.code, ev.value);
			break;
		}
	}
}

void gpio_keys_read(int *vol_up, int *vol_down, int *center) {
	struct input_event ev;
	size_t read_size;
	int got_syn;

	if (!gpio_keys_usable) {
		*vol_up = 0;
		*vol_down = 0;
		*center = 0;
		return;
	}

	got_syn = 0;
	while (!got_syn) {
		read_size = read(gpio_keys_fd, &ev, sizeof(struct input_event));
		if (read_size == -1) {
			logperror("error reading gpio_keys (retrying)");
			continue;
		} else if (read_size < sizeof(struct input_event)) {
			logprintf("2short read of gpio_keys (retrying)");
			continue;
		}

		switch (ev.type) {
		case EV_SYN:
			switch (ev.code) {
			case SYN_REPORT:
				got_syn = 1;
				break;
			default:
				logprintf("0unknown input event, type EV_SYN, "
					"code %x, value %x, from gpio_keys",
					ev.code, ev.value);
				break;
			}
			break;

		case EV_KEY:
			switch (ev.code) {
			case KEY_VOLUMEDOWN:
				*vol_down = ev.value;
				break;
			case KEY_VOLUMEUP:
				*vol_up = ev.value;
				break;
			case KEY_REPLY:
				*center = ev.value;
				break;
			default:
				logprintf("0unknown input event, type EV_KEY, "
					"code %x, value %x, from gpio_keys",
					ev.code, ev.value);
				break;
			}
			break;
	
		default:
			logprintf("0unknown input event, type %x, code %x, "
				"value %x, from gpio_keys", ev.type, ev.code,
				ev.value);
			break;
		}
	}
}

void pmic_keys_read(int *power) {
	struct input_event ev;
	size_t read_size;
	int got_syn;

	if (!pmic_keys_usable) {
		*power = 0;
		return;
	}

	got_syn = 0;
	while (!got_syn) {
		read_size = read(pmic_keys_fd, &ev, sizeof(struct input_event));
		if (read_size == -1) {
			logperror("error reading pmic_keys (retrying)");
			continue;
		} else if (read_size < sizeof(struct input_event)) {
			logprintf("2short read on pmic_keys (retrying)");
			continue;
		}

		switch (ev.type) {
		case EV_SYN:
			switch (ev.code) {
			case SYN_REPORT:
				got_syn = 1;
				break;
			default:
				logprintf("0unknown input event, type EV_SYN, "
					"code %x, value %x, on pmic_keys",
					ev.code, ev.value);
				break;
			}
			break;

		case EV_KEY:
			switch (ev.code) {
			case KEY_END:
				*power = ev.value;
				break;
			default:
				logprintf("0unknown input event, type EV_KEY, "
					"code %x, value %x, on pmic_keys",
					ev.code, ev.value);
				break;
			}
			break;

		default:
			logprintf("0unknown input event, type %x, code %x, "
				"value %x, on pmic_keys", ev.type, ev.code,
				ev.value);
			break;
		}
	}
}
	
void vibrate(int ms) {
	char str_ms[8];

	if (!vibrator_usable)
		return;

	snprintf(str_ms, sizeof(str_ms), "%d\n", ms);

	if (write(vibrator_fd, str_ms, 8) < 2)
		logperror("can't write to vibrator");
}
