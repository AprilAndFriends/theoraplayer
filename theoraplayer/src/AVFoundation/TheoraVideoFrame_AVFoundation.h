/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _TheoraVideoFrame_AVFoundation_h
#define _TheoraVideoFrame_AVFoundation_h

#include "TheoraFrameQueue.h"
#include "TheoraVideoFrame.h"

class TheoraVideoFrame_AVFoundation : public TheoraVideoFrame
{
public:
	TheoraVideoFrame_AVFoundation(TheoraVideoClip* clip);
	void decode(void* src, TheoraOutputMode srcFormat);
};

class TheoraFrameQueue_AVFoundation : public TheoraFrameQueue
{
protected:
	TheoraVideoFrame* createFrameInstance(TheoraVideoClip* clip);
	public:
	TheoraFrameQueue_AVFoundation(TheoraVideoClip* parent);
};

#endif
