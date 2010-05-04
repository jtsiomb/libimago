#include <string.h>
#include "imago2.h"
#include "ftype_module.h"

static int check(struct img_io *io);
static int read(struct img_pixmap *img, struct img_io *io);
static int write(struct img_pixmap *img, struct img_io *io);

int img_register_ppm(void)
{
	static struct ftype_module mod = {".ppm", check, read, write};
	return img_register_module(&mod);
}


static int check(struct img_io *io)
{
	char id[2];
	int res = -1;
	long pos = io->seek(0, SEEK_CUR, io->uptr);

	if(io->read(id, 2, io->uptr) < 2) {
		return -1;
	}

	if(id[0] == 'P' && (id[1] == '6' || id[1] == '3')) {
		res = 0;
	}
	io->seek(pos, SEEK_SET, io->uptr);
	return res;
}

static int read(struct img_pixmap *img, struct img_io *io)
{
	return -1;
}

static int write(struct img_pixmap *img, struct img_io *io)
{
	int i, bleft, res = -1;
	char buf[256], *bufptr;
	struct img_pixmap tmpimg;
	char *pixptr;

	if(img_init(&tmpimg, img->fmt) == -1) {
		return -1;
	}
	img_copy(&tmpimg, img);
	img = &tmpimg;

	img_convert(img, IMG_FMT_RGB24);
	pixptr = img->pixels;

	sprintf(buf, "P6\n#written by libimago2\n%d %d\n255\n", img->width, img->height);
	if(io->write(buf, strlen(buf), io->uptr) < strlen(buf)) {
		goto out;
	}

	bleft = sizeof buf;
	bufptr = buf;

	for(i=0; i<img->width * img->height * 3; i++) {
		if(bleft--) {
			*bufptr++ = *pixptr++;
		} else {
			if(io->write(buf, sizeof buf, io->uptr) < sizeof buf) {
				goto out;
			}
			bufptr = buf;
			bleft = sizeof buf;
		}
	}
	if(bufptr != buf) {
		size_t sz = bufptr - buf;
		if(io->write(buf, sz, io->uptr) < sz) {
			goto out;
		}
	}
	res = 0;

out:
	img_destroy(&tmpimg);
	return res;
}
