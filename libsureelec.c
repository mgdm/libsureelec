/* libsureelec.c
 * Driver for SureElec LCD modules
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <sys/select.h>

#include "libsureelec.h"

static int libsureelec_debug_mode = 0;

static void libsureelec_log(const char *format, ...) {
    if (libsureelec_debug_mode) {
		va_list ap;
		va_start(ap, format); /* measure the required size (the number of elements of format) */

        fprintf( stderr, "libsureelec: " );
        vfprintf( stderr, format, ap );
        fprintf( stderr, "\n" );
    }
}

static int libsureelec_read(libsureelec_ctx *ctx, void *buf, size_t count) {
    fd_set rfds;
    struct timeval tv;
    int read_count;

    /* Watch fd to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(ctx->fd, &rfds);

    read_count = 0;
    while (read_count < count) {
        int retval;

        /*
         * For compatibility, better to reset the timeout delay (may
         * be modified in select() call).
         */
        tv.tv_sec = 1;	/* seconds */
        tv.tv_usec = 0;	/* microseconds */

        retval = select(ctx->fd + 1, &rfds, NULL, NULL, &tv);
        if (retval) {
            int read_result = read(ctx->fd, ((char *)buf) + read_count, count - read_count);
            if (read_result < 0) {
                libsureelec_log("Got no response: %s", strerror(errno));
                return -1;
            } else {
                read_count += read_result;
                libsureelec_log("Got %d bytes, up to %d", read_result, read_count);
            }
        } else {
            libsureelec_log("No answer from device");
            return -1;
        }
    }
    return read_count;
}

static int libsureelec_write(libsureelec_ctx *ctx, const char *seq, int count) {
    int written, written_count = 0;
    while (written_count < count) {
        written = write(ctx->fd, seq, count - written_count);
        if (written == -1) {
            libsureelec_log("Cannot write to port: %s", strerror(errno));
            return -1;
        }
        written_count += written;
    }

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 25000000;
    nanosleep(&ts, NULL);
    return written_count;
}

static int libsureelec_write_char(libsureelec_ctx *ctx, const char seq) {
    int written = 0;

    written = write(ctx->fd, &seq, 1);
    if (written == -1) {
        libsureelec_log("Cannot write to port: %s", strerror(errno));
        return -1;
    }

    return 1;
}

LIBSUREELEC_EXPORT libsureelec_ctx* libsureelec_create(const char *device, int debug) {
    char init_seq[5] = {'\xFE', 'S', 'u', 'r', 'e'};
    libsureelec_ctx *ctx = (libsureelec_ctx *) calloc(1, sizeof(libsureelec_ctx));
    ctx->fd = -1;
    struct termios port_config;

    if (debug > 0) {
        libsureelec_debug_mode = 1;
    } else {
        libsureelec_debug_mode = 0;
    }

    ctx->fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (ctx->fd == -1) {
        libsureelec_log("Failed to open port %s: ", device, strerror(errno));
        return NULL;
    }

    if (!isatty(ctx->fd)) {
       libsureelec_log("Device %s is not a TTY", device);
       return NULL;
    } 

    tcgetattr(ctx->fd, &port_config);

	port_config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
			     | INLCR | IGNCR | ICRNL | IXON);
	port_config.c_oflag &= ~OPOST;
//	port_config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	port_config.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | CSTOPB);
	port_config.c_cflag |= CS8 | CREAD | CLOCAL;
	port_config.c_cc[VMIN] = 1;
	port_config.c_cc[VTIME] = 0;

    port_config.c_lflag = 0;

    if(cfsetispeed(&port_config, B9600) < 0 || cfsetospeed(&port_config, B9600) < 0) {
        libsureelec_log("Failed to set port speed");
        return NULL;
    }

    if(tcsetattr(ctx->fd, TCSANOW, &port_config) < 0) {
        libsureelec_log("Failed to apply port configuration to device %s", device);
        return NULL;
    }
    
    /* LCD is on */
    ctx->display_state = 1;
    
    /* Send init sequence */
    libsureelec_log("Sending init sequence to %s", device);
    libsureelec_write(ctx, init_seq, sizeof(init_seq));

    libsureelec_get_device_info(ctx, &ctx->device_info);
    /* Set up framebuffer */
    ctx->framebuffer_size = ctx->device_info.width * ctx->device_info.height;
    ctx->framebuffer = malloc(ctx->framebuffer_size * sizeof(char));
    libsureelec_log("Framebuffer size is %d chars\n", ctx->framebuffer_size);

    memset(ctx->framebuffer, ' ', ctx->framebuffer_size);
    return ctx;
}

