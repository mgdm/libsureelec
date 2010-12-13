#if defined(__GNUC__) && __GNUC__ >= 4
#   define LIBSUREELEC_EXPORT __attribute__ ((visibility("default")))
#else
#   define LIBSUREELEC_EXPORT
#endif

typedef struct _libsureelec_ctx {
    int fd; /* Serial port file handle */
} libsureelec_ctx;

LIBSUREELEC_EXPORT libsureelec_ctx *libsureelec_create(const char *device, int debug);
LIBSUREELEC_EXPORT libsureelec_ctx *libsureelec_destroy(libsureelec_ctx *ctx);
LIBSUREELEC_EXPORT libsureelec_clear_display(libsureelec_ctx *ctx);
LIBSUREELEC_EXPORT int libsureelec_write_line(libsureelec_ctx *ctx, const char *data, int line);
LIBSUREELEC_EXPORT char *libsureelec_get_screen_size(libsureelec_ctx *ctx);
