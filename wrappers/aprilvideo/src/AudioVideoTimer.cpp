/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <hltypes/hlog.h>
#include <xal/AudioManager.h>
#include <xal/Player.h>
#include <xal/xal.h>

#include "AudioVideoTimer.h"
#include "VideoObject.h"

//#define _DEBUG_SYNC

namespace aprilvideo
{
	AudioVideoTimer::AudioVideoTimer(VideoObject* videoObject, xal::Player* player, float syncOffset) : VideoTimer(videoObject)
	{
		this->player = player;
		this->syncOffset = syncOffset;
		this->prevTickCount = 0ULL;
		this->prevTimePosition = -1.0f;
		this->audioPosition = 0.0f;
		this->syncDiff = 0.0f;
		this->syncDiffFactor = 0.0f;
		this->timeDiff = 0.0f;
		static hstr audiosystem = xal::manager->getName(); // XAL_AS_DISABLED audio system doesn't sync audio & video
		this->disabledAudio = (audiosystem == XAL_AS_DISABLED || !xal::manager->isEnabled());
		this->startedPlaying = false;
	}
	
	void AudioVideoTimer::update(float timeDelta)
	{
		VideoTimer::update(timeDelta);
		if (!this->disabledAudio)
		{
			bool paused = this->isPaused();
			bool playerPaused = this->player->isPaused();
			// use our own time calculation because april's could be tampered with (speedup/slowdown)
			uint64_t tickCount = htickCount();
			if (this->prevTickCount == 0)
			{
				this->prevTickCount = tickCount;
			}
			timeDelta = (tickCount - this->prevTickCount) / 1000.0f;
			if (paused)
			{
				timeDelta = 0.0f;
			}
			if (!paused && !this->startedPlaying)
			{
				this->startedPlaying = true;
				this->player->play();
			}
			else if (paused && !playerPaused && this->startedPlaying && !this->player->isFadingOut())
			{
				this->player->pause();
			}
			else if (!paused && playerPaused)
			{
				this->player->play();
			}
			else if (timeDelta > 0.1f)
			{
				timeDelta = 0.1f; // prevent long hiccups when defoucsing window
			}
			this->prevTickCount = tickCount;
			if (this->player->isPlaying())
			{
				// on some platforms, getTimePosition() isn't accurate enough, so we need to manually update our timer and
				// use the audio position for syncing
#ifdef _DEBUG_SYNC
				float timePosition = (float)(int)this->player->getTimePosition();
#else
				float timePosition = this->player->getTimePosition();
#endif
				if (timePosition != this->prevTimePosition)
				{
					if (timePosition - this->prevTimePosition > 0.1f)
					{
						this->syncDiff = timePosition - this->audioPosition;
						this->syncDiffFactor = (float) fabs(this->syncDiff);
						this->prevTimePosition = timePosition;
#ifdef _DEBUG_SYNC
						hlog::debugf(logTag, "sync diff: %.3f", this->syncDiff);
#endif
					}
					else
					{
						this->audioPosition = this->prevTimePosition = timePosition;
					}
				}
				else
				{
					this->audioPosition += timeDelta;
				}
				if (this->syncDiff != 0.0f)
				{
					float chunk = timeDelta * this->syncDiffFactor;
					if (this->syncDiff > 0)
					{
						if (this->syncDiff - chunk < 0)
						{
							chunk = this->syncDiff;
							this->syncDiff = 0;
						}
						else
						{
							this->syncDiff -= chunk;
						}
						this->audioPosition += chunk;
					}
					else
					{
						if (this->syncDiff + chunk > 0)
						{
							chunk = -this->syncDiff;
							this->syncDiff = 0;
						}
						else
						{
							this->syncDiff += chunk;
						}
						this->audioPosition -= chunk;
					}
				}
				this->time = this->audioPosition - this->syncOffset;
			}
			else if (!xal::manager->isSuspended())
			{
				this->time += timeDelta;
			}
		}
		else
		{
			this->timeDiff += timeDelta;
			this->time = this->timeDiff - this->syncOffset;
		}
	}

	void AudioVideoTimer::pause()
	{
		theoraplayer::Timer::pause();
		this->update(0.0f);
	}

};
