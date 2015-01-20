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
		uint64_t mPrevTickCount;
		float mSyncOffset, mPrevTimePosition, mAudioPosition;
		float mSyncDiff, mSyncDiffFactor;
		xal::Player* mPlayer;
		float mT;
		bool mDisabledAudio;
		bool mStartedPlaying;
	public:
		AudioVideoTimer(VideoObject* object, xal::Player* player, float sync_offset);
		void update(float timeDelta);
	};
}
#endif
