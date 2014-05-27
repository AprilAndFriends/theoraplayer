/************************************************************************************
 This source file is part of the Theora Video Playback Library
 For latest info, see http://libtheoraplayer.googlecode.com
 *************************************************************************************
 Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
 This program is free software; you can redistribute it and/or modify it under
 the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
 *************************************************************************************/
#ifndef APRILVIDEO_TIMER_H
#define APRILVIDEO_TIMER_H

#include <theoraplayer/TheoraTimer.h>

namespace xal
{
	class Player;
}

namespace aprilvideo
{
	class VideoObject;
	
	class VideoTimer : public TheoraTimer
	{
		VideoObject* mObject;
	public:
		VideoTimer(VideoObject* object);
		bool isPaused();
	};
}
#endif
