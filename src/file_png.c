/* PNG module */

#include <png.h>
#include "imago2.h"
#include "ftype_module.h"

static int check_file(struct img_io *io);
static int read_file(struct img_pixmap *img, struct img_io *io);
static int write_file(struct img_pixmap *img, struct img_io *io);

int img_register_png(void)
{
	static struct ftype_module mod = {".png", check_file, read_file, write_file};
	return img_register_module(&mod);
}

static int check_file(struct img_io *io)
{
	unsigned char sig[8];
	int res;
	long pos = io->seek(0, SEEK_CUR, io->uptr);

	if(io->read(sig, 8, io->uptr) < 8) {
		io->seek(pos, SEEK_SET, io->uptr);
		return -1;
	}

	res = png_sig_cmp(sig, 0, 8) == 0 ? 0 : -1;
	io->seek(pos, SEEK_SET, io->uptr);
	return res;
}

static int read_file(struct img_pixmap *img, struct img_io *io)
{
	png_struct *png;
	png_info *info;

	if(!(png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0))) {
		return -1;
	}

	if(!(info = png_create_info_struct(png))) {
		png_destroy_read_struct(&png, 0, 0);
		return -1;
	}

	if(setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, 0);
		return -1;
	}

	/* ... TODO ... */

	png_destroy_read_struct(&png, &info, 0);
	return -1;
}

static int write_file(struct img_pixmap *img, struct img_io *io)
{
	return -1;
}
