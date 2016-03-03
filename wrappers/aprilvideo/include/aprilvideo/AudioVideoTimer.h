/************************************************************************************
 This source file is part of the Theora Video Playback Library
 For latest info, see http://libtheoraplayer.googlecode.com
 *************************************************************************************
 Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
 This program is free software; you can redistribute it and/or modify it under
 the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
 *************************************************************************************/
#ifndef APRILVIDEO_AVTIMER_H
#define APRILVIDEO_AVTIMER_H

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
		AudioVideoTimer(VideoObject* object, xal::Player* player, float sync_offset);

		void update(float timeDelta);
		void pause();
	private:
		uint64_t prevTickCount;
		float syncOffset, prevTimePosition, audioPosition;
		float syncDiff, syncDiffFactor;
		xal::Player* player;
		float t;
		bool disabledAudio;
		bool startedPlaying;
	};
}
#endif
