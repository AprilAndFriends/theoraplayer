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
		APRILUI_CLONEABLE(VideoObject);
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
		harray<aprilui::Texture*> mTextures;
		harray<aprilui::Image*> mVideoImages;
		aprilui::Texture* mTexture;
		aprilui::Image* mVideoImage;
		float mSpeed;
		unsigned long mPrevFrameNumber;
		bool mSeeked;
		int mAlphaPauseThreshold;
		unsigned char mPrevAlpha;
		float mInitialPrecacheFactor, mInitialPrecacheTimeout;
		
		float mAudioSyncOffset;
		hstr mAudioName;
		xal::Player* mAudioPlayer;
		xal::Sound* mSound;
		
		void _update(float timeDelta);
		void _draw();

		april::Image::Format _getTextureFormat();
		void destroyResources();

	public:
		VideoObject(chstr name);
		static aprilui::Object* createInstance(chstr name);
		~VideoObject();
		inline hstr getClassName() const { return "aprilvideo.VideoObject"; }
		
		hstr getFullPath();
		
		aprilui::Texture* getTexture();
		const harray<aprilui::Texture*>& getTextures();

		void play();
		void pause();
		void stop();
		bool isPlaying();
		bool isStopped();
		float getTimePosition();
		virtual bool isPaused();
		float getInitialPrecacheFactor() { return mInitialPrecacheFactor; }
		void setInitialPrecacheFactor(float value);
		void setInitialPrecacheTimeout(float value);

		void _createClip(bool waitForCache = true);
		bool _isClipCreated();
		float getPrecacheFactor();
		int getNumReadyFrames();
		int getNumPrecachedFrames();
		hstr getClipName() { return mClipName; }
		int getClipWidth();
		int getClipHeight();
		bool hasAlphaChannel();
		
		void updateFrame();
		
		void setAlphaThreshold(int threshold);
		inline int getAlphaThreshold() { return mAlphaPauseThreshold; }
		void notifyEvent(chstr type, aprilui::EventArgs* args);

		hstr getProperty(chstr name);
		bool setProperty(chstr name, chstr value);
		harray<aprilui::PropertyDescription> getPropertyDescriptions();

		DEPRECATED_ATTRIBUTE void setAlphaTreshold(int threshold) { setAlphaThreshold(threshold); }
		DEPRECATED_ATTRIBUTE inline int getAlphaTreshold() { return mAlphaPauseThreshold; }

	};
}
#endif
