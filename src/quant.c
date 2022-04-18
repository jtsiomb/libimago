/*
libimago - a multi-format image file input/output library.
Copyright (C) 2010-2022 John Tsiombikas <nuclear@member.fsf.org>

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include "imago2.h"

#define NUM_LEVELS	8

struct octnode;

struct octree {
	struct octnode *root;
	struct octnode *redlist[NUM_LEVELS];
	int redlev;
	int nleaves, maxcol;
};

struct octnode {
	int lvl;
	struct octree *tree;
	int r, g, b, nref;
	int palidx;
	int nsub, leaf;
	struct octnode *sub[8];
	struct octnode *next;
};


static int init_octree(struct octree *tree, int maxcol);
static void destroy_octree(struct octree *tree);

static struct octnode *alloc_node(struct octree *tree, int lvl);
static void free_node(struct octnode *n);
static void free_tree(struct octnode *n);

static void add_color(struct octree *tree, int r, int g, int b, int nref);
static void reduce_colors(struct octree *tree);
static int assign_colors(struct octnode *n, int next, struct img_colormap *cmap);
static int lookup_color(struct octree *tree, int r, int g, int b);
static int subidx(int bit, int r, int g, int b);
/*static void print_tree(struct octnode *n, int lvl);*/

#define CLAMP(x, a, b)	((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
static void add_error(unsigned char *dest, int *err, int s, int *acc)
{
	int r, g, b;
	if(s) {
		r = s * err[0] >> 4;
		g = s * err[1] >> 4;
		b = s * err[2] >> 4;
		acc[0] += r;
		acc[1] += g;
		acc[2] += b;
	} else {
		r = err[0];
		g = err[1];
		b = err[2];
	}
	r += dest[0];
	g += dest[1];
	b += dest[2];
	dest[0] = CLAMP(r, 0, 255);
	dest[1] = CLAMP(g, 0, 255);
	dest[2] = CLAMP(b, 0, 255);
}

int img_quantize(struct img_pixmap *img, int maxcol, enum img_dither dither)
{
	int i, j, cidx, rgbpitch;
	struct octree tree;
	struct img_pixmap newimg;
	struct img_colormap *cmap;
	unsigned char *dest, *rgb;
	int err[3], acc[3];

	if(maxcol < 2 || maxcol > 256) {
		return -1;
	}

	img_init(&newimg);
	if(img_set_pixels(&newimg, img->width, img->height, IMG_FMT_IDX8, 0) == -1) {
		return -1;
	}
	cmap = img_colormap(&newimg);

	/* convert the source image to rgb24 to work with the pixels directly */
	if(img_convert(img, IMG_FMT_RGB24) == -1) {
		img_destroy(&newimg);
		return -1;
	}
	rgbpitch = img->width * 3;

	if(init_octree(&tree, maxcol) == -1) {
		return -1;
	}

	rgb = img->pixels;
	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			add_color(&tree, rgb[0], rgb[1], rgb[2], 1);
			rgb += 3;

			while(tree.nleaves > maxcol) {
				reduce_colors(&tree);
			}
		}
	}

	/* use created octree to generate the palette */
	cmap->ncolors = assign_colors(tree.root, 0, cmap);

	/* replace image pixels */
	dest = newimg.pixels;
	rgb = img->pixels;
	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			cidx = lookup_color(&tree, rgb[0], rgb[1], rgb[2]);
			assert(cidx >= 0 && cidx < maxcol);
			*dest++ = cidx;

			switch(dither) {
			case IMG_DITHER_FLOYD_STEINBERG:
				err[0] = (int)rgb[0] - (int)cmap->color[cidx].r;
				err[1] = (int)rgb[1] - (int)cmap->color[cidx].g;
				err[2] = (int)rgb[2] - (int)cmap->color[cidx].b;
				acc[0] = acc[1] = acc[2] = 0;
				if(j < img->width - 1) {
					add_error(rgb + 3, err, 7, acc);
				}
				if(i < img->height - 1) {
					if(j > 0) {
						add_error(rgb + rgbpitch - 3, err, 3, acc);
					}
					add_error(rgb + rgbpitch, err, 5, acc);
					if(j < img->width - 1) {
						err[0] -= acc[0];
						err[1] -= acc[1];
						err[2] -= acc[2];
						add_error(rgb + rgbpitch + 3, err, 0, 0);
					}
				}
				break;
			default:
				break;
			}

			rgb += 3;
		}
	}

	newimg.name = img->name;
	img->name = 0;
	img_destroy(img);
	*img = newimg;

	destroy_octree(&tree);
	return 0;
}

