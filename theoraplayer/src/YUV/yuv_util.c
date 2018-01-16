/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <stdlib.h>

#include "yuv_util.h"

struct Theoraplayer_PixelTransform* incOut(struct Theoraplayer_PixelTransform* t, int n)
{
	// used for XRGB, XBGR and similar
	t->out += n;
	return t;
}

void _decodeAlpha(struct Theoraplayer_PixelTransform* t, int stride)
{
	unsigned char* ySrc = NULL;
	unsigned char* yLineEnd = NULL;
	unsigned char* out = NULL;
	int luma = 0;
	unsigned int y = 0;
	for (y = 0; y < t->h; y++)
	{
		ySrc = t->y + y * t->yStride + t->w;
		out = t->out + y * stride;
		yLineEnd = ySrc + t->w;
		while (ySrc != yLineEnd)
		{
			luma = (*ySrc);
			// because in YCbCr specification, luma values are in the range of [16, 235]
			// account for 'footroom' and 'headroom' ranges while using luma values as alpha channel
			if (luma <= 16)
			{
				*out = 0;
			}
			else if (luma >= 235)
			{
				*out = 255;
			}
			else
			{
				*out = (unsigned char)(((luma - 16) * 255) / 219);
			}
			++ySrc;
			out += 4;
		}
	}
}
