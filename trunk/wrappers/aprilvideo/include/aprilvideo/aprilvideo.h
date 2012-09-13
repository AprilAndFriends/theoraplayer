/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef APRILVIDEO_H
#define APRILVIDEO_H

#include <hltypes/hstring.h>
#include <aprilui/ObjectImageBox.h>
#include "aprilvideoExport.h"

class TheoraVideoManager;
class TheoraVideoClip;
class TheoraTimer;
namespace aprilui
{
	class Texture;
	class Image;
}

namespace xal
{
	class Sound;
	class Player;
}

namespace aprilvideo
{
	class AprilVideoExport VideoObject : public aprilui::ImageBox
	{
		bool mPrevDoneFlag;
		bool mUseAlpha;
		bool mLoop;
		hstr mClipName;
		TheoraVideoClip* mClip;
		TheoraTimer* mTimer;
		aprilui::Texture* mTexture;
		aprilui::Image* mVideoImage;
		float mSpeed;
		int mAlphaPauseTreshold;

		float mAudioSyncOffset;
		hstr mAudioName;
		xal::Player* mAudioPlayer;
		xal::Sound* mSound;
		
		void destroyResources();
	public:
		VideoObject(chstr name, grect rect);
		static aprilui::Object* createInstance(chstr name, grect rect);
		~VideoObject();
		
		void update(float k);

		void setAlphaTreshold(int treshold);
		int getAlphaTreshold() { return mAlphaPauseTreshold; }
		void notifyEvent(chstr name, void* params);
		bool setProperty(chstr name, chstr value);
		hstr getProperty(chstr name, bool* property_exists);
	};
	
	void AprilVideoExport init(int num_worker_threads = 1);
	void AprilVideoExport destroy();
}
#endif
