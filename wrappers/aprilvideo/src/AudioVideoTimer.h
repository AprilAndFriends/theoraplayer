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
/// Provides an audio-video timer.

#ifndef APRILVIDEO_AUDIO_VIDEO_TIMER_H
#define APRILVIDEO_AUDIO_VIDEO_TIMER_H

#include "VideoTimer.h"

namespace xal
{
	class Player;
}

namespace aprilvideo
{
	class VideoObject;
	
	class AudioVideoTimer : public VideoTimer
	{		
	public:
		AudioVideoTimer(VideoObject* videoObject, xal::Player* player, float syncOffset);

		void update(float timeDelta);
		void pause();

	private:
		xal::Player* player;
		float syncOffset;
		uint64_t prevTickCount;
		float prevTimePosition;
		float audioPosition;
		float syncDiff;
		float syncDiffFactor;
		float timeDiff;
		bool disabledAudio;
		bool startedPlaying;

	};

}
#endif
