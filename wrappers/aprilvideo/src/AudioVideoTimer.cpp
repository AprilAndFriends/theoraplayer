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
#include <hltypes/hlog.h>

namespace aprilvideo
{
	AudioVideoTimer::AudioVideoTimer(xal::Player* player, float sync_offset) : TheoraTimer()
	{
		mSyncOffset = sync_offset;
		mPrevTimePosition = -1;
		mAudioPosition = 0;
		mPlayer = player;
		mSyncApproximated = false;
		mSyncDiff = mSyncDiffFactor = 0;
		mT = 0;
		static hstr audiosystem = xal::mgr->getName(); // XAL_AS_DISABLED audio system doesn't sync audio & video
		mDisabledAudio = (audiosystem == XAL_AS_DISABLED);
	}

	void AudioVideoTimer::update(float time_increase)
	{
		if (!mDisabledAudio)
		{
			if (mPlayer->isPlaying())
			{
				// on some platforms, getTimePosition() isn't accurate enough, so we need to manually update our timer and
				// use the audio position for syncing
				float timePosition = mPlayer->getTimePosition();
				if (timePosition != mPrevTimePosition)
				{
					if (mSyncApproximated)
					{
						mSyncApproximated = false;
						mSyncDiff = timePosition - mAudioPosition;
						mSyncDiffFactor = (float) fabs(mSyncDiff);
						mPrevTimePosition = timePosition;
						hlog::writef("aprilvideo_DEBUG", "sync diff: %.3f", mSyncDiff);
					}
					else
					{
						mAudioPosition = mPrevTimePosition = timePosition;
					}
				}
				else
				{
					mSyncApproximated = true;
					mAudioPosition += time_increase;
				}
				if (mSyncDiff != 0)
				{
					float chunk = time_increase * mSyncDiffFactor;
					
					if (mSyncDiff > 0)
					{
						if (mSyncDiff - chunk < 0)
						{
							chunk = mSyncDiff;
							mSyncDiff = 0;
						}
						else
						{
							mSyncDiff -= chunk;
						}
						mAudioPosition += chunk;
					}
					else
					{
						if (mSyncDiff + chunk > 0)
						{
							chunk = -mSyncDiff;
							mSyncDiff = 0;
						}
						else
						{
							mSyncDiff += chunk;
						}
						mAudioPosition -= chunk;
					}
				}
				mTime = mAudioPosition - mSyncOffset;
			}
			else
			{
				mTime += time_increase;
			}
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