#if 0
int img_gen_shades(struct img_pixmap *img, int levels, int maxcol, int *shade_lut)
{
	int i, j, cidx, r, g, b;
	unsigned int color[3];
	struct octree tree;
	struct img_pixmap newimg = *img;
	struct img_colormap *cmap = img_colormap(img);

	if(!cmap) return -1;

	if(maxcol < 2 || maxcol > 256) {
		return -1;
	}

	if(init_octree(&tree, maxcol) == -1) {
		return -1;
	}

	for(i=0; i<cmap->ncolors; i++) {
		add_color(&tree, cmap->color[i].r, cmap->color[i].g, cmap->color[i].b, 1024);
	}

	for(i=0; i<cmap->ncolors; i++) {
		for(j=0; j<levels - 1; j++) {
			r = cmap->color[i].r * j / (levels - 1);
			g = cmap->color[i].g * j / (levels - 1);
			b = cmap->color[i].b * j / (levels - 1);
			add_color(&tree, r, g, b, 1);

			while(tree.nleaves > maxcol) {
				reduce_colors(&tree);
			}
		}
	}

	newimg.cmap_ncolors = assign_colors(tree.root, 0, newimg.cmap);

	/* replace image pixels */
	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			get_pixel_rgb(img, j, i, color);
			cidx = lookup_color(&tree, color[0], color[1], color[2]);
			put_pixel(&newimg, j, i, cidx);
		}
	}

	/* populate shade_lut based on the new palette, can't generate levels only
	 * for the original colors, because the palette entries will have changed
	 * and moved around.
	 */
	for(i=0; i<newimg.cmap_ncolors; i++) {
		for(j=0; j<levels; j++) {
			r = newimg.cmap[i].r * j / (levels - 1);
			g = newimg.cmap[i].g * j / (levels - 1);
			b = newimg.cmap[i].b * j / (levels - 1);
			*shade_lut++ = lookup_color(&tree, r, g, b);
		}
	}
	for(i=0; i<(maxcol - newimg.cmap_ncolors) * levels; i++) {
		*shade_lut++ = maxcol - 1;
	}

	*img = newimg;

	destroy_octree(&tree);
	return 0;
}
#endif

static int init_octree(struct octree *tree, int maxcol)
{
	memset(tree, 0, sizeof *tree);

	tree->redlev = NUM_LEVELS - 1;
	tree->maxcol = maxcol;

	if(!(tree->root = alloc_node(tree, 0))) {
		return -1;
	}
	return 0;
}

static void destroy_octree(struct octree *tree)
{
	free_tree(tree->root);
}

static struct octnode *alloc_node(struct octree *tree, int lvl)
{
	struct octnode *n;

	if(!(n = calloc(1, sizeof *n))) {
		perror("failed to allocate octree node");
		return 0;
	}

	n->lvl = lvl;
	n->tree = tree;
	n->palidx = -1;

	if(lvl < tree->redlev) {
		n->next = tree->redlist[lvl];
		tree->redlist[lvl] = n;
	} else {
		n->leaf = 1;
		tree->nleaves++;
	}
	return n;
}

static void free_node(struct octnode *n)
{
	struct octnode *prev, dummy;

	dummy.next = n->tree->redlist[n->lvl];
	prev = &dummy;
	while(prev->next) {
		if(prev->next == n) {
			prev->next = n->next;
			break;
		}
		prev = prev->next;
	}
	n->tree->redlist[n->lvl] = dummy.next;

	if(n->leaf) {
		n->tree->nleaves--;
		assert(n->tree->nleaves >= 0);
	}
	free(n);
}

static void free_tree(struct octnode *n)
{
	int i;

	if(!n) return;

	for(i=0; i<8; i++) {
		free_tree(n->sub[i]);
	}
	free_node(n);
}

