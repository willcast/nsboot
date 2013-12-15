/*
	nsboot/lib.c
	globally useful functions for nsboot

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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/limits.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include "lib.h"
#include "log.h"

void qfprintf(const char *fname, const char *fmt, ...) {
	va_list args;
	FILE *fp;

	fp = fopen(fname, "w");
	if (fp == NULL) {
		logprintf("2 can't open %s: %s", fname,
			strerror(errno));
		return;
	}

	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

	fclose(fp);
}

void qfscanf(const char *fname, const char *fmt, ...) {
	va_list args;
	FILE *fp;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		logprintf("2 can't open %s: %s", fname,
			strerror(errno));
		return;
	}

	va_start(args, fmt);
	vfscanf(fp, fmt, args);
	va_end(args);

	fclose(fp);
}

void qpscanf(const char *cmd, const char *fmt,  ...) {
	va_list args;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp == NULL) {
		logprintf("2 can't popen %s: %s", cmd,
			strerror(errno));
		return;
	}

	va_start(args, fmt);
	vfscanf(fp, fmt, args);
	va_end(args);

	pclose(fp);	
}

int hexval(char c) {
	if (c <= '9' && c >= '0') {
		return (c - '0');
	}
	if (c <= 'f' && c >= 'a') {
		return (c - 'a' + 10);
	}
	if (c <= 'F' && c >= 'A') {
		return (c - 'A' + 10);
	}
	return -1;
}

int test_file(const char *path) {
	struct stat test;

	if (stat(path, &test) == -1) return 0;
	return 1;
}

void cp_file(const char *src, const char *dest) {
	int src_fd, dest_fd;
	char *buf;
	size_t size;

	if (!test_file(src)) {
		logprintf("1file %s doesn't exist, so can't copy", src);
		return;
	}

	buf = malloc(BUF_SIZE);
	if (buf == NULL) {
		logperror("2can't allocate buffer");
		return;
	}

	src_fd = open(src, O_RDONLY);
	if (src_fd < 0) {
		logperror("2can't open source file");
		return;
	}

	dest_fd = open(dest, O_WRONLY);
	if (dest_fd < 0) {
		logperror("2can't open destination file");
		close(src_fd);
		return;
	}

	do {
		size = read(src_fd, buf, BUF_SIZE);
		if (size == -1) {
			logperror("error reading from source file");
			break;
		}

		write(dest_fd, buf, size);
		if (size == -1) {
			logperror("error writing to destination file");
			break;
		}
	} while (size == BUF_SIZE);

	close(src_fd);
	close(dest_fd);
	free(buf);
}

void system_logged(char *cmd) {
	pid_t code;
	uint8_t exit_status;
	int sig_num;
	char argv_zero[PATH_MAX];
	char core_path[PATH_MAX];
	char *exe_name;
	char *space;

	assert(cmd != NULL);
	strncpy(argv_zero, cmd, sizeof(argv_zero));
	argv_zero[PATH_MAX-1] = '\0';
	space = strchr(argv_zero, ' ');
	if (space != NULL)
		*space = '\0';

	exe_name = basename(argv_zero);
	
	code = system(cmd);

	if (WIFEXITED(code)) {
		exit_status = WEXITSTATUS(code);

		if (exit_status) {
			logprintf("2command %s died with code %d\n", 
				exe_name, exit_status);
			return;
		}
	} else if (WIFSIGNALED(code)) {
		sig_num = WTERMSIG(code);
		if (WCOREDUMP(code)) {
			logprintf("2command %s died from signal %d and dumped core\n",
				exe_name, sig_num);

			snprintf(core_path, sizeof(core_path), "/var/core-%s",
				exe_name);
			cp_file(core_path, "/mnt/media/nsboot/logs");
			logprintf("0the core dump was copied to /mnt/media/nsboot/logs/core-%s",
				exe_name);
			return;
		} else {
			logprintf("2command %s died from signal %d (but no core was produced)",
				exe_name, sig_num);
			return;
		}
	} else {
		logprintf("2something evil happened to command %s", exe_name);
	}
}

void sysprintf(const char *fmt, ...) {
	va_list args;
	char *buf;
	size_t size;

	va_start(args, fmt);
	size = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	buf = malloc(size + 1);
	if (buf == NULL) {
		logperror("command buffer allocation failed");
		return;
	}

	va_start(args, fmt);
	vsnprintf(buf, size + 1, fmt, args);
	va_end(args);

	system_logged(buf);

	free(buf);
}
