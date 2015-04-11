#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <alloca.h>
#include <imago2.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define LOAD_XSZ	400
#define LOAD_YSZ	160

struct image {
	char *name;
	int width, height;
	void *pixels;
	Pixmap xpm;
};

struct rect {
	int x, y, w, h;
};

int process_args(int argc, char **argv);
void next_image(void);
Window create_window(const char *title, int width, int height);
void set_win_title(Window w, const char *title);
Pixmap create_pixmap(int xsz, int ysz, unsigned char *pixels);
void draw_rect(struct rect *rect);
void redisplay(void);

struct image *imglist;
int nimg, cur = -1;

Display *dpy;
Window win;
GC wingc;
Visual *vis;
Atom xa_wm_proto, xa_wm_delwin;

int main(int argc, char **argv)
{
	int i;

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to open display\n");
		return 1;
	}
	xa_wm_proto = XInternAtom(dpy, "WM_PROTOCOLS", False);
	xa_wm_delwin = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

	if(!(win = create_window("imago viewer", LOAD_XSZ, LOAD_YSZ))) {
		fprintf(stderr, "failed to create window\n");
		return 1;
	}

	if(process_args(argc, argv) == -1) {
		return 1;
	}

	next_image();

	for(;;) {
		XEvent ev;
		XNextEvent(dpy, &ev);

		switch(ev.type) {
		case Expose:
			{
				struct rect rect;
				rect.x = ev.xexpose.x;
				rect.y = ev.xexpose.y;
				rect.w = ev.xexpose.width;
				rect.h = ev.xexpose.height;
				draw_rect(&rect);
			}
			break;

		case ClientMessage:
			if(ev.xclient.message_type == xa_wm_proto &&
					ev.xclient.data.l[0] == xa_wm_delwin) {
				goto quit;
			}
			break;

		case KeyPress:
			{
				KeySym sym = XKeycodeToKeysym(dpy, ev.xkey.keycode, 0);
				switch(sym) {
				case XK_space:
					next_image();
					redisplay();
					break;

				case XK_Escape:
				case XK_Q:
				case XK_q:
					goto quit;
				}
			}
			break;
		}
	}

quit:
	for(i=0; i<nimg; i++) {
		if(imglist[i].xpm) {
			XFreePixmap(dpy, imglist[i].xpm);
		}
		img_free_pixels(imglist[i].pixels);
	}
	XFreeGC(dpy, wingc);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	return 0;
}

int process_args(int argc, char **argv)
{
	int i;

	if(!(imglist = calloc(argc - 1, sizeof *imglist))) {
		perror("failed to allocate image list");
		return -1;
	}

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			fprintf(stderr, "invalid option: %s\n", argv[i]);
			return -1;
		} else {
			void *pix;
			int x, y;
			char *last_slash = strrchr(argv[i], '/');

			if(!(pix = img_load_pixels(argv[i], &x, &y, IMG_FMT_RGBA32))) {
				fprintf(stderr, "failed to load image: %s\n", argv[i]);
				continue;
			}
			imglist[nimg].name = last_slash ? last_slash + 1 : argv[i];
			imglist[nimg].pixels = pix;
			imglist[nimg].width = x;
			imglist[nimg].height = y;
			imglist[nimg].xpm = create_pixmap(x, y, pix);
			++nimg;
		}
	}

	if(!nimg) {
		fprintf(stderr, "no images to show\n");
		return -1;
	}
	return 0;
}

void next_image(void)
{
	char *title;

	cur = (cur + 1) % nimg;
	XResizeWindow(dpy, win, imglist[cur].width, imglist[cur].height);

	title = alloca(strlen(imglist[cur].name) + 16);
	sprintf(title, "[%d/%d] %s\n", cur + 1, nimg, imglist[cur].name);
	set_win_title(win, title);
}

Window create_window(const char *title, int width, int height)
{
	XVisualInfo visinfo;
	XSetWindowAttributes xattr;
	int scr = DefaultScreen(dpy);
	Window root = RootWindow(dpy, scr);

	if(!XMatchVisualInfo(dpy, scr, 24, TrueColor, &visinfo)) {
		fprintf(stderr, "failed to find a suitable visual\n");
		return 0;
	}
	vis = visinfo.visual;

	xattr.background_pixel = xattr.border_pixel = BlackPixel(dpy, scr);
	xattr.colormap = XCreateColormap(dpy, root, vis, AllocNone);

	win = XCreateWindow(dpy, root, 0, 0, width, height, 0, 24, InputOutput, vis,
			CWBackPixel | CWBorderPixel | CWColormap, &xattr);
	if(!win) {
		return 0;
	}
	XSelectInput(dpy, win, ExposureMask | KeyPressMask);
	XSetWMProtocols(dpy, win, &xa_wm_delwin, 1);

	wingc = XCreateGC(dpy, win, 0, 0);

	XMapWindow(dpy, win);
	return win;
}

void set_win_title(Window w, const char *title)
{
	XTextProperty prop;
	XStringListToTextProperty((char**)&title, 1, &prop);
	XSetWMName(dpy, w, &prop);
	XSetWMIconName(dpy, w, &prop);
	XFree(prop.value);
}

static int count_zero(unsigned int x)
{
	int n = 0;
	while((x & 1) == 0) {
		++n;
		x >>= 1;
	}
	return n;
}

Pixmap create_pixmap(int xsz, int ysz, unsigned char *pixels)
{
	int i, rshift, gshift, bshift;
	Pixmap xpm;
	XImage *ximg;
	GC gc;
	uint32_t *xpix;
	Window root = DefaultRootWindow(dpy);

	if(!(xpix = malloc(xsz * ysz * sizeof *xpix))) {
		return 0;
	}

	if(!(ximg = XCreateImage(dpy, vis, 24, ZPixmap, 0, (char*)xpix, xsz, ysz, 8, 0))) {
		return 0;
	}

	rshift = count_zero(ximg->red_mask);
	gshift = count_zero(ximg->green_mask);
	bshift = count_zero(ximg->blue_mask);

	for(i=0; i<xsz * ysz; i++) {
		xpix[i] = (*pixels++ << rshift) & ximg->red_mask;
		xpix[i] |= (*pixels++ << gshift) & ximg->green_mask;
		xpix[i] |= (*pixels++ << bshift) & ximg->blue_mask;
		++pixels;
	}

	xpm = XCreatePixmap(dpy, root, xsz, ysz, 24);
	gc = XCreateGC(dpy, xpm, 0, 0);

	XPutImage(dpy, xpm, gc, ximg, 0, 0, 0, 0, xsz, ysz);
	XDestroyImage(ximg);
	XFreeGC(dpy, gc);
	return xpm;
}

void draw_rect(struct rect *rect)
{
	XCopyArea(dpy, imglist[cur].xpm, win, wingc, rect->x, rect->y,
			rect->w, rect->h, rect->x, rect->y);
}

void redisplay(void)
{
	XEvent ev;
	ev.type = Expose;
	ev.xexpose.x = ev.xexpose.y = 0;
	ev.xexpose.width = imglist[cur].width;
	ev.xexpose.height = imglist[cur].height;
	ev.xexpose.count = 0;
	XSendEvent(dpy, win, False, 0, &ev);
}
