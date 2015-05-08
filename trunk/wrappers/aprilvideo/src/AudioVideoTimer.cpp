/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include <xal/AudioManager.h>
#include <xal/Player.h>
#include <xal/xal.h>
#include "AudioVideoTimer.h"
#include <hltypes/hlog.h>
#include "VideoObject.h"

namespace aprilvideo
{
	AudioVideoTimer::AudioVideoTimer(VideoObject* object, xal::Player* player, float sync_offset) : VideoTimer(object)
	{
		mPrevTickCount = 0;
		mSyncOffset = sync_offset;
		mPrevTimePosition = -1;
		mAudioPosition = 0;
		mPlayer = player;
		mSyncDiff = mSyncDiffFactor = 0;
		mT = 0;
		static hstr audiosystem = xal::manager->getName(); // XAL_AS_DISABLED audio system doesn't sync audio & video
		mDisabledAudio = (audiosystem == XAL_AS_DISABLED || !xal::manager->isEnabled());
		mStartedPlaying = 0;
	}
	
	void AudioVideoTimer::update(float timeDelta)
	{
		VideoTimer::update(timeDelta);
		if (!mDisabledAudio)
		{
			bool paused = isPaused(), playerPaused = mPlayer->isPaused();
			// use our own time calculation because april's could be tampered with (speedup/slowdown)
			uint64_t tickCount = htickCount();
			if (mPrevTickCount == 0)
			{
				mPrevTickCount = tickCount;
			}
			timeDelta = (tickCount - mPrevTickCount) / 1000.0f;
			if (paused) timeDelta = 0;
			if (!paused && !mStartedPlaying)
			{
				mStartedPlaying = 1;
				mPlayer->play();
			}
			else if (paused && !playerPaused && mStartedPlaying && !mPlayer->isFadingOut())
			{
				mPlayer->pause();
			}
			else if (!paused && playerPaused)
			{
				mPlayer->play();
			}
			
			else if (timeDelta > 0.1f) timeDelta = 0.1f; // prevent long hiccups when defoucsing window

			mPrevTickCount = tickCount;
			if (mPlayer->isPlaying())
			{
				// on some platforms, getTimePosition() isn't accurate enough, so we need to manually update our timer and
				// use the audio position for syncing
#if defined(_DEBUG) && 0 // debug testing
				float timePosition = (int) mPlayer->getTimePosition();
#else
				float timePosition = mPlayer->getTimePosition();
#endif
				if (timePosition != mPrevTimePosition)
				{
					if (timePosition - mPrevTimePosition > 0.1f)
					{
						mSyncDiff = timePosition - mAudioPosition;
						mSyncDiffFactor = (float) fabs(mSyncDiff);
						mPrevTimePosition = timePosition;
#if defined(_DEBUG) && 0 // debug testing
						hlog::writef(logTag + "_DEBUG", "sync diff: %.3f", mSyncDiff);
#endif
					}
					else
					{
						mAudioPosition = mPrevTimePosition = timePosition;
					}
				}
				else
				{
					mAudioPosition += timeDelta;
				}
				if (mSyncDiff != 0)
				{
					float chunk = timeDelta * mSyncDiffFactor;
					
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
			else if (!xal::manager->isSuspended())
			{
				mTime += timeDelta;
			}
		}
		else
		{
			mT += timeDelta;
			mTime = mT - mSyncOffset;
		}
	}
};
