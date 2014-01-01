/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "yuv_util.h"

struct TheoraPixelTransform* incOut(struct TheoraPixelTransform* t, int n)
{
	// used for XRGB, XBGR and similar
	t->out += n;
	return t;
}

void _decodeAlpha(struct TheoraPixelTransform* t, int stride)
{
	int width = t->w;
	unsigned char *ySrc, *yLineEnd, *out, luma;
	unsigned int y;
	for (y = 0; y < t->h; y++)
	{
		ySrc = t->y + y * t->yStride + width;
		out = t->out + y * stride;
		
		for (yLineEnd = ySrc + width; ySrc != yLineEnd; ySrc++, out += 4)
		{
			luma = *ySrc;
			if (luma < 32) luma = 0;
			if (luma > 224) luma = 255;
			*out = luma;
		}
	}
}
