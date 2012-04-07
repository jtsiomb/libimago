#include "imago2.h"

/* to avoid dependency to OpenGL, I'll define all the relevant GL macros manually */
#define GL_UNSIGNED_BYTE	0x1401
#define GL_FLOAT			0x1406

#define GL_LUMINANCE		0x1909
#define GL_RGB				0x1907
#define GL_RGBA				0x1908

#define GL_RGBA32F			0x8814
#define GL_RGB32F			0x8815
#define GL_LUMINANCE32F		0x8818


unsigned int img_fmt_glfmt(enum img_fmt fmt)
{
	switch(fmt) {
	case IMG_FMT_GREY8:
	case IMG_FMT_GREYF:
		return GL_LUMINANCE;

	case IMG_FMT_RGB24:
	case IMG_FMT_RGBF:
		return GL_RGB;

	case IMG_FMT_RGBA32:
	case IMG_FMT_RGBAF:
		return GL_RGBA;

	default:
		break;
	}
	return 0;
}

unsigned int img_fmt_gltype(enum img_fmt fmt)
{
	switch(fmt) {
	case IMG_FMT_GREY8:
	case IMG_FMT_RGB24:
	case IMG_FMT_RGBA32:
		return GL_UNSIGNED_BYTE;

	case IMG_FMT_GREYF:
	case IMG_FMT_RGBF:
	case IMG_FMT_RGBAF:
		return GL_FLOAT;

	default:
		break;
	}
	return 0;
}

unsigned int img_fmt_glintfmt(enum img_fmt fmt)
{
	switch(fmt) {
	case IMG_FMT_GREY8:
		return GL_LUMINANCE;
	case IMG_FMT_RGB24:
		return GL_RGB;
	case IMG_FMT_RGBA32:
		return GL_RGBA;
	case IMG_FMT_GREYF:
		return GL_LUMINANCE32F;
	case IMG_FMT_RGBF:
		return GL_RGB32F;
	case IMG_FMT_RGBAF:
		return GL_RGBA32F;
	default:
		break;
	}
	return 0;
}

unsigned int img_glfmt(struct img_pixmap *img)
{
	return img_fmt_glfmt(img->fmt);
}

unsigned int img_gltype(struct img_pixmap *img)
{
	return img_fmt_gltype(img->fmt);
}

unsigned int img_glintfmt(struct img_pixmap *img)
{
	return img_fmt_glintfmt(img->fmt);
}
