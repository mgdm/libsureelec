/* libsureelec.c
 * Driver for SureElec LCD modules
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
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

static libsureelec_read(libsureelec_ctx *ctx, void *buf, size_t count) {
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

static int libsureelec_write(libsureelec_ctx *ctx, const unsigned char *seq, int count) {
    int written, written_count = 0;
    while (written_count < count) {
        written = write(ctx->fd, seq, count - written_count);
        if (written == -1) {
            libsureelec_log("Cannot write to port: %s", strerror(errno));
            return -1;
        }
        written_count += written;
    }

    return written_count;
}

static int libsureelec_write_char(libsureelec_ctx *ctx, const unsigned char seq) {
    int written = 0;

    written = write(ctx->fd, &seq, 1);
    if (written == -1) {
        libsureelec_log("Cannot write to port: %s", strerror(errno));
        return -1;
    }

    return 1;
}

LIBSUREELEC_EXPORT libsureelec_ctx* libsureelec_create(const char *device, int debug) {
    unsigned char init_seq[5] = {'\xFE', 'S', 'u', 'r', 'e'};
    libsureelec_ctx *ctx = (libsureelec_ctx *) calloc(1, sizeof(libsureelec_ctx));
    ctx->fd = -1;
    struct termios port_config;
    int i;

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
    usleep(10000);

    libsureelec_get_device_info(ctx, &ctx->device_info);
    /* Set up framebuffer */
    ctx->framebuffer_size = ctx->device_info.width * ctx->device_info.height;
    ctx->framebuffer = malloc(ctx->framebuffer_size * sizeof(unsigned char));
    memset(ctx->framebuffer, ' ', ctx->framebuffer_size);
    return ctx;
}

LIBSUREELEC_EXPORT libsureelec_ctx *libsureelec_destroy(libsureelec_ctx *ctx) {
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
        libsureelec_write_line(ctx, ctx->framebuffer + (ctx->device_info.width * line), line + 1);
    }
}

LIBSUREELEC_EXPORT int libsureelec_write_line(libsureelec_ctx *ctx, const char *data, int line) {

    int data_size;
	unsigned char cmd[4] = {'\xFE', '\x47', '\x01', 0};
    unsigned char *dest;

    if (line < 1 || line > ctx->device_info.height) {
        return -1;
    }

    data_size = strlen(data);
    if (data_size > ctx->device_info.width) {
        data_size = ctx->device_info.width;
    }

    dest = ctx->framebuffer + (ctx->device_info.width * (line - 1));
    dest = memcpy(dest, data, data_size);

    cmd[3] = line; 
    libsureelec_write(ctx, cmd, sizeof(cmd));
    libsureelec_write(ctx, dest, ctx->device_info.width);
    usleep(25000);
}

LIBSUREELEC_EXPORT int libsureelec_get_device_info(libsureelec_ctx *ctx, libsureelec_device_info *device_info) {
    const unsigned char cmd[2] = { '\xFE', '\x76' };
    unsigned char buf[11], temp[4];
    char *end_ptr;

    libsureelec_write(ctx, cmd, sizeof(cmd));
    usleep(10000);
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

    libsureelec_log("Found %d*%d device with %dKbit ROM", device_info->height, device_info->width, device_info->rom_size);
    if (device_info->has_thermal_sensor) {
        libsureelec_log("with thermal sensor");
    }
    
    if (device_info->has_light_sensor) {
        libsureelec_log("with light sensor");
    }

    return 0;
}

LIBSUREELEC_EXPORT void libsureelec_toggle_display(libsureelec_ctx *ctx) {
    const unsigned char cmd[2] = { '\xFE', '\x64' };
    libsureelec_write(ctx, cmd, sizeof(cmd));
    usleep(10000);
}

LIBSUREELEC_EXPORT void libsureelec_set_contrast(libsureelec_ctx *ctx, int contrast) {
    unsigned char cmd[3] = { '\xFE', '\x50', 0 };
    if (contrast > 255) {
        contrast = 255;
    } else if (contrast < 1) {
        contrast = 1;
    }

    cmd[2] = contrast;
    libsureelec_write(ctx, cmd, sizeof(cmd));
    ctx->contrast = contrast;
    usleep(10000);
}

LIBSUREELEC_EXPORT void libsureelec_set_brightness(libsureelec_ctx *ctx, int brightness) {
    unsigned char cmd[3] = { '\xFE', '\x98', 0 };
    if (brightness > 255) {
        brightness = 255;
    } else if (brightness < 1) {
        brightness = 1;
    }

    cmd[2] = brightness;
    libsureelec_write(ctx, cmd, sizeof(cmd));
    ctx->brightness = brightness;
    usleep(10000);
}

LIBSUREELEC_EXPORT long libsureelec_get_temperature(libsureelec_ctx *ctx) {
    const unsigned char cmd[2] = { '\xFE', '\x77' };
    unsigned char buf[5];
    char *p;
    long retval;

    if (ctx->device_info.has_thermal_sensor != 1) {
        return LIBSUREELEC_NO_TEMP_SENSOR;
    }

    memset(buf, ' ', sizeof(buf));
    libsureelec_write(ctx, cmd, sizeof(cmd));
    usleep(10000);
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

LIBSUREELEC_EXPORT long libsureelec_get_contrast(libsureelec_ctx *ctx) {
    const unsigned char cmd[2] = { '\xFE', '\x63' };
    unsigned char buf[5];
    char *p;
    long retval;

    memset(buf, ' ', sizeof(buf));
    libsureelec_write(ctx, cmd, sizeof(cmd));
    usleep(10000);
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

LIBSUREELEC_EXPORT long libsureelec_get_brightness(libsureelec_ctx *ctx) {
    const unsigned char cmd[2] = { '\xFE', '\x62' };
    unsigned char buf[7];
    char *p;
    long retval;

    memset(buf, ' ', sizeof(buf));
    libsureelec_write(ctx, cmd, sizeof(cmd));
    usleep(10000);
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
