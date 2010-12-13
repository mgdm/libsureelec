/* libsureelec.c
 * Driver for SureElec LCD modules, ripped from lcdproc
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

#include "libsureelec.h"

static unsigned char cmd_prefix[3] = {'\xFE', '\x56', 0};
static unsigned char init_seq[7] = {'\x54', '\x58', '\x4B', '\x52', '\x44', '\x41', '\x60'};
//static unsigned char init_seq[5] = {'\376','S','u','r','e'};

static int debug_mode = 1;

static void libsureelec_log(const char *format, ...) {
    if (debug_mode) {
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

        printf("fd: %d", ctx->fd);
        retval = select(ctx->fd + 1, &rfds, NULL, NULL, &tv);
        if (retval) {
            int read_result = read(ctx->fd + 1, ((char *)buf) + read_count, count - read_count);
            if (read_result < 0) {
                return -1;
            } else {
                read_count += read_result;
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

static int libsureelec_write_char(libsureelec_ctx *ctx, const unsigned char seq, int count) {
    int written = 0;

    printf("%x\n", seq);
    written = write(ctx->fd, &seq, 1);
    if (written == -1) {
        libsureelec_log("Cannot write to port: %s", strerror(errno));
        return -1;
    }

    return 1;
}

LIBSUREELEC_EXPORT libsureelec_ctx* libsureelec_create(const char *device, int debug) {
    libsureelec_ctx *ctx = (libsureelec_ctx *) calloc(1, sizeof(libsureelec_ctx));
    ctx->fd = -1;
    struct termios port_config;
	unsigned char cmd[3] = {'\xFE', '\x56', 0};
    int i;

    ctx->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
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
	port_config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	port_config.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | CSTOPB);
	port_config.c_cflag |= CS8 | CREAD | CLOCAL;
	port_config.c_cc[VMIN] = 1;
	port_config.c_cc[VTIME] = 10;

    if(cfsetispeed(&port_config, B9600) < 0 || cfsetospeed(&port_config, B9600) < 0) {
        libsureelec_log("Failed to set port speed");
        return NULL;
    }

    if(tcsetattr(ctx->fd, TCSANOW, &port_config) < 0) {
        libsureelec_log("Failed to apply port configuration to device %s", device);
        return NULL;
    }
    
	for (i = 0; i < sizeof(init_seq); i++) {
		if (libsureelec_write_char(ctx, (const unsigned char) '\xFE', 1) == -1
		    || libsureelec_write_char(ctx, init_seq[i], 1) == -1) {
            libsureelec_log("Cannot write init sequence to device %s on FD %d", device, ctx->fd);
			return NULL;
        }
	}

    if (libsureelec_write(ctx, init_seq, sizeof(init_seq)) == -1) {
        libsureelec_log("Failed to initialize device %s", device);
        return NULL;
    }

    return ctx;
}

LIBSUREELEC_EXPORT libsureelec_ctx *libsureelec_destroy(libsureelec_ctx *ctx) {
    if (ctx->fd) {
        close(ctx->fd);
    }

    free(ctx);
}

LIBSUREELEC_EXPORT libsureelec_clear_display(libsureelec_ctx *ctx) {
    
}

LIBSUREELEC_EXPORT int libsureelec_write_line(libsureelec_ctx *ctx, const char *data, int line) {

    int data_size;
	unsigned char cmd[4] = {'\xFE', '\x47', '\x01', 0};

    if (line < 1 || line > 4) {
        return -1;
    }

    data_size = strlen(data);
    if (data_size > 20) {
        data_size = 20;
    }
    libsureelec_log("Writing %d characters to line %d: %s", data_size, line, data);
   
    cmd[3] = line; 
    libsureelec_write(ctx, cmd, sizeof(cmd));
    libsureelec_write(ctx, data, data_size);
}

LIBSUREELEC_EXPORT char * libsureelec_get_screen_size(libsureelec_ctx *ctx) {
    unsigned char cmd[2] = {'\xFE', '\x76'};
    char *buf = strdup("                  ");

    libsureelec_write(ctx, cmd, sizeof(cmd));
    libsureelec_read(ctx, &buf, sizeof(buf));
    return(buf);
}
