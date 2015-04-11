/*
libimago - a multi-format image file input/output library.
Copyright (C) 2010-2015 John Tsiombikas <nuclear@member.fsf.org>

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

/* -- Targa (tga) module -- */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "imago2.h"
#include "ftype_module.h"


#if  defined(__i386__) || defined(__ia64__) || defined(WIN32) || \
    (defined(__alpha__) || defined(__alpha)) || \
     defined(__arm__) || \
    (defined(__mips__) && defined(__MIPSEL__)) || \
     defined(__SYMBIAN32__) || \
     defined(__x86_64__) || \
     defined(__LITTLE_ENDIAN__)
/* little endian */
#define read_int16_le(f)	read_int16(f)
#else
/* big endian */
#define read_int16_le(f)	read_int16_inv(f)
#endif	/* endian check */


enum {
	IMG_NONE,
	IMG_CMAP,
	IMG_RGBA,
	IMG_BW,

	IMG_RLE_CMAP = 9,
	IMG_RLE_RGBA,
	IMG_RLE_BW
};

#define IS_RLE(x)	((x) >= IMG_RLE_CMAP)
#define IS_RGBA(x)	((x) == IMG_RGBA || (x) == IMG_RLE_RGBA)


struct tga_header {
	uint8_t idlen;			/* id field length */
	uint8_t cmap_type;		/* color map type (0:no color map, 1:color map present) */
	uint8_t img_type;		/* image type:
							 * 0: no image data
							 *	1: uncomp. color-mapped		 9: RLE color-mapped
							 *	2: uncomp. true color		10: RLE true color
							 *	3: uncomp. black/white		11: RLE black/white */
	uint16_t cmap_first;	/* color map first entry index */
	uint16_t cmap_len;		/* color map length */
	uint8_t cmap_entry_sz;	/* color map entry size */
	uint16_t img_x;			/* X-origin of the image */
	uint16_t img_y;			/* Y-origin of the image */
	uint16_t img_width;		/* image width */
	uint16_t img_height;	/* image height */
	uint8_t img_bpp;		/* bits per pixel */
	uint8_t img_desc;		/* descriptor:
							 * bits 0 - 3: alpha or overlay bits
							 * bits 5 & 4: origin (0 = bottom/left, 1 = top/right)
							 * bits 7 & 6: data interleaving */
};

struct tga_footer {
	uint32_t ext_off;		/* extension area offset */
	uint32_t devdir_off;	/* developer directory offset */
	char sig[18];				/* signature with . and \0 */
};


static int check(struct img_io *io);
static int read(struct img_pixmap *img, struct img_io *io);
static int write(struct img_pixmap *img, struct img_io *io);
static int read_pixel(struct img_io *io, int rdalpha, uint32_t *pix);
static int16_t read_int16(struct img_io *io);
static int16_t read_int16_inv(struct img_io *io);

int img_register_tga(void)
{
	static struct ftype_module mod = {".tga", check, read, write};
	return img_register_module(&mod);
}


static int check(struct img_io *io)
{
	struct tga_footer foot;
	int res = -1;
	long pos = io->seek(0, SEEK_CUR, io->uptr);
	io->seek(-18, SEEK_END, io->uptr);

	if(io->read(foot.sig, 17, io->uptr) < 17) {
		io->seek(pos, SEEK_SET, io->uptr);
		return -1;
	}

	if(memcmp(foot.sig, "TRUEVISION-XFILE.", 17) == 0) {
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

static int read(struct img_pixmap *img, struct img_io *io)
{
	struct tga_header hdr;
	unsigned long x, y;
	int i, c;
	uint32_t ppixel = 0;
	int rle_mode = 0, rle_pix_left = 0;
	int rdalpha;

	/* read header */
	hdr.idlen = iofgetc(io);
	hdr.cmap_type = iofgetc(io);
	hdr.img_type = iofgetc(io);
	hdr.cmap_first = read_int16_le(io);
	hdr.cmap_len = read_int16_le(io);
	hdr.cmap_entry_sz = iofgetc(io);
	hdr.img_x = read_int16_le(io);
	hdr.img_y = read_int16_le(io);
	hdr.img_width = read_int16_le(io);
	hdr.img_height = read_int16_le(io);
	hdr.img_bpp = iofgetc(io);
	if((c = iofgetc(io)) == -1) {
		return -1;
	}
	hdr.img_desc = c;

	if(!IS_RGBA(hdr.img_type)) {
		fprintf(stderr, "only true color tga images supported\n");
		return -1;
	}

	io->seek(hdr.idlen, SEEK_CUR, io);	/* skip the image ID */

	/* skip the color map if it exists */
	if(hdr.cmap_type == 1) {
		io->seek(hdr.cmap_len * hdr.cmap_entry_sz / 8, SEEK_CUR, io);
	}

	x = hdr.img_width;
	y = hdr.img_height;
	rdalpha = hdr.img_desc & 0xf;

	/* TODO make this IMG_FMT_RGB24 if there's no alpha channel */
	if(img_set_pixels(img, x, y, IMG_FMT_RGBA32, 0) == -1) {
		return -1;
	}

	for(i=0; i<y; i++) {
		uint32_t *ptr;
		int j;

		ptr = (uint32_t*)img->pixels + ((hdr.img_desc & 0x20) ? i : y - (i + 1)) * x;

		for(j=0; j<x; j++) {
			/* if the image is raw, then just read the next pixel */
			if(!IS_RLE(hdr.img_type)) {
				if(read_pixel(io, rdalpha, &ppixel) == -1) {
					return -1;
				}
			} else {
				/* otherwise, for RLE... */

				/* if we have pixels left in the packet ... */
				if(rle_pix_left) {
					/* if it's a raw packet, read the next pixel, otherwise keep the same */
					if(!rle_mode) {
						if(read_pixel(io, rdalpha, &ppixel) == -1) {
							return -1;
						}
					}
					--rle_pix_left;
				} else {
					/* read RLE packet header */
					unsigned char phdr = iofgetc(io);
					rle_mode = (phdr & 128);		/* last bit shows the mode for this packet (1: rle, 0: raw) */
					rle_pix_left = (phdr & ~128);	/* the rest gives the count of pixels minus one (we also read one here, so no +1) */
					/* and read the first pixel of the packet */
					if(read_pixel(io, rdalpha, &ppixel) == -1) {
						return -1;
					}
				}
			}

			*ptr++ = ppixel;
		}
	}

	return 0;
}

static int write(struct img_pixmap *img, struct img_io *io)
{
	return -1;	/* TODO */
}

#define PACK_COLOR32(r,g,b,a) \
	((((a) & 0xff) << 24) | \
	 (((r) & 0xff) << 0) | \
	 (((g) & 0xff) << 8) | \
	 (((b) & 0xff) << 16))

static int read_pixel(struct img_io *io, int rdalpha, uint32_t *pix)
{
	int r, g, b, a;
	b = iofgetc(io);
	g = iofgetc(io);
	r = iofgetc(io);
	a = rdalpha ? iofgetc(io) : 0xff;
	*pix = PACK_COLOR32(r, g, b, a);
	return a == -1 || r == -1 ? -1 : 0;
}

static int16_t read_int16(struct img_io *io)
{
	int16_t v;
	io->read(&v, 2, io);
	return v;
}

static int16_t read_int16_inv(struct img_io *io)
{
	int16_t v;
	io->read(&v, 2, io);
	return ((v >> 8) & 0xff) | (v << 8);
}
