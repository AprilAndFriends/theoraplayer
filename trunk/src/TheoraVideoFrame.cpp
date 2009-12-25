/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2009 Kresimir Spes (kreso@cateia.com)

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the 
Free Software Foundation; either version 2 of the License, or (at your option) 
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
*************************************************************************************/
#include "TheoraVideoFrame.h"
#include "TheoraVideoClip.h"
#include <theora/theoradec.h>

#define TH_MAX( a, b ) ((a > b) ? a : b)
#define TH_MIN( a, b ) ((a < b) ? a : b)
//#define CLIP_RGB_COLOR( rgb_color_test ) TH_MAX( TH_MIN(rgb_color_test, 255), 0 )

// this is the bitwise version of the above code, twice as fast!
#define CLIP_RGB_COLOR(x) ((x & 0xFFFFFF00) == 0 ? x : (x & 0x80000000 ? 0 : 255))


// the folowing macros are used to simplify conversion functions
#define FOREACH_PIXEL_422 int t,y;\
unsigned char *ySrc=yuv[0].data,*yLineEnd,\
			  *uSrc=yuv[1].data,\
              *vSrc=yuv[2].data;\
for (y=0;y<yuv[0].height;y++) {\
	for (yLineEnd=ySrc+yuv[0].width,t=0;ySrc != yLineEnd;ySrc++) {
// ---------------------------------------------------------------
#define FOREACH_PIXEL_422_END if (t=!t == 1) { uSrc++; vSrc++; }\
}\
ySrc+=yuv[0].stride-yuv[0].width;\
uSrc-=yuv[1].width; vSrc-=yuv[2].width;\
if (y%2 == 1) { uSrc+=yuv[1].stride; vSrc+=yuv[2].stride; }\
}
// end conversion macros


unsigned int YTable [256];
unsigned int BUTable[256];
unsigned int GUTable[256];
unsigned int GVTable[256];
unsigned int RVTable[256];

void decodeRGB(th_img_plane* yuv,unsigned char* out)
{
	int rgbY,rV,gUV,bU;
	unsigned char cv,cu;
	int t,y;
	unsigned char *ySrc=yuv[0].data,*yLineEnd,
				  *uSrc=yuv[1].data,
				  *vSrc=yuv[2].data;
	unsigned char* out2=out+yuv[0].width*3;

	for (y=0;y<yuv[0].height;y+=2)
	{
		for (yLineEnd=ySrc+yuv[0].width,t=0;ySrc != yLineEnd;ySrc++)
		{
			rgbY=YTable[*ySrc];
			if (!t)
			{
				cu=*uSrc; cv=*vSrc;
				rV   = RVTable[cv];
				gUV  = GUTable[cu] + GVTable[cv];
				bU   = BUTable[cu];
			}
			out[0] = CLIP_RGB_COLOR((rgbY + rV ) >> 13);
			out[1] = CLIP_RGB_COLOR((rgbY - gUV) >> 13);
			out[2] = CLIP_RGB_COLOR((rgbY + bU ) >> 13);
			rgbY=YTable[*(ySrc+yuv[0].stride)];
			out2[0] = CLIP_RGB_COLOR((rgbY + rV ) >> 13);
			out2[1] = CLIP_RGB_COLOR((rgbY - gUV) >> 13);
			out2[2] = CLIP_RGB_COLOR((rgbY + bU ) >> 13);

			out+=3; out2+=3;

			if (t=!t == 1) { uSrc++; vSrc++; }
		}
		out+=3*yuv[0].width; out2+=3*yuv[0].width;
		ySrc+=yuv[0].stride*2-yuv[0].width;
		//uSrc-=yuv[1].width; vSrc-=yuv[2].width;
		uSrc+=yuv[1].stride-yuv[1].width;
		vSrc+=yuv[2].stride-yuv[2].width;
	}
}

void decodeGrey(th_img_plane* yuv,unsigned char* out)
{
	unsigned char *ySrc=yuv[0].data,*yLineEnd;
	for (int y=0;y<yuv[0].height;y++,ySrc+=yuv[0].stride-yuv[0].width)
		for (yLineEnd=ySrc+yuv[0].width;ySrc != yLineEnd;ySrc++,out++)
			out[0]=*ySrc;
}

void decodeGrey3(th_img_plane* yuv,unsigned char* out)
{
	unsigned char *ySrc=yuv[0].data,*yLineEnd;
	for (int y=0;y<yuv[0].height;y++,ySrc+=yuv[0].stride-yuv[0].width)
		for (yLineEnd=ySrc+yuv[0].width;ySrc != yLineEnd;ySrc++,out+=3)
			out[0]=out[1]=out[2]=*ySrc;
}