static void add_color(struct octree *tree, int r, int g, int b, int nref)
{
	int i, idx, rr, gg, bb;
	struct octnode *n;

	rr = r * nref;
	gg = g * nref;
	bb = b * nref;

	n = tree->root;
	n->r += rr;
	n->g += gg;
	n->b += bb;
	n->nref += nref;

	for(i=0; i<NUM_LEVELS; i++) {
		if(n->leaf) break;

		idx = subidx(i, r, g, b);

		if(!n->sub[idx]) {
			if(!(n->sub[idx] = alloc_node(tree, i + 1))) {
				continue;
			}
		}
		n->nsub++;
		n = n->sub[idx];

		n->r += rr;
		n->g += gg;
		n->b += bb;
		n->nref += nref;
	}
}

static struct octnode *get_reducible(struct octree *tree)
{
	int best_nref;
	struct octnode dummy, *n, *prev, *best_prev, *best = 0;

	while(tree->redlev >= 0) {
		best_nref = INT_MAX;
		best = 0;
		dummy.next = tree->redlist[tree->redlev];
		prev = &dummy;
		while(prev->next) {
			n = prev->next;
			if(n->nref < best_nref) {
				best = n;
				best_nref = n->nref;
				best_prev = prev;
			}
			prev = prev->next;
		}
		if(best) {
			best_prev->next = best->next;
			tree->redlist[tree->redlev] = dummy.next;
			break;
		}
		tree->redlev--;
	}

	return best;
}

static void reduce_colors(struct octree *tree)
{
	int i;
	struct octnode *n;

	if(!(n = get_reducible(tree))) {
		fprintf(stderr, "warning: no reducible nodes!\n");
		return;
	}
	for(i=0; i<8; i++) {
		if(n->sub[i]) {
			free_node(n->sub[i]);
			n->sub[i] = 0;
		}
	}
	n->leaf = 1;
	tree->nleaves++;
}

static int assign_colors(struct octnode *n, int next, struct img_colormap *cmap)
{
	int i;

	if(!n) return next;

	if(n->leaf) {
		assert(next < n->tree->maxcol);
		assert(n->nref);
		cmap->color[next].r = n->r / n->nref;
		cmap->color[next].g = n->g / n->nref;
		cmap->color[next].b = n->b / n->nref;
		n->palidx = next;
		return next + 1;
	}

	for(i=0; i<8; i++) {
		next = assign_colors(n->sub[i], next, cmap);
	}
	return next;
}

static int lookup_color(struct octree *tree, int r, int g, int b)
{
	int i, j, idx;
	struct octnode *n;

	n = tree->root;
	for(i=0; i<NUM_LEVELS; i++) {
		if(n->leaf) {
			assert(n->palidx >= 0);
			return n->palidx;
		}

		idx = subidx(i, r, g, b);
		for(j=0; j<8; j++) {
			if(n->sub[idx]) break;
			idx = (idx + 1) & 7;
		}

		assert(n->sub[idx]);
		n = n->sub[idx];
	}

	return 0;	/* lookup_color failed */
}

static int subidx(int bit, int r, int g, int b)
{
	assert(bit >= 0 && bit < NUM_LEVELS);
	bit = NUM_LEVELS - 1 - bit;
	return ((r >> bit) & 1) | ((g >> (bit - 1)) & 2) | ((b >> (bit - 2)) & 4);
}

/*
static void print_tree(struct octnode *n, int lvl)
{
	int i;
	char ptrbuf[32], *p;

	if(!n) return;

	for(i=0; i<lvl; i++) {
		fputs("|  ", stdout);
	}

	sprintf(ptrbuf, "%p", (void*)n);
	p = ptrbuf + strlen(ptrbuf) - 4;

	if(n->nref) {
		printf("+-(%d) %s: <%d %d %d> #%d", n->lvl, p, n->r / n->nref, n->g / n->nref,
				n->b / n->nref, n->nref);
	} else {
		printf("+-(%d) %s: <- - -> #0", n->lvl, p);
	}
	if(n->palidx >= 0) printf(" [%d]", n->palidx);
	if(n->leaf) printf(" LEAF");
	putchar('\n');

	for(i=0; i<8; i++) {
		print_tree(n->sub[i], lvl + 1);
	}
}
*/
