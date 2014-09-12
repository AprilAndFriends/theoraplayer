/************************************************************************************
 This source file is part of the Theora Video Playback Library
 For latest info, see http://libtheoraplayer.googlecode.com
 *************************************************************************************
 Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
 This program is free software; you can redistribute it and/or modify it under
 the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
 *************************************************************************************/
#ifndef APRILVIDEO_OBJECT_H
#define APRILVIDEO_OBJECT_H

#include <hltypes/hstring.h>
#include <gtypes/Rectangle.h>
#include <april/aprilUtil.h>
#include <april/Image.h>
#include <aprilui/ObjectImageBox.h>

#include "aprilVideoExport.h"

class TheoraVideoClip;
class TheoraTimer;

namespace aprilui
{
	class Image;
	class Texture;
}

namespace xal
{
	class Player;
	class Sound;
}

namespace aprilvideo
{
	class aprilVideoExport VideoObject : public aprilui::ImageBox
	{
	private:
		static harray<aprilui::PropertyDescription> _propertyDescriptions;

	protected:

		bool mPrevDoneFlag;
		bool mUseAlpha;
		bool mLoop;
		hstr mClipName;
		TheoraVideoClip* mClip;
		TheoraTimer* mTimer;
		april::BlendMode mBlendMode;
		aprilui::Texture* mTexture;
		aprilui::Image* mVideoImage;
		float mSpeed;
		unsigned long mPrevFrameNumber;
		bool mSeeked;
		int mAlphaPauseTreshold;
		unsigned char mPrevAlpha;
		
		float mAudioSyncOffset;
		hstr mAudioName;
		xal::Player* mAudioPlayer;
		xal::Sound* mSound;
		
		april::Image::Format _getTextureFormat();
		void destroyResources();

	public:
		VideoObject(chstr name, grect rect);
		static aprilui::Object* createInstance(chstr name, grect rect);
		~VideoObject();
		hstr getClassName() const { return "VideoObject"; }
		
		hstr getFullPath();
		
		void play();
		void pause();
		void stop();
		bool isPlaying();
		bool isStopped();
		float getTimePosition();
		virtual bool isPaused();
		
		void _createClip(bool waitForCache = true);
		bool _isClipCreated();
		float getPrecacheFactor();
		hstr getClipName() { return mClipName; }
		
		void update(float timeDelta);
		void updateFrame();
		void OnDraw();
		
		void setAlphaTreshold(int treshold);
		inline int getAlphaTreshold() { return mAlphaPauseTreshold; }
		void notifyEvent(chstr type, aprilui::EventArgs* args);

		hstr getProperty(chstr name);
		bool setProperty(chstr name, chstr value);
		harray<aprilui::PropertyDescription> getPropertyDescriptions();

	};
}
#endif
