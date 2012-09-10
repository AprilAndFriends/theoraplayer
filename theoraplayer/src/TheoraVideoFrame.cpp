/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include <memory.h>
#include <theora/theoradec.h>
#include "TheoraVideoFrame.h"
#include "TheoraVideoClip.h"

// clips a value between [0,255] using fast bitwise operations
#define CLIP_RGB_COLOR(x) ((x & 0xFFFFFF00) == 0 ? x : (x & 0x80000000 ? 0 : 255))

int YTable [256];
int BUTable[256];
int GUTable[256];
int GVTable[256];
int RVTable[256];

static void _decodeAlpha(th_img_plane* yuv, unsigned char* out, int stride)
{
	int y, width = yuv[0].width / 2;
	unsigned char *ySrc, *yLineEnd, *out1, luma;
	
	for (y=0; y<yuv[0].height; y++)
	{
		ySrc = yuv[0].data + y * yuv[0].stride + width;
		out1 = out + y * stride;
		
		for (yLineEnd = ySrc + width;ySrc != yLineEnd; ySrc++, out1 += 4)
		{
			luma = *ySrc;
			if (luma < 32) luma = 0;
			if (luma > 224) luma = 255;
			*out1 = luma;
		}
	}
}

static void _decodeRGB(th_img_plane* yuv,unsigned char* out,int stride,int nBytes,int max_width = 0)
{
	// BGR differs from RGB conversion fuction only in byte order
	// So if you make changes to the RGB function, make sure to mirror those
	// changes in the BGR function as well
	int rgbY,rV,gUV,bU,t,y, width = max_width == 0 ? yuv[0].width : max_width;
	unsigned char cv,cu;
	unsigned char *ySrc, *yLineEnd, *uSrc, *vSrc, *out1, *out2;
	
	for (y=0;y<yuv[0].height;y+=2)
	{
		ySrc = yuv[0].data + y * yuv[0].stride;
		uSrc = yuv[1].data + y * yuv[1].stride / 2;
		vSrc = yuv[2].data + y * yuv[2].stride / 2;
		out1 = out + y * stride;
		out2 = out + (y + 1) * stride;
		
		for (yLineEnd=ySrc+width,t=0;ySrc != yLineEnd;ySrc++,out1+=nBytes,out2+=nBytes,t=!t)
		{
			if (!t)
			{
				cu = *uSrc; cv = *vSrc;
				rV   = RVTable[cv];
				gUV  = GUTable[cu] + GVTable[cv];
				bU   = BUTable[cu];
			}
			else { uSrc++; vSrc++; }
			
			rgbY=YTable[*ySrc];
			out1[0] = CLIP_RGB_COLOR((rgbY + rV ) >> 13);
			out1[1] = CLIP_RGB_COLOR((rgbY - gUV) >> 13);
			out1[2] = CLIP_RGB_COLOR((rgbY + bU ) >> 13);
			
			rgbY=YTable[*(ySrc+yuv[0].stride)];
			out2[0] = CLIP_RGB_COLOR((rgbY + rV ) >> 13);
			out2[1] = CLIP_RGB_COLOR((rgbY - gUV) >> 13);
			out2[2] = CLIP_RGB_COLOR((rgbY + bU ) >> 13);
		}
	}
}