LIBSUREELEC_EXPORT void libsureelec_destroy(libsureelec_ctx *ctx) {
    if (ctx->fd != -1) {
        close(ctx->fd);
    }

    if (ctx->framebuffer != NULL) {
        free(ctx->framebuffer);
    }
        
    free(ctx);
}

LIBSUREELEC_EXPORT void libsureelec_clear_display(libsureelec_ctx *ctx) {
    int line;
    memset(ctx->framebuffer, ' ', ctx->framebuffer_size);

    for (line = 0; line < ctx->device_info.height; line++) {
        libsureelec_display_line(ctx, line + 1, ctx->framebuffer + (ctx->device_info.width * line));
    }
}

LIBSUREELEC_EXPORT int libsureelec_display_line(libsureelec_ctx *ctx, int line, const char *data) {

    int data_size;
	char cmd[4] = {'\xFE', '\x47', '\x01', 0};
    char *dest;

    if (line < 1 || line > ctx->device_info.height) {
        return -1;
    }

    if (data == NULL) {
        /* We're just refreshing the display, so write what's in the framebuffer */
        dest = &ctx->framebuffer[(line - 1) * ctx->device_info.width];
    } else {
        data_size = strlen(data);
        if (data_size > ctx->device_info.width) {
            data_size = ctx->device_info.width;
        }

        dest = ctx->framebuffer + (ctx->device_info.width * (line - 1));
        memset(dest, ' ', ctx->device_info.width);
        dest = memcpy(dest, data, data_size);
    }

    cmd[3] = line; 
    libsureelec_write(ctx, cmd, sizeof(cmd));
    libsureelec_write(ctx, dest, ctx->device_info.width);
    return 0;
}

LIBSUREELEC_EXPORT int libsureelec_get_device_info(libsureelec_ctx *ctx, libsureelec_device_info *device_info) {
    const char cmd[2] = { '\xFE', '\x76' };
    char buf[11], temp[4];
    char *end_ptr;

    libsureelec_write(ctx, cmd, sizeof(cmd));
    libsureelec_read(ctx, &buf, sizeof(buf));

    errno = 0;
    memset(temp, '\0', sizeof(temp));
    memcpy(temp, buf, 2);
    device_info->width = strtol(temp, &end_ptr, 10);
    if (errno != 0 || *end_ptr != 0 || end_ptr == (char *) buf) {
        libsureelec_log("Failed to get device info");
        return -1;
    }
    
    errno = 0;
    memset(temp, '\0', sizeof(temp));
    memcpy(temp, buf + 2, 2);
    device_info->height = strtol(temp, &end_ptr, 10);
    if (errno != 0 || *end_ptr != 0 || end_ptr == (char *) buf) {
        libsureelec_log("Failed to get device info");
        return -1;
    }

    if (buf[4] == 1) {
        device_info->has_rx8025 = 1;
    } else {
        device_info->has_rx8025 = 0;
    }
    libsureelec_log("ROM size: %d", buf[5] - 48);
    device_info->rom_size = 2 << (buf[5] - 49);

    if (buf[6] == '1') {
        device_info->has_light_sensor = 1;
    } else {
        device_info->has_light_sensor = 0;
    }

    if (buf[7] == '1' || buf[7] == '2') {
        device_info->has_thermal_sensor = 1;
    } else {
        device_info->has_thermal_sensor = 0;
    }

    libsureelec_log("Found %dx%d device with %dKbit ROM", device_info->width, device_info->height, device_info->rom_size);
    if (device_info->has_thermal_sensor) {
        libsureelec_log("with thermal sensor");
    }
    
    if (device_info->has_light_sensor) {
        libsureelec_log("with light sensor");
    }

    return 0;
}

LIBSUREELEC_EXPORT void libsureelec_toggle_display(libsureelec_ctx *ctx) {
    const char cmd[2] = { '\xFE', '\x64' };
    libsureelec_write(ctx, cmd, sizeof(cmd));
}

