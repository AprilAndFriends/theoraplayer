/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "TheoraVideoFrame_AVFoundation.h"
#import <CoreVideo/CVPixelBuffer.h>

static void _decodeAlpha(unsigned char* src, unsigned char* out, int w, int h, int srcStride, int dstStride)
{
	int y;
	unsigned char *ySrc, *yLineEnd, *out1, luma;
	
	for (y = 0; y < h; y++)
	{
		ySrc = src + y * srcStride + w * 4;
		out1 = out + y * dstStride;
		
		for (yLineEnd = ySrc + w * 4; ySrc != yLineEnd; ySrc += 4, out1 += 4)
		{
			luma = *ySrc;
			if (luma < 32) luma = 0;
			if (luma > 224) luma = 255;
			*out1 = luma;
		}
	}
}

static void _decodeRGB(unsigned char* src, unsigned char* out, int w, int h, int srcStride, int dstStride, int nBytes, int max_width = 0)
{
	int y, srcDiff = srcStride - w * 4, dstDiff = dstStride - w * nBytes;
	unsigned char *psrc = src, *dst = out, *srcEnd;
	for (y = 0; y < h; y++)
	{
		dst = out + y * dstStride;
		srcEnd = psrc + w * 4;
		while (psrc != srcEnd)
		{
			dst[0] = psrc[2];
			dst[1] = psrc[1];
			dst[2] = psrc[0];
			psrc += 4;
			dst += nBytes;
		}
		psrc += srcDiff;
		dst += dstDiff;
	}
}

static void _decodeBGR(unsigned char* src, unsigned char* out, int w, int h, int srcStride, int dstStride, int nBytes, int max_width = 0)
{
	// TODO
}

static void decodeRGB(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeRGB(src, out, w, h, srcStride, w * 3, 3);
}

static void decodeRGBA(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeRGB(src, out, w, h, srcStride, w * 4, 4, w / 2);
	_decodeAlpha(src, out + 3, w, h, srcStride, w * 4);
}

static void decodeRGBX(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeRGB(src, out, w, h, srcStride, w * 4, 4);
}

static void decodeXRGB(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeRGB(src, out + 1, w, h, srcStride, w * 4, 4);
}

static void decodeARGB(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeRGB(src,out + 1, w, h, srcStride, w * 4, 4, w / 2);
	_decodeAlpha(src, out, w, h, srcStride, w * 4);
}

static void decodeBGR(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeBGR(src, out, w, h, srcStride, w * 3, 3);
}

static void decodeBGRX(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	if (srcStride == w * 4)
	{
		memcpy(out, src, w * h * 4);
	}
	else
	{
		for (int y = 0; y < h; y++, out += w * 4, src += srcStride)
			memcpy(out, src, w * 4);
	}
}

static void decodeBGRA(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeBGR(src, out, w, h, srcStride, w * 4, 4, w / 2);
	_decodeAlpha(src, out + 3, w, h, srcStride, w * 4);
}

static void decodeXBGR(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeBGR(src, out + 1, w, h, srcStride, w * 4, 4);
}

static void decodeABGR(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeBGR(src, out, w, h, srcStride, w * 4, 4, w / 2);
	_decodeAlpha(src, out, w, h, srcStride, w * 4);
}

static void decodeGrey(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	// TODO
}

static void _decodeGrey3(unsigned char* src,unsigned char* out, int w, int h, int srcStride, int dstStride, int nBytes)
{
	// TODO
}

static void decodeGrey3(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeGrey3(src, out, w, h, srcStride, w * 3, 3);
}

static void decodeGreyX(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeGrey3(src, out, w, h, srcStride, w * 4, 4);
}

static void decodeGreyA(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	// TODO
}

static void decodeXGrey(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeGrey3(src, out + 1, w, h, srcStride, w * 4, 4);
}

static void decodeAGrey(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	// TODO
}

unsigned int swapByteOrder(unsigned int ui)
{
    return (ui >> 24) | ((ui<<8) & 0x00FF0000) | ((ui>>8) & 0x0000FF00) | (ui << 24);
}

