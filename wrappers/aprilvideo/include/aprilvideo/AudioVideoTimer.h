/************************************************************************************
 This source file is part of the Theora Video Playback Library
 For latest info, see http://libtheoraplayer.sourceforge.net/
 *************************************************************************************
 Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
 This program is free software; you can redistribute it and/or modify it under
 the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
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
	class AudioVideoTimer : public TheoraTimer
	{
		float mSyncOffset;
		xal::Player* mPlayer;
		float mT;
		bool mDisabledAudio;
	public:
		AudioVideoTimer(xal::Player* player, float sync_offset);
		void update(float time_increase);

		void pause();
		void play();
		bool isPaused();
		void stop();
	};
}
#endif
