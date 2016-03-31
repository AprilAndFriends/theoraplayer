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
		this->prevTickCount = 0;
		this->syncOffset = sync_offset;
		this->prevTimePosition = -1;
		this->audioPosition = 0;
		this->player = player;
		this->syncDiff = this->syncDiffFactor = 0;
		this->t = 0;
		static hstr audiosystem = xal::manager->getName(); // XAL_AS_DISABLED audio system doesn't sync audio & video
		this->disabledAudio = (audiosystem == XAL_AS_DISABLED || !xal::manager->isEnabled());
		this->startedPlaying = 0;
	}
	
	void AudioVideoTimer::update(float timeDelta)
	{
		VideoTimer::update(timeDelta);
		if (!this->disabledAudio)
		{
			bool paused = isPaused(), playerPaused = this->player->isPaused();
			// use our own time calculation because april's could be tampered with (speedup/slowdown)
			uint64_t tickCount = htickCount();
			if (this->prevTickCount == 0)
			{
				this->prevTickCount = tickCount;
			}
			timeDelta = (tickCount - this->prevTickCount) / 1000.0f;
			if (paused)
			{
				timeDelta = 0;
			}
			if (!paused && !this->startedPlaying)
			{
				this->startedPlaying = 1;
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
#if defined(_DEBUG) && 0 // debug testing
				float timePosition = (int) this->player->getTimePosition();
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
#if defined(_DEBUG) && 0 // debug testing
						hlog::writef(logTag + "_DEBUG", "sync diff: %.3f", this->syncDiff);
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
				if (this->syncDiff != 0)
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
			this->t += timeDelta;
			this->time = this->t - this->syncOffset;
		}
	}

	void AudioVideoTimer::pause()
	{
		TheoraTimer::pause();
		this->update(0.0f);
		/*
		if (!this->disabledAudio)
		{
			bool paused = isPaused(), playerPaused = this->player->isPaused();
			// use our own time calculation because april's could be tampered with (speedup/slowdown)
			uint64_t tickCount = htickCount();
			if (this->prevTickCount == 0)
			{
				this->prevTickCount = tickCount;
			}
			timeDelta = (tickCount - this->prevTickCount) / 1000.0f;
			if (paused) timeDelta = 0;
			if (!paused && !this->startedPlaying)
			{
				this->startedPlaying = 1;
				this->player->play();
			}
			else if (paused && !playerPaused && this->startedPlaying && !this->player->isFadingOut())
			{
				this->player->pause();
			}
		}
		*/
	}

};
