/*
libimago - a multi-format image file input/output library.
Copyright (C) 2010 John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* -- Portable Pixmap (PPM) module -- */

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
		io->seek(pos, SEEK_SET, io->uptr);
		return -1;
	}

	if(id[0] == 'P' && (id[1] == '6' || id[1] == '3')) {
		res = 0;
	}
	io->seek(pos, SEEK_SET, io->uptr);
	return res;
}

static int iofgetc(struct img_io *io)
{
	char c;
	return io->read(&c, 1, io->uptr) < 1 ? -1 : c;
}

static char *iofgets(char *buf, int size, struct img_io *io)
{
	int c;
	char *ptr = buf;

	while(--size > 0 && (c = iofgetc(io)) != -1) {
		*ptr++ = c;
		if(c == '\n') break;
	}
	*ptr = 0;

	return ptr == buf ? 0 : buf;
}

/* TODO: implement P3 reading */
static int read(struct img_pixmap *img, struct img_io *io)
{
	char buf[256];
	int xsz, ysz, maxval, got_hdrlines = 1;

	if(!iofgets(buf, sizeof buf, io)) {
		return -1;
	}
	if(!(buf[0] == 'P' && (buf[1] == '6' || buf[1] == '3'))) {
		return -1;
	}

	while(got_hdrlines < 3 && iofgets(buf, sizeof buf, io)) {
		if(buf[0] == '#') continue;

		switch(got_hdrlines) {
		case 1:
			if(sscanf(buf, "%d %d\n", &xsz, &ysz) < 2) {
				return -1;
			}
			break;

		case 2:
			if(sscanf(buf, "%d\n", &maxval) < 1) {
				return -1;
			}
		default:
			break;
		}
		got_hdrlines++;
	}

	if(xsz < 1 || ysz < 1 || maxval != 255) {
		return -1;
	}

	if(img_set_pixels(img, xsz, ysz, IMG_FMT_RGB24, 0) == -1) {
		return -1;
	}

	if(io->read(img->pixels, xsz * ysz * 3, io->uptr) < xsz * ysz * 3) {
		return -1;
	}
	return 0;
}

static int write(struct img_pixmap *img, struct img_io *io)
{
	int sz;
	char buf[256];
	struct img_pixmap tmpimg;

	img_init(&tmpimg);

	if(img->fmt != IMG_FMT_RGB24) {
		if(img_copy(&tmpimg, img) == -1) {
			return -1;
		}
		if(img_convert(&tmpimg, IMG_FMT_RGB24) == -1) {
			return -1;
		}
		img = &tmpimg;
	}

	sprintf(buf, "P6\n#written by libimago2\n%d %d\n255\n", img->width, img->height);
	if(io->write(buf, strlen(buf), io->uptr) < strlen(buf)) {
		img_destroy(&tmpimg);
		return -1;
	}

	sz = img->width * img->height * 3;
	if(io->write(img->pixels, sz, io->uptr) < sz) {
		img_destroy(&tmpimg);
		return -1;
	}

	img_destroy(&tmpimg);
	return 0;
}
