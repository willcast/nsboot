/*
	nsboot/boot.h
	header of functions to handle boot items

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

#ifndef NSBOOT_CFG_H
#define NSBOOT_CFG_H

#define PROC_CMDLINE_MAX 512

struct boot_item {
	char *label;
	char *cfgdev;
	char *kernel;
	char *initrd;
	char *append;
	int prio;
};

struct boot_item * add_boot_item(char *);

void parse_proc_cmdline(void);
void check_keep_arg(char *);
void read_kb_file(char *);

void scan_boot_lvs(void);
void free_boot_items(void);
void boot_kexec(int);

int nduid_to_serialno(char *, char *);
int hexval(char);

#endif
