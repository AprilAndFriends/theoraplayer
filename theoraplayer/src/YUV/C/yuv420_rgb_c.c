/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "yuv_c.h"

#define CLIP_RGB_COLOR(dst, x) \
	tmp = (x) >> 13;\
	if ((tmp & ~0xFF) == 0) dst = tmp;\
	else                    dst = (-tmp) >> 31;

#define _decodeRGB(stride, nBytes, maxWidth, i1, i2, i3, j1, j2, j3)\
	register int tmp;\
	int nBytes2 = nBytes * 2, cv, cu, rgbY1, rgbY2, rgbY3, rgbY4, rV, gUV, bU, width = maxWidth == 0 ? t->w : maxWidth;\
	unsigned int y;\
	unsigned char *ySrcEven, *ySrcOdd, *yLineEnd, *uSrc, *vSrc, *out1, *out2;\
	\
	for (y=0; y < t->h; y += 2)\
	{\
		ySrcEven = t->y + y * t->yStride;\
		ySrcOdd  = t->y + (y + 1) * t->yStride;\
		uSrc = t->u + y * t->uStride / 2;\
		vSrc = t->v + y * t->vStride / 2;\
		out1 = t->out + y * stride;\
		out2 = t->out + (y + 1) * stride;\
		\
		for (yLineEnd = ySrcEven + width; ySrcEven != yLineEnd;)\
		{\
			cu = *uSrc; uSrc++;\
			cv = *vSrc; vSrc++;\
			rV   = RVTable[cv];\
			gUV  = GUTable[cu] + GVTable[cv];\
			bU   = BUTable[cu];\
			\
			rgbY1 = YTable[*ySrcEven]; ySrcEven++;\
			rgbY2 = YTable[*ySrcOdd];  ySrcOdd++;\
			rgbY3 = YTable[*ySrcEven]; ySrcEven++;\
			rgbY4 = YTable[*ySrcOdd];  ySrcOdd++;\
			\
			CLIP_RGB_COLOR(out1[i1], rgbY1 + rV );\
			CLIP_RGB_COLOR(out1[i2], rgbY1 - gUV);\
			CLIP_RGB_COLOR(out1[i3], rgbY1 + bU );\
			\
			CLIP_RGB_COLOR(out2[i1], rgbY2 + rV );\
			CLIP_RGB_COLOR(out2[i2], rgbY2 - gUV);\
			CLIP_RGB_COLOR(out2[i3], rgbY2 + bU );\
			\
			CLIP_RGB_COLOR(out1[j1], rgbY3 + rV );\
			CLIP_RGB_COLOR(out1[j2], rgbY3 - gUV);\
			CLIP_RGB_COLOR(out1[j3], rgbY3 + bU );\
			\
			CLIP_RGB_COLOR(out2[j1], rgbY4 + rV );\
			CLIP_RGB_COLOR(out2[j2], rgbY4 - gUV);\
			CLIP_RGB_COLOR(out2[j3], rgbY4 + bU );\
			\
			out1 += nBytes2;  out2 += nBytes2;\
		}\
	}

void decodeRGB(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 3, 3, 0, 0, 1, 2, 3, 4, 5);
}

void decodeRGBA(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 4, 4, 0, 0, 1, 2, 4, 5, 6);
	_decodeAlpha(incOut(t, 3), t->w * 4);
}

void decodeRGBX(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 4, 4, 0, 0, 1, 2, 4, 5, 6);
}

void decodeARGB(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 4, 4, 0, 1, 2, 3, 5, 6, 7);
	_decodeAlpha(t, t->w * 4);
}

void decodeXRGB(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 4, 4, 0, 1, 2, 3, 5, 6, 7);
}

void decodeBGR(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 3, 3, 0, 2, 1, 0, 5, 4, 3);
}

void decodeBGRA(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 4, 4, 0, 2, 1, 0, 6, 5, 4);
	_decodeAlpha(incOut(t, 3), t->w * 4);
}

void decodeBGRX(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 4, 4, 0, 2, 1, 0, 6, 5, 4);
}

void decodeABGR(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 4, 4, 0, 3, 2, 1, 7, 6, 5);
	_decodeAlpha(t, t->w * 4);
}

void decodeXBGR(struct TheoraPixelTransform* t)
{
	_decodeRGB(t->w * 4, 4, 0, 3, 2, 1, 7, 6, 5);
}
