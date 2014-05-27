/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include "VideoTimer.h"
#include "VideoObject.h"

namespace aprilvideo
{
	VideoTimer::VideoTimer(VideoObject* object) : TheoraTimer()
	{
		mObject = object;
	}

	bool VideoTimer::isPaused()
	{
		bool objPaused = mObject->isPaused();
		return TheoraTimer::isPaused() || objPaused;
	}

};
