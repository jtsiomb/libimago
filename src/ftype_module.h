#ifndef FTYPE_MODULE_H_
#define FTYPE_MODULE_H_

#include "imago2.h"

struct ftype_module {
	char *suffix;	/* used for format autodetection during saving only */

	int (*check)(struct img_io *io);
	int (*read)(struct img_pixmap *img, struct img_io *io);
	int (*write)(struct img_pixmap *img, struct img_io *io);
};

int img_register_module(struct ftype_module *mod);

struct ftype_module *img_find_format_module(struct img_io *io);
struct ftype_module *img_guess_format(const char *fname);
struct ftype_module *img_get_module(int idx);


#endif	/* FTYPE_MODULE_H_ */
