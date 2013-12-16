/*
	nsboot/osk.c
	onscreen keyboard and numpad implementation

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

#include <stdlib.h>
#include <string.h>

#include "fb.h"
#include "input.h"
#include "log.h"

const char keyboard[4][4][14][2] = { {
	{ "`","1","2","3","4","5","6","7","8","9","0","-","=","\\" },
	{ "", "q","w","e","r","t","y","u","i","o","p","[","]", ""  },
	{ "", "a","s","d","f","g","h","j","k","l",";","'", "", ""  },
 	{ "", "z","x","c","v","b","n","m",",",".","/", "", "", ""  }
}, {
	{ "~","!","#","@","$","%","^","&","*","(",")","_","+","|" },
	{ "", "Q","W","E","R","T","Y","U","I","O","P","{","}",""  },
	{ "", "A","S","D","F","G","H","J","K","L",":","\"","",""  },
	{ "", "Z","X","C","V","B","N","M","<",">","?", "", "",""  }
}, {
	{ "`","1","2","3","4","5","6","7","8","9","0","-","=","\\" },
	{ "", "Q","W","E","R","T","Y","U","I","O","P","[","]",""   },
	{ "", "A","S","D","F","G","H","J","K","L",";","'","", ""   },
	{ "", "Z","X","C","V","B","N","M",",",".","/", "", "",""   }
}, {
	{ "~","!","#","@","$","%","^","&","*","(",")","_","+","|" },
	{ "", "q","w","e","r","t","y","u","i","o","p","{","}", ""  },
	{ "", "a","s","d","f","g","h","j","k","l",":","\"", "", ""  },
 	{ "", "z","x","c","v","b","n","m","<",">","?", "", "", ""  }
} };

// num_input: display modal numeric entry screen (numpad)
// returns the selected number
// const char *prompt: message to display above input area.
long num_input(const char *prompt) {
	int done = 0, cur_ch = 0;
	char buf[12];

	memset(buf, '\0', sizeof(buf));

	while (!done) {
		if (cur_ch > sizeof(buf) - 2)
			cur_ch = sizeof(buf) - 2;

		clear_screen();

		text(prompt, 16,16, 2,2, 0xFFFFFFFF, 0xFF000000);
		text(buf, 16,68, 6,6, 0xFFFFFFFF, 0xFF000000);

		text_box("7", 16,192, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("8", 156,192, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("9", 296,192, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("4", 16,332, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("5", 156,332, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("6", 296,332, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("1", 16,472, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("2", 156,472, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("3", 296,472, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("back", 16,612, 124,124, 1,
			0xFFFFFFFF,0xFF000000,0xFFFFFFFF);
		text_box("0", 156,612, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("enter", 296,612, 124,124, 1,
			0xFFFFFFFF,0xFF000000,0xFFFFFFFF);

		update_screen();
		input_read();

		if (was_clicked(16, 174, 124, 124)) buf[cur_ch++] = '7';
		else if (was_clicked(156, 192, 124, 124)) buf[cur_ch++] = '8';
		else if (was_clicked(296, 192, 124, 124)) buf[cur_ch++] = '9';
		else if (was_clicked(16, 332, 124, 124)) buf[cur_ch++] = '4';
		else if (was_clicked(156, 332, 124, 124)) buf[cur_ch++] = '5';
		else if (was_clicked(296, 332, 124, 124)) buf[cur_ch++] = '6';
		else if (was_clicked(16, 472, 124, 124)) buf[cur_ch++] = '1';
		else if (was_clicked(156, 472, 124, 124)) buf[cur_ch++] = '2';
		else if (was_clicked(296, 472, 124, 124)) buf[cur_ch++] = '3';
		else if (was_clicked(16, 612, 124, 124)) {
			if ((buf[cur_ch] == '\0') && cur_ch) --cur_ch;
			buf[cur_ch] = '\0';
		} else if (was_clicked(156, 612, 124, 124)) buf[cur_ch++] = '0';
		else if (was_clicked(296, 612, 124, 124)) done = 1;
	}

	return atol(buf);
}

// text_input: display modal string entry screen (keyboard)
// returns the string that was entered
// const char *prompt: message to display above the input field.
// FIXME: limited to one line of input:
//        with standard 8x16 font on a 1024px wide screen, this means
//        a hard limit of 128 visible characters!
char * text_input(const char *prompt) {
	int ts_x, ts_y;
	int done = 0;
	char *output;
	int cur_ch = 0, shift = 0, caps = 0, n;

	output = malloc(128 * sizeof(char));;
	if (output == NULL) {
		logperror("can't allocate textbuf");
		return NULL;
	}
	memset(output, '\0', 128 * sizeof(char));

	while (!done) {
		n = shift + caps*2;
		if (cur_ch > 126) cur_ch = 126;
		clear_screen();

		text(prompt, 16,16, 2,2, 0xFFFFFFFF, 0xFF000000);
		text(output, 0,68, 1,1, 0xFF808080, 0xFF000000);

		for (int y = 0; y < 4; ++y) for (int x = 0; x < 14; ++x)
			if (keyboard[n][y][x][0] != '\0')
				text_box(keyboard[n][y][x], 16+64*x,384+64*y, 52,52,
					2, 0xFFFFFFFF,0xFF808080,0xFFFFFFFF);

		text_box("back", 912,384, 52,52, 1,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("caps", 16,512, 52,52, 1,
			caps ? 0xFFFFFFFF : 0xFF000000,
			caps ? 0xFF000000 : 0xFFFFFFFF,
			caps ? 0xFFFFFFFF : 0xFF000000);
		text_box("enter", 784,512, 116,52, 2,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("shift", 16,576, 52,52, 1,
			shift ? 0xFFFFFFFF : 0xFF000000,
			shift ? 0xFF000000 : 0xFFFFFFFF,
			shift ? 0xFFFFFFFF : 0xFF000000);
		text_box("shift", 784,576, 116,52, 2,
			shift ? 0xFFFFFFFF : 0xFF000000,
			shift ? 0xFF000000 : 0xFFFFFFFF,
			shift ? 0xFFFFFFFF : 0xFF000000);
		text_box("", 208,640, 448,52, 1,
			0xFF000000,0xFFFFFFFF,0xFF000000);

		update_screen();
		input_read();

		for (int y = 0; y < 4; ++y) for (int x = 0; x < 14; ++x)
			if (was_clicked((16+64*x), (384+64*y), 52,52) &&
				(keyboard[n][y][x][0] != '\0')) {
					output[cur_ch] = keyboard[n][y][x][0];
					++cur_ch;
					shift = 0;
					break;
			}

		if (was_clicked(912, 384, 52, 52)) {
			if ((output[cur_ch] == '\0') && cur_ch) --cur_ch;
			output[cur_ch] = '\0';
		} else if (was_clicked(16, 512, 52, 52))
			caps = !caps;
		else if (was_clicked(784, 512, 116, 52))
			done = 1;
		else if (was_clicked(16, 576, 116, 52) || was_clicked(784, 576, 116, 52))
			shift = !shift;
		else if (was_clicked(208, 640, 448, 52))
			output[cur_ch++] = ' ';

	}

	return output;
}
