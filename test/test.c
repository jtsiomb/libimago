#include <stdio.h>
#include <imago2.h>

int main(void)
{
	int i, j, xsz = 512, ysz = 512;
	struct img_pixmap img;
	unsigned char *pix;

	img_init(&img, IMG_FMT_RGB24);
	img_set_pixels(&img, xsz, ysz, IMG_FMT_RGB24, 0);

	pix = img.pixels;
	for(i=0; i<ysz; i++) {
		for(j=0; j<xsz; j++) {
			int bw = ((i >> 3) & 1) == ((j >> 3) & 1);

			*pix++ = bw ? 255 : 0;
			*pix++ = 127;
			*pix++ = bw ? 0 : 255;
		}
	}

	if(img_save(&img, "foo.ppm") == -1) {
		fprintf(stderr, "failed to save file foo.ppm\n");
		return 1;
	}

	img_destroy(&img);
	return 0;
}