static void _decodeYUV(unsigned char* src, unsigned char* out, int w, int h, int srcStride, int dstStride, int nBytes)
{
	CVPlanarPixelBufferInfo_YCbCrPlanar* bigEndianYuv = (CVPlanarPixelBufferInfo_YCbCrPlanar*) src;
	CVPlanarPixelBufferInfo_YCbCrPlanar yuv;
	yuv.componentInfoY.offset = swapByteOrder(bigEndianYuv->componentInfoY.offset);
	yuv.componentInfoY.rowBytes = swapByteOrder(bigEndianYuv->componentInfoY.rowBytes);
	yuv.componentInfoCb.offset = swapByteOrder(bigEndianYuv->componentInfoCb.offset);
	yuv.componentInfoCb.rowBytes = swapByteOrder(bigEndianYuv->componentInfoCb.rowBytes);
	yuv.componentInfoCr.offset = swapByteOrder(bigEndianYuv->componentInfoCr.offset);
	yuv.componentInfoCr.rowBytes = swapByteOrder(bigEndianYuv->componentInfoCr.rowBytes);
	
	int t,y;
	unsigned char cu,cv;
	unsigned char *ySrc=src + yuv.componentInfoY.offset, *yLineEnd,
	*uSrc=src + yuv.componentInfoCr.offset,
	*vSrc=src + yuv.componentInfoCr.offset,
	*out2=out+dstStride;
	
	dstStride+=dstStride-w*nBytes;
	
	for (y=0;y<h;y+=2)
	{
		for (yLineEnd=ySrc+w,t=0;ySrc != yLineEnd;ySrc++,out+=nBytes,out2+=nBytes,t=!t)
		{
			if (!t) { cu=*uSrc; cv=*vSrc; }
			else { uSrc++; vSrc++; }
			out[0]  = *ySrc;
			out2[0] = *(ySrc+yuv.componentInfoY.rowBytes);
			out[1] = cu; out2[1] = cu;
			out[2] = cv; out2[2] = cv;
		}
		out+=dstStride; out2+=dstStride;
		ySrc+=yuv.componentInfoY.rowBytes*2-w;
		uSrc+=yuv.componentInfoCr.rowBytes-w/2;
		vSrc+=yuv.componentInfoCb.rowBytes-w/2;
	}
}

static void decodeYUV(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeYUV(src, out, w, h, srcStride, w * 3, 3);
}

static void decodeYUVX(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeYUV(src, out, w, h, srcStride, w * 4, 4);
}

static void decodeYUVA(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	// TODO
}

static void decodeXYUV(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	_decodeYUV(src, out + 1, w, h, srcStride, w * 4, 4);
}

static void decodeAYUV(unsigned char* src, unsigned char* out, int w, int h, int srcStride)
{
	// TODO
}

static void (*conversion_functions[])(unsigned char*, unsigned char*, int, int, int) = {0,
	decodeRGB,
	decodeRGBA,
	decodeRGBX,
	decodeARGB,
	decodeXRGB,
	decodeBGR,
	decodeBGRA,
	decodeBGRX,
	decodeABGR,
	decodeXBGR,
	decodeGrey,
	decodeGrey3,
	decodeGreyA,
	decodeGreyX,
	decodeAGrey,
	decodeXGrey,
	decodeYUV,
	decodeYUVA,
	decodeYUVX,
	decodeAYUV,
	decodeXYUV
};

TheoraVideoFrame_AVFoundation::TheoraVideoFrame_AVFoundation(TheoraVideoClip* clip) : TheoraVideoFrame(clip)
{
	
}

void TheoraVideoFrame_AVFoundation::decode(void* src, TheoraOutputMode srcFormat)
{
	conversion_functions[mParent->getOutputMode()]((unsigned char*) src, mBuffer, mParent->getWidth(), mParent->getHeight(), mParent->getStride());
	mReady = true;
}

TheoraFrameQueue_AVFoundation::TheoraFrameQueue_AVFoundation(TheoraVideoClip* parent) : TheoraFrameQueue(parent)
{
	
}

TheoraVideoFrame* TheoraFrameQueue_AVFoundation::createFrameInstance(TheoraVideoClip* clip)
{
	return new TheoraVideoFrame_AVFoundation(clip);
}
