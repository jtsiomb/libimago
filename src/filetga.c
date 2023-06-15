/*
libimago - a multi-format image file input/output library.
Copyright (C) 2010-2021 John Tsiombikas <nuclear@member.fsf.org>

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
#include "imago2.h"
#include "ftmodule.h"
#include "byteord.h"

#ifdef __GNUC__
#define PACKED	__attribute__((packed))
#endif


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

#if defined(__WATCOMC__) || defined(_MSC_VER)
#pragma pack(push, 1)
#endif

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
} PACKED;

struct tga_footer {
	uint32_t ext_off;		/* extension area offset */
	uint32_t devdir_off;	/* developer directory offset */
	char sig[18];				/* signature with . and \0 */
} PACKED;

#if defined(__WATCOMC__) || defined(_MSC_VER)
#pragma pack(pop)
#endif


static int check(struct img_io *io);
static int read_tga(struct img_pixmap *img, struct img_io *io);
static int write_tga(struct img_pixmap *img, struct img_io *io);
static int write_header(struct tga_header *hdr, struct img_io *io);
static int read_pixel(struct img_io *io, int fmt, unsigned char *pix);
static int fmt_to_tga_type(int fmt);

int img_register_tga(void)
{
	static struct ftype_module mod = {".tga:.targa", check, read_tga, write_tga};
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
	int c = 0;
	return io->read(&c, 1, io->uptr) < 1 ? -1 : c;
}

static int read_tga(struct img_pixmap *img, struct img_io *io)
{
	struct tga_header hdr;
	unsigned long x, y;
	int i, idx, c, r, g, b;
	int rle_mode = 0, rle_pix_left = 0;
	int pixel_bytes;
	int fmt;
	struct img_colormap cmap;

	/* read header */
	hdr.idlen = iofgetc(io);
	hdr.cmap_type = iofgetc(io);
	hdr.img_type = iofgetc(io);
	hdr.cmap_first = img_read_int16_le(io);
	hdr.cmap_len = img_read_int16_le(io);
	hdr.cmap_entry_sz = iofgetc(io);
	hdr.img_x = img_read_int16_le(io);
	hdr.img_y = img_read_int16_le(io);
	hdr.img_width = img_read_int16_le(io);
	hdr.img_height = img_read_int16_le(io);
	hdr.img_bpp = iofgetc(io);
	if((c = iofgetc(io)) == -1) {
		return -1;
	}
	hdr.img_desc = c;

	io->seek(hdr.idlen, SEEK_CUR, io->uptr);	/* skip the image ID */

	/* read the color map if it exists */
	if(hdr.cmap_type == 1) {
		cmap.ncolors = hdr.cmap_len;

		for(i=0; i<hdr.cmap_len; i++) {
			switch(hdr.cmap_entry_sz) {
			case 16:
				c = img_read_int16_le(io);
				r = (c & 0x7c00) >> 7;
				g = (c & 0x03e0) >> 2;
				b = (c & 0x001f) << 3;
				break;

			case 24:
				b = iofgetc(io);
				g = iofgetc(io);
				r = iofgetc(io);
				break;

			case 32:
				b = iofgetc(io);
				g = iofgetc(io);
				r = iofgetc(io);
				iofgetc(io);	/* ignore attribute byte */
			}

			idx = i + hdr.cmap_first;
			if(idx < 256) {
				cmap.color[idx].r = r;
				cmap.color[idx].g = g;
				cmap.color[idx].b = b;
				if(cmap.ncolors <= idx) cmap.ncolors = idx + 1;
			}
		}
	}

	x = hdr.img_width;
	y = hdr.img_height;

	if(hdr.img_type == IMG_CMAP || hdr.img_type == IMG_RLE_CMAP) {
		if(hdr.img_bpp != 8) {
			fprintf(stderr, "read_tga: indexed images with more than 8bpp not supported\n");
			return -1;
		}
		pixel_bytes = 1;
		fmt = IMG_FMT_IDX8;
	} else {
		int alpha = hdr.img_desc & 0xf;
		pixel_bytes = alpha ? 4 : 3;
		fmt = alpha ? IMG_FMT_RGBA32 : IMG_FMT_RGB24;
	}

	if(img_set_pixels(img, x, y, fmt, 0) == -1) {
		return -1;
	}

	for(i=0; i<y; i++) {
		unsigned char *ptr;
		int j, k;

		ptr = (unsigned char*)img->pixels + ((hdr.img_desc & 0x20) ? i : y - (i + 1)) * x * pixel_bytes;

		for(j=0; j<x; j++) {
			/* if the image is raw, then just read the next pixel */
			if(!IS_RLE(hdr.img_type)) {
				if(read_pixel(io, fmt, ptr) == -1) {
					return -1;
				}
			} else {
				/* otherwise, for RLE... */

				/* if we have pixels left in the packet ... */
				if(rle_pix_left) {
					/* if it's a raw packet, read the next pixel, otherwise keep the same */
					if(!rle_mode) {
						if(read_pixel(io, fmt, ptr) == -1) {
							return -1;
						}
					} else {
						for(k=0; k<pixel_bytes; k++) {
							ptr[k] = ptr[k - pixel_bytes];
						}
					}
					--rle_pix_left;
				} else {
					/* read RLE packet header */
					unsigned char phdr = iofgetc(io);
					rle_mode = (phdr & 128);		/* last bit shows the mode for this packet (1: rle, 0: raw) */
					rle_pix_left = (phdr & ~128);	/* the rest gives the count of pixels minus one (we also read one here, so no +1) */
					/* and read the first pixel of the packet */
					if(read_pixel(io, fmt, ptr) == -1) {
						return -1;
					}
				}
			}

			ptr += pixel_bytes;
		}
	}

	if(fmt == IMG_FMT_IDX8) {
		struct img_colormap *dest = img_colormap(img);
		*dest = cmap;
	}

	return 0;
}

