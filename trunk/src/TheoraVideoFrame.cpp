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
#define CLIP_RGB_COLOR( rgb_color_test ) TH_MAX( TH_MIN(rgb_color_test, 255), 0 )

unsigned int YTable [256];
unsigned int BUTable[256];
unsigned int GUTable[256];
unsigned int GVTable[256];
unsigned int RVTable[256];

TheoraVideoFrame::TheoraVideoFrame(TheoraVideoClip* parent)
{
	mReady=mInUse=false;
	mParent=parent;
	mIteration=0;
	mBuffer=new unsigned char[mParent->mWidth * mParent->mHeight * 4];
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

void TheoraVideoFrame::decodeRGB(void* _yuv)
{
	th_img_plane* yuv=(th_img_plane*) _yuv;
	int t,y;
	unsigned char *ySrc=yuv[0].data, *ySrc2=yuv[0].data,*ySrcEnd,
				  *uSrc=yuv[1].data, *uSrc2=yuv[1].data,
	              *vSrc=yuv[2].data, *vSrc2=yuv[2].data;
	unsigned char *out=mBuffer;
	int cu, cy, cv, bU, gUV, rV, rgbY;

	for (y=0;y<yuv[0].height;y++)
	{
		t=0; ySrc=ySrc2; ySrcEnd=ySrc2+yuv[0].width; uSrc=uSrc2; vSrc=vSrc2;
		while (ySrc != ySrcEnd)
		{
            cy=*ySrc;
            
            // the following #ifdefs are (should be) temporary. There is something wrong with the conversion
            // tables on MacOS and I can't figure it out (yet!).
#ifndef __APPLE__
			//get corresponding lookup values
			rgbY = YTable[cy];
#endif
			if (t=!t == 1)
			{
				cu=*uSrc; cv=*vSrc;
#ifndef __APPLE__
				rV   = RVTable[cv];
				gUV  = GUTable[cu] + GVTable[cv];
				bU   = BUTable[cu];
#endif
				uSrc++; vSrc++;
			}
			//scale down - brings are values back into the 8 bits of a byte
#ifndef __APPLE__
			out[0] = CLIP_RGB_COLOR((rgbY + rV ) >> 13);
			out[1] = CLIP_RGB_COLOR((rgbY - gUV) >> 13);
			out[2] = CLIP_RGB_COLOR((rgbY + bU ) >> 13);
#else
            out[0] = CLIP_RGB_COLOR(1.164*(cy - 16) + 1.596*(cv - 128));
            out[1] = CLIP_RGB_COLOR(1.164*(cy - 16)                   + 2.018*(cu - 128));
            out[2] = CLIP_RGB_COLOR( 1.164*(cy - 16) - 0.813*(cv - 128) - 0.391*(cu - 128));
#endif
            
			out+=3;
			ySrc++;
		}
		ySrc2+=yuv[0].stride;
		if (y%2 == 1) { uSrc2+=yuv[1].stride;; vSrc2+=yuv[2].stride; }
	}
}

void TheoraVideoFrame::decodeGrey(void* _yuv)
{
	th_img_plane* yuv=(th_img_plane*) _yuv;
	unsigned char*ySrc=yuv[0].data,*ySrc2=yuv[0].data,*ySrcEnd;
	unsigned int cy;
	unsigned char* out=mBuffer;
	int y;

	for (y=0;y<yuv[0].height;y++)
	{
		ySrc=ySrc2; ySrcEnd=ySrc2+yuv[0].width;
		while (ySrc != ySrcEnd)
		{
			cy=*ySrc;
			out[0]=out[1]=out[2]=cy;
			out+=3;
			ySrc++;
		}
		ySrc2+=yuv[0].stride;
	}
}

void TheoraVideoFrame::decodeYUV(void* _yuv)
{
	th_img_plane* yuv=(th_img_plane*) _yuv;
	int t,y;
	unsigned char *ySrc=yuv[0].data, *ySrc2=yuv[0].data,*ySrcEnd,
				  *uSrc=yuv[1].data, *uSrc2=yuv[1].data,
	              *vSrc=yuv[2].data, *vSrc2=yuv[2].data;
	unsigned char *out=mBuffer;

	for (y=0;y<yuv[0].height;y++)
	{
		t=0; ySrc=ySrc2; ySrcEnd=ySrc2+yuv[0].width; uSrc=uSrc2; vSrc=vSrc2;
		while (ySrc != ySrcEnd)
		{
			out[0]=*ySrc;
			out[1]=*uSrc;
			out[2]=*vSrc;
			out+=3;
			ySrc++;
			if (t=!t == 1) { uSrc++; vSrc++; }
		}
		ySrc2+=yuv[0].stride;
		if (y%2 == 1) { uSrc2+=yuv[1].stride;; vSrc2+=yuv[2].stride; }
	}
}

void TheoraVideoFrame::decode(void* yuv)
{
	TheoraOutputMode mode=mParent->getOutputMode();
	if      (mode == TH_RGB)  decodeRGB(yuv);
	else if (mode == TH_GREY) decodeGrey(yuv);
	else if (mode == TH_YUV)  decodeYUV(yuv);
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
#ifndef __APPLE__
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
#endif
}