static void _decodeBGR(th_img_plane* yuv,unsigned char* out,int stride,int nBytes,int max_width = 0)
{
	// BGR differs from RGB conversion fuction only in byte order
	// So if you make changes to the RGB function, make sure to mirror those
	// changes in the BGR function as well
	int rgbY,rV,gUV,bU,t,y, width = max_width == 0 ? yuv[0].width : max_width;
	unsigned char cv,cu;
	unsigned char *ySrc, *yLineEnd, *uSrc, *vSrc, *out1, *out2;
	
	for (y=0;y<yuv[0].height;y+=2)
	{
		ySrc = yuv[0].data + y * yuv[0].stride;
		uSrc = yuv[1].data + y * yuv[1].stride / 2;
		vSrc = yuv[2].data + y * yuv[2].stride / 2;
		out1 = out + y * stride;
		out2 = out + (y + 1) * stride;
		
		for (yLineEnd=ySrc+width,t=0;ySrc != yLineEnd;ySrc++,out1+=nBytes,out2+=nBytes,t=!t)
		{
			if (!t)
			{
				cu = *uSrc; cv = *vSrc;
				rV   = RVTable[cv];
				gUV  = GUTable[cu] + GVTable[cv];
				bU   = BUTable[cu];
			}
			else { uSrc++; vSrc++; }
			
			rgbY=YTable[*ySrc];
			out1[2] = CLIP_RGB_COLOR((rgbY + rV ) >> 13);
			out1[1] = CLIP_RGB_COLOR((rgbY - gUV) >> 13);
			out1[0] = CLIP_RGB_COLOR((rgbY + bU ) >> 13);
			
			rgbY=YTable[*(ySrc+yuv[0].stride)];
			out2[2] = CLIP_RGB_COLOR((rgbY + rV ) >> 13);
			out2[1] = CLIP_RGB_COLOR((rgbY - gUV) >> 13);
			out2[0] = CLIP_RGB_COLOR((rgbY + bU ) >> 13);
		}
	}
}

static void decodeRGB(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeRGB(yuv,out,stride*3,3);
}

static void decodeRGBA(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeRGB(yuv, out, stride * 4, 4, yuv[0].width / 2);
	_decodeAlpha(yuv, out + 3, stride * 4);
}

static void decodeRGBX(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeRGB(yuv,out,stride*4,4);
}

static void decodeXRGB(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeRGB(yuv,out+1,stride*4,4);
}

static void decodeARGB(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeRGB(yuv,out+1,stride*4,4, yuv[0].width / 2);
	_decodeAlpha(yuv, out, stride * 4);
}

static void decodeBGR(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeBGR(yuv,out,stride*3,3);
}

static void decodeBGRX(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeBGR(yuv,out,stride*4,4);
}

static void decodeBGRA(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeBGR(yuv, out, stride * 4, 4, yuv[0].width / 2);
	_decodeAlpha(yuv, out + 3, stride * 4);
}

static void decodeXBGR(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeBGR(yuv,out+1,stride*4,4);
}

static void decodeABGR(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeBGR(yuv, out, stride * 4, 4, yuv[0].width / 2);
	_decodeAlpha(yuv, out, stride * 4);
}

static void decodeGrey(th_img_plane* yuv,unsigned char* out,int stride)
{
	unsigned char *ySrc=yuv[0].data,*yLineEnd;
	for (int y=0;y<yuv[0].height;y++,ySrc+=yuv[0].stride-yuv[0].width,out+=stride-yuv[0].width)
		for (yLineEnd=ySrc+yuv[0].width;ySrc != yLineEnd;ySrc++,out++)
			out[0]=*ySrc;
}

static void _decodeGrey3(th_img_plane* yuv,unsigned char* out,int stride,int nBytes)
{
	unsigned char *ySrc=yuv[0].data,*yLineEnd;
	for (int y=0;y<yuv[0].height;y++,ySrc+=yuv[0].stride-yuv[0].width,out+=stride-yuv[0].width*nBytes)
		for (yLineEnd=ySrc+yuv[0].width;ySrc != yLineEnd;ySrc++,out+=nBytes)
			out[0]=out[1]=out[2]=*ySrc;
}

static void decodeGrey3(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeGrey3(yuv,out,stride*3,3);
}

static void decodeGreyX(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeGrey3(yuv,out,stride*4,4);
}

static void decodeGreyA(th_img_plane* yuv,unsigned char* out,int stride)
{
	// TODO
}

static void decodeXGrey(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeGrey3(yuv,out+1,stride*4,4);
}

static void decodeAGrey(th_img_plane* yuv,unsigned char* out,int stride)
{
	// TODO
}


