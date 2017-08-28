libimago
========

Overview
--------
Imago is a simple C library for reading and writing images in many different
image file formats.

Currently supported file formats:
 * PNG (requires libpng).
 * JPEG (requires libjpeg).
 * Targa: 24bit or 32bit, raw, or RLE compressed.
 * Portable PixMap (PPM), and Portable GreyMap (PGM).
 * Radiance shared exponent HDR (RGBE).
 * LBM: InterLeaved BitMap (ILBM), and Planar BitMap (PBM).

License
-------

Copyright (C) 2011-2017 John Tsiombikas <nuclear@member.fsf.org>

You may freely use, modify and/or redistribute libimago, under the terms of the
GNU Lesser General Public License (LGPL) version 3 (or at your option, any
later version published by the Free Software Foundation). See COPYING_ and
COPYING.LESSER_ for details.

Download
--------
Latest release: http://nuclear.mutantstargoat.com/sw/libimago/files/libimago-2.1.tar.gz

Grab the source code from github: https://github.com/jtsiomb/libimago

Web site: http://nuclear.mutantstargoat.com/sw/libimago

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

Build
-----
To build and install ``imago2`` on UNIX run::

 ./configure
 make
 make install

If you wish to avoid the ``libpng`` or ``libjpeg`` dependencies, you may disable
support for these formats by passing ``--disable-png`` or ``--disable-jpeg`` to
``configure``.

To build on windows try to open the included visual studio project. For this to
work you will have to download and build any dependencies (libpng, zlib, and
libjpeg), point the visual studio include/library paths to the correct
directories, and call the old ones from the deep.

If the build succeeds, a ``usr`` hierarchy will have been created in the project.
Feel free to run ``install.bat`` to copy it to ``C:\usr``, to try and impose some
semblance of sanity to the system.

To cross-compile for windows with mingw-w64, try the following incantation::

 ./configure --prefix=/usr/i686-w64-mingw32
 make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar sys=MINGW32
 make install sys=MINGW32

.. _COPYING: http://www.gnu.org/licenses/gpl
.. _COPYING.LESSER: http://www.gnu.org/licenses/lgpl
