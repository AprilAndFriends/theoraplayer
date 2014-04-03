/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include <xal/AudioManager.h>
#include <xal/Player.h>
#include <xal/xal.h>
#include "AudioVideoTimer.h"

namespace aprilvideo
{
	AudioVideoTimer::AudioVideoTimer(xal::Player* player, float sync_offset) : TheoraTimer()
	{
		mSyncOffset = sync_offset;
		mPlayer = player;
		mT = 0;
		static hstr audiosystem = xal::mgr->getName(); // XAL_AS_DISABLED audio system doesn't sync audio & video
		mDisabledAudio = (audiosystem == XAL_AS_DISABLED);
	}

	void AudioVideoTimer::update(float time_increase)
	{
		if (!mDisabledAudio)
		{
			if (mPlayer->isPlaying())
				mTime = mPlayer->getTimePosition() - mSyncOffset;
			else
				mTime += time_increase;
		}
		else
		{
			mT += time_increase;
			mTime = mT - mSyncOffset;
		}
	}
	
	void AudioVideoTimer::pause()
	{
		if (mDisabledAudio) TheoraTimer::pause();
		else                mPlayer->pause();
	}
	
	void AudioVideoTimer::play()
	{
		if (mDisabledAudio) TheoraTimer::play();
		else                mPlayer->play();
	}
	
	bool AudioVideoTimer::isPaused()
	{
		if (mDisabledAudio) return TheoraTimer::isPaused();
		return mPlayer->isPaused();
	}
	
	void AudioVideoTimer::stop()
	{
		if (mDisabledAudio) TheoraTimer::stop();
		else                mPlayer->stop();
	}
};
