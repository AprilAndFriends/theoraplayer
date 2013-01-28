/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "yuv_c.h"

#define CLIP_RGB_COLOR(x) ((x & 0xFFFFFF00) == 0 ? x : (x & 0x80000000 ? 0 : 255))

int YTable [256];
int BUTable[256];
int GUTable[256];
int GVTable[256];
int RVTable[256];

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

void initYUVConversionModule()
{
	//used to bring the table into the high side (scale up) so we
	//can maintain high precision and not use floats (FIXED POINT)
	
	// this is the pseudocode for yuv->rgb conversion
	//        r = 1.164*(*ySrc - 16) + 1.596*(cv - 128);
	//        b = 1.164*(*ySrc - 16)                   + 2.018*(cu - 128);
	//        g = 1.164*(*ySrc - 16) - 0.813*(cv - 128) - 0.391*(cu - 128);
	
    double scale = 1L << 13, temp;
	
	int i;
	for (i = 0; i < 256; i++)
	{
		temp = i - 128;
		
		YTable[i]  = (int)((1.164 * scale + 0.5) * (i - 16));	//Calc Y component
		RVTable[i] = (int)((1.596 * scale + 0.5) * temp);		//Calc R component
		GUTable[i] = (int)((0.391 * scale + 0.5) * temp);		//Calc G u & v components
		GVTable[i] = (int)((0.813 * scale + 0.5) * temp);
		BUTable[i] = (int)((2.018 * scale + 0.5) * temp);		//Calc B component
	}
}
