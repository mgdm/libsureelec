#include <stdlib.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include "libsureelec.h"

int main(int argc, char **argv) {
    libsureelec_ctx *ctx;
    char *data;
    int i;
    struct sysinfo sys_info;

    ctx = libsureelec_create("/dev/ttyUSB0", 0);
    
    if (ctx == NULL) {
        printf("Failed to initialize context\n");
        return -1;
    }

    libsureelec_set_contrast(ctx, 1);
    libsureelec_set_brightness(ctx, 254);
    libsureelec_clear_display(ctx);

    sysinfo(&sys_info);

    long foo = libsureelec_get_temperature(ctx);
    unsigned char *string = calloc(25, sizeof(unsigned char));
    sprintf(string, "Temp is %ld degrees C", foo);
    libsureelec_write_line(ctx, string, 1);
    
    sprintf(string, "Uptime is %ld", sys_info.uptime);
    libsureelec_write_line(ctx, string, 2);

    libsureelec_write_line(ctx, "Driven by", 3);
    libsureelec_write_line(ctx, "  mgdm's libsureelec", 4);

    unsigned char *info = libsureelec_get_device_info(ctx);
    printf("Device info is: %s\n", info);
    libsureelec_destroy(ctx);
}
