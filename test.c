#include <stdlib.h>
#include <stdio.h>
#include "libsureelec.h"

int main(int argc, char **argv) {
    libsureelec_ctx *ctx;
    char *data;

    ctx = libsureelec_create("/dev/ttyUSB0", 1);
 //   data = libsureelec_get_screen_size(ctx);
//    printf("%s\n", data);
    
    if (ctx == NULL) {
        printf("Failed to initialize context\n");
        return -1;
    }

    libsureelec_write_line(ctx, "Hello world!", 2);
    libsureelec_destroy(ctx);
}