LIBSUREELEC_EXPORT void libsureelec_set_contrast(libsureelec_ctx *ctx, int contrast) {
    char cmd[3] = { '\xFE', '\x50', 0 };
    if (contrast > 255) {
        contrast = 255;
    } else if (contrast < 1) {
        contrast = 1;
    }

    cmd[2] = contrast;
    libsureelec_write(ctx, cmd, sizeof(cmd));
    ctx->contrast = contrast;
}

LIBSUREELEC_EXPORT void libsureelec_set_brightness(libsureelec_ctx *ctx, int brightness) {
    char cmd[3] = { '\xFE', '\x98', 0 };
    if (brightness > 255) {
        brightness = 255;
    } else if (brightness < 1) {
        brightness = 1;
    }

    cmd[2] = brightness;
    libsureelec_write(ctx, cmd, sizeof(cmd));
    ctx->brightness = brightness;
}

LIBSUREELEC_EXPORT int libsureelec_get_temperature(libsureelec_ctx *ctx) {
    const char cmd[2] = { '\xFE', '\x77' };
    char buf[5];
    char *p;
    int retval;

    if (ctx->device_info.has_thermal_sensor != 1) {
        return LIBSUREELEC_NO_TEMP_SENSOR;
    }

    memset(buf, ' ', sizeof(buf));
    libsureelec_write(ctx, cmd, sizeof(cmd));
    libsureelec_read(ctx, buf, 5);

    if (buf[0] == 'T') {
        return LIBSUREELEC_TEMP_OUT_OF_RANGE;
    }

    if (buf[4] == 'C') {
        libsureelec_log("Temperature is in degrees C.");
    } else {
        libsureelec_log("Temperature is in degrees F.");
    }

    buf[3] = '\0';
    errno = 0;
    retval = strtol(buf, &p, 10);
    if (errno != 0 || *p != 0 || p == (char *) buf) {
        libsureelec_log("Failed to convert temperature");
        return LIBSUREELEC_TEMP_OUT_OF_RANGE;
    }

    return(retval);
}

LIBSUREELEC_EXPORT int libsureelec_get_contrast(libsureelec_ctx *ctx) {
    const char cmd[2] = { '\xFE', '\x63' };
    char buf[5];
    char *p;
    int retval;

    memset(buf, ' ', sizeof(buf));
    libsureelec_write(ctx, cmd, sizeof(cmd));
    libsureelec_read(ctx, buf, 5);

    libsureelec_log("Contrast buffer: %s", buf);

    errno = 0;
    retval = strtol(buf + 2, &p, 10);
    if (errno != 0 || *p != 0 || p == (char *) buf) {
        libsureelec_log("Failed to convert contrast");
        return -1;
    }

    return(retval);
}

LIBSUREELEC_EXPORT int libsureelec_get_brightness(libsureelec_ctx *ctx) {
    const char cmd[2] = { '\xFE', '\x62' };
    char buf[7];
    char *p;
    int retval;

    memset(buf, ' ', sizeof(buf));
    libsureelec_write(ctx, cmd, sizeof(cmd));
    libsureelec_read(ctx, buf, 7);

    libsureelec_log("Brightness buffer: %s", buf);

    errno = 0;
    retval = strtol(buf + 4, &p, 10);
    if (errno != 0 || *p != 0 || p == (char *) buf) {
        libsureelec_log("Failed to convert brightness");
        return -1;
    }

    return(retval);
}

LIBSUREELEC_EXPORT void libsureelec_refresh(libsureelec_ctx *ctx) {
    int i;
    for (i = 1; i <= ctx->device_info.height; i++) {
        libsureelec_display_line(ctx, i, NULL);
    }
}

LIBSUREELEC_EXPORT void libsureelec_scroll_vert(libsureelec_ctx *ctx, int line, int direction, int distance, int wrap) {
    switch (direction) {
        case LIBSUREELEC_UP:
            if (distance > 4) {
                distance = 4;
            }
            char *src = &ctx->framebuffer[(distance) * ctx->device_info.width];
            memmove(ctx->framebuffer, src, (ctx->device_info.height - distance) * ctx->device_info.width);
            libsureelec_refresh(ctx);
            break;
        case LIBSUREELEC_DOWN:
            if (distance > 4) {
                distance = 4;
            }
            char *dest = &ctx->framebuffer[(distance) * ctx->device_info.width];
            memmove(dest, ctx->framebuffer, (ctx->device_info.height - distance) * ctx->device_info.width);
            libsureelec_refresh(ctx);
            break;
    }
}
