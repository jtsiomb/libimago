#ifndef IMAGO2_H_
#define IMAGO2_H_

#include <stdio.h>

#ifdef __cplusplus
#define IMG_OPTARG(arg, val)	arg = val
#else
#define IMG_OPTARG(arg, val)	arg
#endif

/* XXX if you change this make sure to also change pack/unpack arrays in conv.c */
enum img_fmt {
	IMG_FMT_GREY8,
	IMG_FMT_RGB24,
	IMG_FMT_RGBA32,
	IMG_FMT_GREYF,
	IMG_FMT_RGBF,
	IMG_FMT_RGBAF,

	NUM_IMG_FMT
};

struct img_pixmap {
	void *pixels;
	int width, height;
	enum img_fmt fmt;
	int pixelsz;
	char *name;
};

struct img_io {
	void *uptr;	/* user-data */

	size_t (*read)(void *buf, size_t bytes, void *uptr);
	size_t (*write)(void *buf, size_t bytes, void *uptr);
	long (*seek)(long offs, int whence, void *uptr);
};

#ifdef __cplusplus
extern "C" {
#endif

int img_init(struct img_pixmap *img, IMG_OPTARG(enum img_fmt fmt, IMG_FMT_RGBA32));
int img_destroy(struct img_pixmap *img);

struct img_pixmap *img_create(IMG_OPTARG(enum img_fmt fmt, IMG_FMT_RGBA32));
void img_free(struct img_pixmap *img);

int img_set_name(struct img_pixmap *img, const char *name);

int img_copy(struct img_pixmap *dest, struct img_pixmap *src);

/* pix can be null, in which case the pixelbuffer is allocated but not filled with data */
int img_set_pixels(struct img_pixmap *img, int w, int h, enum img_fmt fmt, void *pix);

/* convenience functions to easily save/load an image as an array of pixels,
 * without having to deal with our image structure.
 */
void *img_load_pixels(const char *fname, int *xsz, int *ysz, IMG_OPTARG(enum img_fmt fmt, IMG_RGBA32));
int img_save_pixels(const char *fname, void *pix, int xsz, int ysz, IMG_OPTARG(enum img_fmt fmt, IMG_RGBA32));
void img_free_pixels(void *pix);

int img_load(struct img_pixmap *img, const char *fname);
int img_save(struct img_pixmap *img, const char *fname);

int img_read_file(struct img_pixmap *img, FILE *fp);
int img_write_file(struct img_pixmap *img, FILE *fp);

int img_read(struct img_pixmap *img, struct img_io *io);
int img_write(struct img_pixmap *img, struct img_io *io);

/* image processing functions */
/* TODO */

int img_convert(struct img_pixmap *img, enum img_fmt tofmt);
int img_to_float(struct img_pixmap *img);
int img_to_integer(struct img_pixmap *img);

int img_is_float(struct img_pixmap *img);
int img_has_alpha(struct img_pixmap *img);

/* io-struct functions */
void img_io_set_user_data(struct img_io *io, void *uptr);
void img_io_set_read_func(struct img_io *io, size_t (*read)(void*, size_t, void*));
void img_io_set_write_func(struct img_io *io, size_t (*write)(void*, size_t, void*));
void img_io_set_seek_func(struct img_io *io, long (*seek)(long, int, void*));


#ifdef __cplusplus
}
#endif


#endif	/* IMAGO_H_ */
