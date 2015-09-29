/***************************************************\
* 						    *
* 	File: RasterFile.h			    *
* 	Copyright (c) 1984 IBM			    *
* 	Date: Thu May 24 17:30:37 1984		    *
* 	Author: James Gosling			    *
* 						    *
* Definition of the layout of a raster image file.  *
* 						    *
\***************************************************/

struct RasterHeader {
	int Magic;
	short width;		/* Width in pixels */
	short height;		/* Height in pixels */
	short depth;		/* number of bits per pixel */
};				/* This is followed by the bits of the image,
				   one row after another.  Each row is padded
				   to a multiple of 16 bits. */

#define RasterMagic 0xF10040BB
