#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "libsureelec.h"

libsureelec_ctx *ctx;

void print_usage(char *bin_name) {
    printf("Usage: %s DEVICE\n", bin_name);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    ctx = libsureelec_create(argv[1], 0);

    if (ctx == NULL) {
        printf("Failed to connect to device %s\n", argv[1]);
        return -1;
    }

    libsureelec_set_contrast(ctx, 1);
    libsureelec_set_brightness(ctx, 254);
    libsureelec_clear_display(ctx);

    int current = 0;
    char buf[20];
    while (1) {
        char *input = readline("> ");
        size_t len;
        if (!input) {
            break;
        }

        memset(buf, ' ', 20);
        len = strlen(input);
        memcpy(buf, input, (len > 20) ? 20 : len);
        printf("Writing %s to screen line %d\n", input, current + 1);
        libsureelec_display_line(ctx, buf, current + 1);
        current = (current + 1) % 4;
    }
}
