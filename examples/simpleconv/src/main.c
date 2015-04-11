#include <stdio.h>
#include <imago2.h>

int main(int argc, char **argv)
{
	const char *infile = "foo.jpg";
	const char *outfile = "bar.jpg";
	int i, j, xsz = 512, ysz = 512;
	struct img_pixmap img;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-' && argv[i][2] == 0) {
			switch(argv[i][1]) {
			case 'i':
				infile = argv[++i];
				break;

			case 'o':
				outfile = argv[++i];
				break;

			default:
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return 1;
			}
		} else {
			fprintf(stderr, "invalid argument: %s\n", argv[i]);
			return 1;
		}
	}

	img_init(&img);

	if(img_load(&img, infile) == -1) {
		unsigned char *pix;

		fprintf(stderr, "failed to load image: %s, generating instead\n", infile);

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

	if(img_save(&img, outfile) == -1) {
		fprintf(stderr, "failed to save file %s\n", outfile);
		return 1;
	}

	img_destroy(&img);
	return 0;
}
