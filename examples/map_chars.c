#include <stdlib.h>
#include <stdio.h>

#include "libsureelec.h"

libsureelec_ctx *display;

int main(int argc, char **argv) {
	int page, line, character;
	unsigned char base;
	char chars[20];

	if (argc < 2) {
		fprintf(stderr, "Need device name\n");
		return -1;
	}

	display = libsureelec_create(argv[1], 0);

	if (display == NULL) {
		fprintf(stderr, "Failed to open display\n");
	}

	libsureelec_set_contrast(display, 1);
	libsureelec_set_brightness(display, 255);

	base = 32;
	for (page = 1; page < 5; page++) {
		printf("Page %d - characters %u to %u\n", page, base, base + 40);
		libsureelec_clear_display(display);

		for (line = 1; line <= 4; line++) {
			for (character = 0; character < 20; character++) {
				chars[character] = base;
				base++;
			}

			libsureelec_display_line(display, line, chars);
		}

		sleep(5);
	}
}
