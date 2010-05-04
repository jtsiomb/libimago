#ifndef CCIMAGO_H_
#define CCIMAGO_H_

#include "imago.h"


class ImgPixmap : public img_pixmap {
public:
	ImgPixmap(enum img_fmt fmt = IMG_FMT_RGBA32);
	~ImgPixmap();

	void set_pixels(int w, int h, img_fmt fmt, void *pix);

	bool load(const char *fname);
	bool save(const char *fname) const;

	bool load(FILE *fp);
	bool save(FILE *fp) const;

	bool read(const img_io &io);
	bool write(const img_io &io) const;
};

#endif	// CCIMAGO_H_
