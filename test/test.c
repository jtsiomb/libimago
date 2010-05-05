#include <stdio.h>
#include <imago2.h>

#define INFILE	"foo.jpg"
#define OUTFILE	"bar.jpg"

int main(void)
{
	int i, j, xsz = 512, ysz = 512;
	struct img_pixmap img;

	img_init(&img, IMG_FMT_RGB24);

	if(img_load(&img, INFILE) == -1) {
		unsigned char *pix;

		fprintf(stderr, "failed to load image: " INFILE ", generating instead\n");

		if(img_set_pixels(&img, xsz, ysz, IMG_FMT_RGB24, 0) == -1) {
			perror("wtf");
			return 1;
		}

		pix = img.pixels;
		for(i=0; i<ysz; i++) {
			for(j=0; j<xsz; j++) {
				int bw = ((i >> 5) & 1) == ((j >> 5) & 1);

				*pix++ = bw ? 255 : 0;
				*pix++ = 127;
				*pix++ = bw ? 0 : 255;
			}
		}
	}

	if(img_save(&img, OUTFILE) == -1) {
		fprintf(stderr, "failed to save file " OUTFILE "\n");
		return 1;
	}

	img_destroy(&img);
	return 0;
}
