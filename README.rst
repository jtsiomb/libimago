libimago
========

Overview
--------
Imago is a simple C library for reading and writing images in many different
image file formats.

Currently supported file formats:
 * PNG (requires libpng).
 * JPEG (requires libjpeg).
 * Targa: 24/32bit, raw or RLE compressed.
 * Portable pixmap (PPM), and portable greymap (PGM).
 * Radiance shared exponent HDR (RGBE).

Download
--------
Latest release: http://nuclear.mutantstargoat.com/sw/libimago/files/libimago-2.0.tar.gz

Grab the source code from github: https://github.com/jtsiomb/libimago

web site: http://nuclear.mutantstargoat.com/sw/libimago

License
-------

Copyright (C) 2011-2017 John Tsiombikas <nuclear@member.fsf.org>

You may freely use, modify and/or redistribute libimago, under the terms of the
GNU Lesser General Public License (LGPL) version 3 (or at your option, any
later version published by the Free Software Foundation). See COPYING_ and
COPYING.LESSER_ for details.

Usage example
-------------

Check out the example program under ``test/``, and the *heavily*
commented ``imago2.h`` header file, to find out how to use libimago.

The simplest way to load image data in RGBA 32bit is:

.. code:: c

 int width, height;
 unsigned char *pixels = img_load_pixels("foo.png", &width, &height, IMG_FMT_RGBA32);
 img_free_pixels(pixels);

To load image data in the closest possible format to whatever is natively
stored in each particular image file, use:

.. code:: c
 struct img_pixmap img;
 img_init(&img);
 img_load(&img, "foo.png");
 /* use img.fmt to determine the actual pixel format we got */
 img_destroy(&img);

There's also an optional interface for loading an image and creating an OpenGL
texture out of it in a single call:

.. code:: c

 unsigned int texture = img_gltexture_load("foo.png");

.. _COPYING: http://www.gnu.org/licenses/gpl
.. _COPYING.LESSER: http://www.gnu.org/licenses/lgpl