void decodeGreyX(th_img_plane* yuv,unsigned char* out)
{
	unsigned char *ySrc=yuv[0].data,*yLineEnd;
	for (int y=0;y<yuv[0].height;y++,ySrc+=yuv[0].stride-yuv[0].width)
		for (yLineEnd=ySrc+yuv[0].width;ySrc != yLineEnd;ySrc++,out+=4)
		{
			out[0]=out[1]=out[2]=*ySrc;
			out[3]=255;
		}
}

void decodeXGrey(th_img_plane* yuv,unsigned char* out)
{
	unsigned char *ySrc=yuv[0].data,*yLineEnd;
	for (int y=0;y<yuv[0].height;y++,ySrc+=yuv[0].stride-yuv[0].width)
		for (yLineEnd=ySrc+yuv[0].width;ySrc != yLineEnd;ySrc++,out+=4)
		{
			out[0]=255;
			out[1]=out[2]=out[3]=*ySrc;
		}
}


void decodeYUV(th_img_plane* yuv,unsigned char* out)
{
	FOREACH_PIXEL_422
	{
		out[0]=*ySrc; out[1]=*uSrc; out[2]=*vSrc;
		out+=3;
	}
	FOREACH_PIXEL_422_END
}

void decodeYUVA(th_img_plane* yuv,unsigned char* out)
{
	FOREACH_PIXEL_422
	{
		out[0]=*ySrc; out[1]=*uSrc; out[2]=*vSrc; out[3]=255;
		out+=4;
	}
	FOREACH_PIXEL_422_END
}

void decodeAYUV(th_img_plane* yuv,unsigned char* out)
{
	FOREACH_PIXEL_422
	{
		out[0]=255; out[1]=*ySrc; out[2]=*uSrc; out[3]=*vSrc;
		out+=4;
	}
	FOREACH_PIXEL_422_END
}

void (*conversion_functions[])(th_img_plane*,unsigned char* out)={0,
    decodeRGB,//TH_RGB
	0,//TH_BGR
	0,//TH_RGBA
	0,//TH_BGRA
	0,//TH_ARGB
	0,//TH_ABGR
	decodeGrey,//TH_GREY
	decodeGrey3,//TH_GREY3
	decodeGreyX,//TH_GREY3X
	decodeXGrey,//TH_XGREY3
	decodeYUV,//TH_YUV
	decodeYUVA,//TH_YUVX
	decodeAYUV,//TH_XYUV
};
// --------------------------------------------------------------
TheoraVideoFrame::TheoraVideoFrame(TheoraVideoClip* parent)
{
	mReady=mInUse=false;
	mParent=parent;
	mIteration=0;
	// number of bytes based on output mode
	int bytemap[]={0,3,3,4,4,4,4,4,3,4,4,3,4,4};
	mBuffer=new unsigned char[mParent->mWidth * mParent->mHeight * bytemap[mParent->getOutputMode()]];
}

TheoraVideoFrame::~TheoraVideoFrame()
{
	if (mBuffer) delete [] mBuffer;
}

int TheoraVideoFrame::getWidth()
{
	return mParent->mWidth;
}

int TheoraVideoFrame::getHeight()
{
	return mParent->mHeight;
}

unsigned char* TheoraVideoFrame::getBuffer()
{
	return mBuffer;
}

void TheoraVideoFrame::decode(void* yuv)
{
	conversion_functions[mParent->getOutputMode()]((th_img_plane*) yuv,mBuffer);
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
//        r = 1.164*(*ySrc - 16) + 1.596*(cv - 128);
//        b = 1.164*(*ySrc - 16)                   + 2.018*(cu - 128);
//        g = 1.164*(*ySrc - 16) - 0.813*(cv - 128) - 0.391*(cu - 128);
    double scale = 1L << 13, temp;
	
	for (int i = 0; i < 256; i++)
	{
		temp = i - 128;
		
		YTable[i]  = (unsigned int)((1.164 * scale + 0.5) * (i - 16));	//Calc Y component
		RVTable[i] = (unsigned int)((1.596 * scale + 0.5) * temp);		//Calc R component
		GUTable[i] = (unsigned int)((0.391 * scale + 0.5) * temp);		//Calc G u & v components
		GVTable[i] = (unsigned int)((0.813 * scale + 0.5) * temp);
		BUTable[i] = (unsigned int)((2.018 * scale + 0.5) * temp);		//Calc B component
	}
}
