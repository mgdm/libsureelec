#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <time.h>
#include "libsureelec.h"
    

void print_usage(const char *bin_name) {
    printf("Usage: %s DEVICE\n", bin_name);
}

char *print_uptime(long up_secs, unsigned char *retval) {
    long days, hours, mins, secs;
    if (retval == NULL) {
       retval = malloc(20 * sizeof(unsigned char));
    }

    mins = up_secs / 60;
    hours = mins / 60;
    hours = hours % 24;

    days = up_secs / (60 * 60 * 24);

    mins = (int) up_secs / 60;
    hours = mins / 60;
    hours = hours % 24;
    mins = mins % 60;
    secs = up_secs % 60;

    snprintf(retval, 20, "Up %ldd, %02ld:%02ld:%02ld", days, hours, mins, secs);
    return retval;
}

int main(int argc, char **argv) {
    libsureelec_ctx *ctx;
    int i;
    struct sysinfo sys_info;
    char hostname[20];
    unsigned char *string, *uptime_string;

    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    ctx = libsureelec_create(argv[1], 0);
    
    if (ctx == NULL) {
        printf("Failed to initialize context\n");
        return -1;
    }

    libsureelec_set_contrast(ctx, 1);
    libsureelec_set_brightness(ctx, 254);
    libsureelec_clear_display(ctx);

    gethostname(hostname, 20);
    libsureelec_write_line(ctx, hostname, 1);
    
    string = (unsigned char *) calloc(20, sizeof(unsigned char));
    uptime_string = (unsigned char *) calloc(20, sizeof(unsigned char));
    while(1) {
        long foo = libsureelec_get_temperature(ctx);
        sysinfo(&sys_info);

        snprintf(string, 20, "Temp is %ld deg C", foo);
        libsureelec_write_line(ctx, string, 2);

        uptime_string = print_uptime(sys_info.uptime, uptime_string);
        libsureelec_write_line(ctx, uptime_string, 3);

        snprintf(string, 20, "Load %.2lf %.2lf %.2lf", 
                sys_info.loads[0] / 65536.0, 
                sys_info.loads[1] / 65536.0, 
                sys_info.loads[2] / 65536.0);
        libsureelec_write_line(ctx, string, 4);

        usleep(50000);
    }

    libsureelec_destroy(ctx);
}
