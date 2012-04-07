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

#define GL_TEXTURE_2D			0x0de1
#define GL_TEXTURE_WRAP_S		0x2802
#define GL_TEXTURE_WRAP_T		0x2803
#define GL_TEXTURE_MAG_FILTER	0x2800
#define GL_TEXTURE_MIN_FILTER	0x2801
#define GL_LINEAR				0x2601
#define GL_REPEAT				0x2901


typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef void GLvoid;

/* for the same reason I'll load GL functions dynamically */
static void (*gl_gen_textures)(GLsizei, GLuint*);
/*static void (*gl_delete_textures)(GLsizei, const GLuint*);*/
static void (*gl_bind_texture)(GLenum, GLuint);
static void (*gl_tex_parameteri)(GLenum, GLenum, GLint);
static void (*gl_tex_image2d)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);

static int load_glfunc(void);

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

unsigned int img_gltexture(struct img_pixmap *img)
{
	unsigned int tex;
	unsigned int intfmt, fmt, type;

	if(!gl_gen_textures) {
		if(load_glfunc() == -1) {
			fprintf(stderr, "imago: failed to initialize the OpenGL helpers\n");
			return 0;
		}
	}

	intfmt = img_glintfmt(img);
	fmt = img_glfmt(img);
	type = img_gltype(img);

	gl_gen_textures(1, &tex);
	gl_bind_texture(GL_TEXTURE_2D, tex);
	gl_tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl_tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	gl_tex_parameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gl_tex_image2d(GL_TEXTURE_2D, 0, intfmt, img->width, img->height, 0, fmt, type, img->pixels);
	return tex;
}

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

static int load_glfunc(void)
{
#if defined(__unix__) || defined(__APPLE__)
	gl_gen_textures = dlsym(0, "glGenTextures");
	gl_bind_texture = dlsym(0, "glBindTexture");
	gl_tex_parameteri = dlsym(0, "glTexParameteri");
	gl_tex_image2d = dlsym(0, "glTexImage2D");
#endif

#ifdef WIN32
	HANDLE handle = GetModuleHandle(0);
	gl_gen_textures = GetProcAddress(handle, "glGenTextures");
	gl_bind_texture = GetProcAddress(handle, "glBindTexture");
	gl_tex_parameteri = GetProcAddress(handle, "glTexParameteri");
	gl_tex_image2d = GetProcAddress(handle, "glTexImage2D");
#endif

	return (gl_gen_textures && gl_bind_texture && gl_tex_parameteri && gl_tex_image2d) ? 0 : -1;
}