/* TODO: implement RLE compression */
static int write_tga(struct img_pixmap *img, struct img_io *io)
{
	int i, j, res = -1;
	struct tga_header hdr = {0};
	struct tga_footer foot = {0};
	struct img_pixmap tmpimg;
	size_t sz;
	struct img_colormap tmpcmap, *cmap = 0;
	unsigned char *pixptr, *scanline;

	img_init(&tmpimg);

	/* if the input image is floating-point, we need to convert it to integer */
	if(img_is_float(img)) {
		if(img_copy(&tmpimg, img) == -1) {
			goto end;
		}
		if(img_to_integer(&tmpimg) == -1) {
			goto end;
		}
		img = &tmpimg;

	} else if(img->fmt == IMG_FMT_RGB565) {
		/* if it's 565 just convert it to RGB24 first */
		if(img_copy(&tmpimg, img) == -1) {
			goto end;
		}
		if(img_convert(&tmpimg, IMG_FMT_RGB24) == -1) {
			goto end;
		}
		img = &tmpimg;
	}

	hdr.img_type = fmt_to_tga_type(img->fmt);
	hdr.img_width = img->width;
	hdr.img_height = img->height;
	hdr.img_bpp = img->pixelsz * 8;
	hdr.img_desc = 0x20;	/* origin: top-left */
	if(img_has_alpha(img)) {
		hdr.img_desc |= 8;
	}

	if(img->fmt == IMG_FMT_IDX8) {
		cmap = img_colormap(img);
		hdr.cmap_len = cmap->ncolors;
		hdr.cmap_entry_sz = 24;
		hdr.cmap_type = 1;
	}

	if(write_header(&hdr, io) == -1) {
		goto end;
	}

	if(cmap) {
		for(i=0; i<cmap->ncolors; i++) {
			tmpcmap.color[i].r = cmap->color[i].b;
			tmpcmap.color[i].g = cmap->color[i].g;
			tmpcmap.color[i].b = cmap->color[i].r;
		}
		sz = cmap->ncolors * 3;
		if(io->write(tmpcmap.color, sz, io->uptr) < sz) {
			goto end;
		}
	}

	if(img->fmt == IMG_FMT_GREY8 || img->fmt == IMG_FMT_IDX8) {
		sz = img->height * img->width * img->pixelsz;
		if(io->write(img->pixels, sz, io->uptr) < sz) {
			goto end;
		}
	} else {
		sz = img->width * img->pixelsz;
		if(!(scanline = malloc(sz))) {
			goto end;
		}

		pixptr = img->pixels;
		for(i=0; i<img->height; i++) {
			unsigned char *dest = scanline;
			for(j=0; j<img->width; j++) {
				dest[0] = pixptr[2];
				dest[1] = pixptr[1];
				dest[2] = pixptr[0];
				if(img->fmt == IMG_FMT_RGBA32) {
					dest[3] = pixptr[3];
				}
				pixptr += img->pixelsz;
				dest += img->pixelsz;
			}
			if(io->write(scanline, sz, io->uptr) < sz) {
				free(scanline);
				goto end;
			}
		}

		free(scanline);
	}

	strcpy(foot.sig, "TRUEVISION-XFILE.");
	io->write(&foot, sizeof foot, io->uptr);

	res = 0;
end:
	img_destroy(&tmpimg);
	return res;
}

static int write_header(struct tga_header *hdr, struct img_io *io)
{
#ifdef IMAGO_BIG_ENDIAN
	hdr->cmap_first = img_bswap_int16(hdr->cmap_first);
	hdr->cmap_len = img_bswap_int16(hdr->cmap_len);
	hdr->img_x = img_bswap_int16(hdr->img_x);
	hdr->img_y = img_bswap_int16(hdr->img_y);
	hdr->img_width = img_bswap_int16(hdr->img_width);
	hdr->img_height = img_bswap_int16(hdr->img_height);
#endif

	if(io->write(hdr, sizeof *hdr, io->uptr) < sizeof *hdr) {
		return -1;
	}
	return 0;
}

static int read_pixel(struct img_io *io, int fmt, unsigned char *pix)
{
	int r, g, b, a;

	if(fmt == IMG_FMT_IDX8) {
		if((b = iofgetc(io)) == -1) {
			return -1;
		}
		*pix = b;
		return 0;
	}

	if((b = iofgetc(io)) == -1 || (g = iofgetc(io)) == -1 || (r = iofgetc(io)) == -1) {
		return -1;
	}

	pix[0] = r;
	pix[1] = g;
	pix[2] = b;

	if(fmt == IMG_FMT_RGBA32) {
		if((a = iofgetc(io)) == -1) {
			return -1;
		}
		pix[3] = a;
	}
	return 0;
}

static int fmt_to_tga_type(int fmt)
{
	switch(fmt) {
	case IMG_FMT_GREY8:
		return IMG_BW;
	case IMG_FMT_IDX8:
		return IMG_CMAP;
	case IMG_FMT_RGB24:
	case IMG_FMT_RGBA32:
	case IMG_FMT_RGB565:
		return IMG_RGBA;
	default:
		break;
	}
	return -1;
}
