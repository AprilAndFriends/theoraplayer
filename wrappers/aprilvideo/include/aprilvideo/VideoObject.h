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
	public:
		VideoObject(chstr name);
		static aprilui::Object* createInstance(chstr name);
		~VideoObject();
		inline hstr getClassName() const { return "aprilvideo.VideoObject"; }
		
		hstr getFullPath();
		
		aprilui::Texture* getTexture();
		const harray<aprilui::Texture*>& getTextures();
		float getTimePosition();
		float getInitialPrecacheFactor() { return this->initialPrecacheFactor; }
		float getPrecacheFactor();
		int getNumReadyFrames();
		int getNumPrecachedFrames();
		hstr getClipName() { return this->clipName; }
		int getClipWidth();
		int getClipHeight();
		hstr getProperty(chstr name);
		harray<aprilui::PropertyDescription> getPropertyDescriptions();
		
		inline int getAlphaThreshold() { return this->alphaPauseThreshold; }

		DEPRECATED_ATTRIBUTE inline int getAlphaTreshold() { return this->alphaPauseThreshold; }

		void setInitialPrecacheFactor(float value);
		void setInitialPrecacheTimeout(float value);
		void setAlphaThreshold(int threshold);
		bool setProperty(chstr name, chstr value);
		DEPRECATED_ATTRIBUTE void setAlphaTreshold(int threshold) { setAlphaThreshold(threshold); }

		void play();
		void pause();
		void stop();
		void _createClip(bool waitForCache = true);
		void updateFrame();

		void notifyEvent(chstr type, aprilui::EventArgs* args);
			
		bool isPlaying();
		bool isStopped();
		virtual bool isPaused();
		/// @note This method checks the timer rather than alphaThreshold like isPaused()
		bool _isVideoPaused();		
		bool hasAlphaChannel();
		bool _isClipCreated();

	protected:
		bool prevDoneFlag;
		bool useAlpha;
		bool loop;
		hstr clipName;
		TheoraVideoClip* clip;
		TheoraTimer* timer;
		april::BlendMode blendMode;
		harray<aprilui::Texture*> textures;
		harray<aprilui::Image*> videoImages;
		aprilui::Texture* texture;
		aprilui::Image* videoImage;
		float speed;
		unsigned long prevFrameNumber;
		bool seeked;
		int alphaPauseThreshold;
		unsigned char prevAlpha;
		float initialPrecacheFactor, initialPrecacheTimeout;

		float audioSyncOffset;
		hstr audioName;
		xal::Player* audioPlayer;
		xal::Sound* sound;

		april::Image::Format _getTextureFormat();

		void _update(float timeDelta);
		void _draw();
		
		void destroyResources();
	private:
		static harray<aprilui::PropertyDescription> _propertyDescriptions;
	};
}
#endif