static void _decodeYUV(th_img_plane* yuv,unsigned char* out,int stride,int nBytes)
{
	int t,y;
	unsigned char cu,cv;
	unsigned char *ySrc=yuv[0].data,*yLineEnd,
				  *uSrc=yuv[1].data,
				  *vSrc=yuv[2].data,
	              *out2=out+stride;

	stride+=stride-yuv[0].width*nBytes;
	
	for (y=0;y<yuv[0].height;y+=2)
	{
		for (yLineEnd=ySrc+yuv[0].width,t=0;ySrc != yLineEnd;ySrc++,out+=nBytes,out2+=nBytes,t=!t)
		{
			if (!t) { cu=*uSrc; cv=*vSrc; }
			else { uSrc++; vSrc++; }
			out[0]  = *ySrc;
			out2[0] = *(ySrc+yuv[0].stride);
			out[1] = cu; out2[1] = cu;
			out[2] = cv; out2[2] = cv;
		}
		out+=stride; out2+=stride;
		ySrc+=yuv[0].stride*2-yuv[0].width;
		uSrc+=yuv[1].stride-yuv[1].width;
		vSrc+=yuv[2].stride-yuv[2].width;
	}
}

static void decodeYUV(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeYUV(yuv,out,stride*3,3);
}

static void decodeYUVX(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeYUV(yuv,out,stride*4,4);
}

static void decodeYUVA(th_img_plane* yuv,unsigned char* out,int stride)
{
	// TODO
}

static void decodeXYUV(th_img_plane* yuv,unsigned char* out,int stride)
{
	_decodeYUV(yuv,out+1,stride*4,4);
}

static void decodeAYUV(th_img_plane* yuv,unsigned char* out,int stride)
{
	// TODO
}

void (*conversion_functions[])(th_img_plane*,unsigned char*,int)={0,
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
// --------------------------------------------------------------
TheoraVideoFrame::TheoraVideoFrame(TheoraVideoClip* parent)
{
	mReady=mInUse=false;
	mParent=parent;
	mIteration=0;
	// number of bytes based on output mode
	int bytemap[]={0,3,4,4,4,4,3,4,4,4,4,1,3,4,4,4,4,3,4,4,4,4};
	int size=mParent->getStride() * mParent->mHeight * bytemap[mParent->getOutputMode()];
	mBuffer=new unsigned char[size];
	memset(mBuffer,255,size);
}

TheoraVideoFrame::~TheoraVideoFrame()
{
	if (mBuffer) delete [] mBuffer;
}

int TheoraVideoFrame::getWidth()
{
	return mParent->getWidth();
}

int TheoraVideoFrame::getStride()
{
	return mParent->mStride;
}

int TheoraVideoFrame::getHeight()
{
	return mParent->getHeight();
}

unsigned char* TheoraVideoFrame::getBuffer()
{
	return mBuffer;
}

void TheoraVideoFrame::decode(void* yuv)
{
	conversion_functions[mParent->getOutputMode()]((th_img_plane*) yuv,mBuffer,mParent->mStride);
	mReady=true;
}

void TheoraVideoFrame::clear()
{
	mInUse=mReady=false;
}

void createYUVtoRGBtables()
{
	//used to bring the table into the high side (scale up) so we
	//can maintain high precision and not use floats (FIXED POINT)

// this is the pseudocode for yuv->rgb conversion
//        r = 1.164*(*ySrc - 16) + 1.596*(cv - 128);
//        b = 1.164*(*ySrc - 16)                   + 2.018*(cu - 128);
//        g = 1.164*(*ySrc - 16) - 0.813*(cv - 128) - 0.391*(cu - 128);

    double scale = 1L << 13, temp;
	
	for (int i = 0; i < 256; i++)
	{
		temp = i - 128;

		YTable[i]  = (int)((1.164 * scale + 0.5) * (i - 16));	//Calc Y component
		RVTable[i] = (int)((1.596 * scale + 0.5) * temp);		//Calc R component
		GUTable[i] = (int)((0.391 * scale + 0.5) * temp);		//Calc G u & v components
		GVTable[i] = (int)((0.813 * scale + 0.5) * temp);
		BUTable[i] = (int)((2.018 * scale + 0.5) * temp);		//Calc B component
	}
}
