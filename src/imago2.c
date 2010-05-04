#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imago2.h"
#include "ftype_module.h"

static int pixel_size(enum img_fmt fmt);
static size_t def_read(void *buf, size_t bytes, void *uptr);
static size_t def_write(void *buf, size_t bytes, void *uptr);
static long def_seek(long offset, int whence, void *uptr);


int img_init(struct img_pixmap *img, enum img_fmt fmt)
{
	img->pixels = 0;
	img->width = img->height = 0;
	img->fmt = fmt;
	img->pixelsz = pixel_size(fmt);
	img->name = 0;
	return 0;
}


int img_destroy(struct img_pixmap *img)
{
	free(img->pixels);
	img->pixels = 0;	/* just in case... */
	img->width = img->height = 0xbadbeef;
	free(img->name);
	return 0;
}

struct img_pixmap *img_create(enum img_fmt fmt)
{
	struct img_pixmap *p;

	if(!(p = malloc(sizeof *p))) {
		return 0;
	}
	if(img_init(p, fmt) == -1) {
		free(p);
		return 0;
	}
	return p;
}

void img_free(struct img_pixmap *img)
{
	img_destroy(img);
	free(img);
}

int img_set_name(struct img_pixmap *img, const char *name)
{
	char *tmp;

	if(!(tmp = malloc(strlen(name) + 1))) {
		return -1;
	}
	strcpy(tmp, name);
	img->name = tmp;
	return 0;
}

int img_copy(struct img_pixmap *dest, struct img_pixmap *src)
{
	return img_set_pixels(dest, src->width, src->height, src->fmt, src->pixels);
}

int img_set_pixels(struct img_pixmap *img, int w, int h, enum img_fmt fmt, void *pix)
{
	void *newpix;
	int pixsz = pixel_size(fmt);

	if(!(newpix = malloc(w * h * pixsz))) {
		return -1;
	}

	if(pix) {
		memcpy(newpix, pix, w * h * pixsz);
	} else {
		memset(newpix, 0, w * h * pixsz);
	}

	free(img->pixels);
	img->pixels = newpix;
	img->width = w;
	img->height = h;
	img->pixelsz = pixsz;
	img->fmt = fmt;
	return 0;
}

void *img_load_pixels(const char *fname, int *xsz, int *ysz, enum img_fmt fmt)
{
	struct img_pixmap img;

	if(img_init(&img, fmt) == -1) {
		return 0;
	}
	if(img_load(&img, fname) == -1) {
		return 0;
	}

	*xsz = img.width;
	*ysz = img.height;
	return img.pixels;
}

int img_save_pixels(const char *fname, void *pix, int xsz, int ysz, enum img_fmt fmt)
{
	struct img_pixmap img;

	if(img_init(&img, fmt) == -1) {
		return 0;
	}
	img.name = (char*)fname;
	img.width = xsz;
	img.height = ysz;
	img.pixels = pix;

	return img_save(&img, fname);
}

void img_free_pixels(void *pix)
{
	free(pix);
}

int img_load(struct img_pixmap *img, const char *fname)
{
	int res;
	FILE *fp;

	if(!(fp = fopen(fname, "rb"))) {
		return -1;
	}
	res = img_read_file(img, fp);
	fclose(fp);
	return res;
}

/* TODO implement filetype selection */
int img_save(struct img_pixmap *img, const char *fname)
{
	int res;
	FILE *fp;

	img_set_name(img, fname);

	if(!(fp = fopen(fname, "wb"))) {
		return -1;
	}
	res = img_write_file(img, fp);
	fclose(fp);
	return res;
}

int img_read_file(struct img_pixmap *img, FILE *fp)
{
	struct img_io io = {0, def_read, def_write, def_seek};

	io.uptr = fp;
	return img_read(img, &io);
}

int img_write_file(struct img_pixmap *img, FILE *fp)
{
	struct img_io io = {0, def_read, def_write, def_seek};

	io.uptr = fp;
	return img_write(img, &io);
}

int img_read(struct img_pixmap *img, struct img_io *io)
{
	struct ftype_module *mod;

	if((mod = img_find_format_module(io))) {
		return mod->read(img, io);
	}
	return -1;
}

int img_write(struct img_pixmap *img, struct img_io *io)
{
	struct ftype_module *mod;

	if(!img->name || !(mod = img_guess_format(img->name))) {
		/* TODO throw some sort of warning? */
		if(!(mod = img_get_module(0))) {
			return -1;
		}
	}

	return mod->write(img, io);
}

static int pixel_size(enum img_fmt fmt)
{
	switch(fmt) {
	case IMG_FMT_GREY8:
		return 1;
	case IMG_FMT_RGB24:
		return 3;
	case IMG_FMT_RGBA32:
		return 4;
	case IMG_FMT_GREYF:
		return sizeof(float);
	case IMG_FMT_RGBF:
		return 3 * sizeof(float);
	case IMG_FMT_RGBAF:
		return 4 * sizeof(float);
	default:
		break;
	}
	return 0;
}

static size_t def_read(void *buf, size_t bytes, void *uptr)
{
	return uptr ? fread(buf, 1, bytes, uptr) : 0;
}

static size_t def_write(void *buf, size_t bytes, void *uptr)
{
	return uptr ? fwrite(buf, 1, bytes, uptr) : 0;
}

static long def_seek(long offset, int whence, void *uptr)
{
	if(!uptr || fseek(uptr, offset, whence) == -1) {
		return -1;
	}
	return ftell(uptr);
}

