/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "TheoraVideoFrame.h"
#include "TheoraVideoClip.h"

TheoraVideoFrame::TheoraVideoFrame(TheoraVideoClip* parent)
{
	mReady = mInUse = false;
	mParent = parent;
	mIteration = 0;
	// number of bytes based on output mode
	int bytemap[]={0, 3, 4, 4, 4, 4, 3, 4, 4, 4, 4, 1, 3, 4, 4, 4, 4, 3, 4, 4, 4, 4};
	unsigned int size = mParent->getStride() * mParent->mHeight * bytemap[mParent->getOutputMode()];
	mBuffer = new unsigned char[size];
	memset(mBuffer, 255, size);
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

void TheoraVideoFrame::clear()
{
	mInUse = mReady = false;
}
