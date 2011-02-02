#if defined(__GNUC__) && __GNUC__ >= 4
#   define LIBSUREELEC_EXPORT __attribute__ ((visibility("default")))
#else
#   define LIBSUREELEC_EXPORT
#endif

#ifdef __APPLE__
#include <string.h>
/* Mac OS X don't have strndup even if _GNU_SOURCE is defined */
char *strndup (const char *s, size_t n)
{
	size_t len = strlen (s);
	char *ret;

	if (len <= n)
		return strdup (s);

	ret = malloc(n + 1);
	strncpy(ret, s, n);
	ret[n] = '\0';
	return ret;
}
#endif

typedef struct _libsureelec_device_info {
    long width;
    long height;
    long rom_size;
    long has_rx8025;
    long has_light_sensor;
    long has_thermal_sensor;
} libsureelec_device_info;

typedef struct _libsureelec_ctx {
	int fd; /* Serial port file handle */
    libsureelec_device_info device_info;
	unsigned char *framebuffer;
    int framebuffer_size;
	int display_state;
	int contrast;
	int brightness;
} libsureelec_ctx;

#define LIBSUREELEC_TEMP_OUT_OF_RANGE -999
#define LIBSUREELEC_NO_TEMP_SENSOR -998

LIBSUREELEC_EXPORT libsureelec_ctx* libsureelec_create(const char *device, int debug);
LIBSUREELEC_EXPORT libsureelec_ctx *libsureelec_destroy(libsureelec_ctx *ctx);
LIBSUREELEC_EXPORT libsureelec_clear_display(libsureelec_ctx *ctx);
LIBSUREELEC_EXPORT int libsureelec_write_line(libsureelec_ctx *ctx, const char *data, int line);
LIBSUREELEC_EXPORT int libsureelec_get_device_info(libsureelec_ctx *ctx, libsureelec_device_info *device_info);
LIBSUREELEC_EXPORT void libsureelec_toggle_display(libsureelec_ctx *ctx);
LIBSUREELEC_EXPORT void libsureelec_set_contrast(libsureelec_ctx *ctx, int contrast);
LIBSUREELEC_EXPORT long libsureelec_get_temperature(libsureelec_ctx *ctx);
LIBSUREELEC_EXPORT long libsureelec_get_contrast(libsureelec_ctx *ctx);
LIBSUREELEC_EXPORT long libsureelec_get_brightness(libsureelec_ctx *ctx);

