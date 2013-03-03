/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _TheoraVideoFrame_h
#define _TheoraVideoFrame_h

#include "TheoraExport.h"
#include "TheoraVideoClip.h"

struct TheoraPixelTransform;
/**
	
*/
class TheoraPlayerExport TheoraVideoFrame
{
protected:
	TheoraVideoClip* mParent;
	unsigned char* mBuffer;
	unsigned long mFrameNumber;
public:
	//! global time in seconds this frame should be displayed on
	float mTimeToDisplay;
	//! whether the frame is ready for display or not
	bool mReady;
	//! indicates the frame is being used by TheoraWorkerThread instance
	bool mInUse;
	//! used to detect when the video restarted to ensure smooth playback
	int mIteration;
	
	int mBpp;

	TheoraVideoFrame(TheoraVideoClip* parent);
	virtual ~TheoraVideoFrame();

	//! internal function, do not use directly
	void _setFrameNumber(int number) { mFrameNumber = number; }
	//! returns the frame number of this frame in the theora stream
	int getFrameNumber() { return mFrameNumber; }

	void clear();

	int getWidth();
	int getStride();
	int getHeight();

	unsigned char* getBuffer();

	//! Called by TheoraVideoClip to decode a source buffer onto itself
	virtual void decode(struct TheoraPixelTransform* t);
};
#endif
