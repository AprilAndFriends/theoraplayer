/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines a custom Timer object.

#ifndef APRILVIDEO_VIDEO_TIMER_H
#define APRILVIDEO_VIDEO_TIMER_H

#include <theoraplayer/Timer.h>

namespace xal
{
	class Player;
}

namespace aprilvideo
{
	class VideoObject;
	
	class VideoTimer : public theoraplayer::Timer
	{		
	public:
		VideoTimer(VideoObject* object);

		bool isPaused();

	protected:
		VideoObject* videoObject;

	};

}
#endif
