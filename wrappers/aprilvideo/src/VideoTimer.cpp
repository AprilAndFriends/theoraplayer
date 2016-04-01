/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "VideoObject.h"
#include "VideoTimer.h"

namespace aprilvideo
{
	VideoTimer::VideoTimer(VideoObject* videoObject) : theoraplayer::Timer()
	{
		this->videoObject = videoObject;
	}

	bool VideoTimer::isPaused()
	{
		return (theoraplayer::Timer::isPaused() || this->videoObject->isPaused());
	}

};
