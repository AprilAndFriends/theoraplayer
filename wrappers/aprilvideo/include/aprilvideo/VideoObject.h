/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Provides an aprilui object for video rendering.

#ifndef APRILVIDEO_VIDEO_OBJECT_H
#define APRILVIDEO_VIDEO_OBJECT_H

#include <april/aprilUtil.h>
#include <april/Image.h>
#include <aprilui/ObjectImageBox.h>
#include <gtypes/Rectangle.h>
#include <hltypes/henum.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hstring.h>

#include "aprilvideoExport.h"

namespace theoraplayer
{
	class Timer;
	class VideoClip;
}

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
	class aprilvideoExport VideoObject : public aprilui::ImageBox
	{
		APRILUI_CLONEABLE(VideoObject);
	public:
		HL_ENUM_CLASS_DECLARE(PlaybackState,
		(
			HL_ENUM_DECLARE(PlaybackState, Undefined);
			HL_ENUM_DECLARE(PlaybackState, Playing);
			HL_ENUM_DECLARE(PlaybackState, Stopped);
			HL_ENUM_DECLARE(PlaybackState, Paused);
		));

		VideoObject(chstr name);
		~VideoObject();
		inline hstr getClassName() const { return "aprilvideo.VideoObject"; }

		static aprilui::Object* createInstance(chstr name);
		
		HL_DEFINE_GET(hstr, videoClipName, VideoClipName);
		void setVideoClipName(chstr value);
		HL_DEFINE_ISSET(videoClipUseAlpha, VideoClipUseAlpha);
		HL_DEFINE_GET(int, pauseAlphaThreshold, PauseAlphaThreshold);
		void setPauseAlphaThreshold(int value);
		HL_DEFINE_IS(looping, Looping);
		void setLooping(bool value);
		HL_DEFINE_GET(float, initialPrecacheFactor, InitialPrecacheFactor);
		void setInitialPrecacheFactor(float value);
		HL_DEFINE_GET(float, initialPrecacheTimeout, InitialPrecacheTimeout);
		void setInitialPrecacheTimeout(float value);
		HL_DEFINE_GETSET(hstr, soundName, SoundName);
		HL_DEFINE_GETSET(float, audioSyncOffset, AudioSyncOffset);
		HL_DEFINE_GET(float, speed, Speed);
		void setSpeed(float value);
		float getTimePosition();
		void setTimePosition(float value);
		int getReadyFramesCount();
		int getPrecachedFramesCount();
		int getVideoClipWidth();
		int getVideoClipHeight();
		float getVideoClipDuration();
		bool hasVideoClipAlphaChannel();
		float getPrecacheFactor();
		HL_DEFINE_GET(aprilui::Texture*, currentTexture, CurrentTexture);
		HL_DEFINE_GET(harray<aprilui::Texture*>, textures, Textures);
		bool isPlaying();
		bool isStopped();
		virtual bool isPaused();
		bool isVideoClipPaused();
		bool isVideoClipCreated();
		PlaybackState getPlaybackState();
		void setPlaybackState(PlaybackState value);

		harray<aprilui::PropertyDescription> getPropertyDescriptions();

		hstr getProperty(chstr name);
		bool setProperty(chstr name, chstr value);

		void notifyEvent(chstr type, aprilui::EventArgs* args);

		void play();
		void stop();
		void pause();
		void updateFrame();
		void createVideoClip(bool waitForCache = true);

		DEPRECATED_ATTRIBUTE inline aprilui::Texture* getTexture()				{ return this->getCurrentTexture(); }
		DEPRECATED_ATTRIBUTE inline void _createClip(bool waitForCache = true)	{ this->createVideoClip(waitForCache); }
		DEPRECATED_ATTRIBUTE inline bool _isVideoPaused()						{ return this->isVideoClipPaused(); }
		DEPRECATED_ATTRIBUTE inline bool _isClipCreated()						{ return this->isVideoClipCreated(); }
		DEPRECATED_ATTRIBUTE inline int getClipWidth()							{ return this->getVideoClipWidth(); }
		DEPRECATED_ATTRIBUTE inline int getClipHeight()							{ return this->getVideoClipHeight(); }
		DEPRECATED_ATTRIBUTE inline int getNumReadyFrames()						{ return this->getReadyFramesCount(); }
		DEPRECATED_ATTRIBUTE inline int getNumPrecachedFrames()					{ return this->getPrecachedFramesCount(); }
		DEPRECATED_ATTRIBUTE inline int getAlphaThreshold()						{ return this->getPauseAlphaThreshold(); }
		DEPRECATED_ATTRIBUTE inline void setAlphaThreshold(int value)			{ this->setPauseAlphaThreshold(value); }
		DEPRECATED_ATTRIBUTE inline int getAlphaTreshold()						{ return this->getPauseAlphaThreshold(); }
		DEPRECATED_ATTRIBUTE inline void setAlphaTreshold(int value)			{ this->setPauseAlphaThreshold(value); }

	protected:
		hstr videoClipName;
		bool videoClipUseAlpha;
		int pauseAlphaThreshold;
		bool looping;
		float initialPrecacheFactor;
		float initialPrecacheTimeout;
		hstr soundName;
		float audioSyncOffset;
		april::BlendMode blendMode;
		float speed;

		theoraplayer::VideoClip* clip;
		theoraplayer::Timer* timer;
		aprilui::Texture* currentTexture;
		harray<aprilui::Texture*> textures;
		aprilui::Image* currentVideoImage;
		harray<aprilui::Image*> videoImages;
		xal::Sound* sound;
		xal::Player* audioPlayer;

		april::Image::Format _getTextureFormat();

		void _update(float timeDelta);
		void _draw();
		
		void _tryCreateVideoClip();
		void _destroyResources();
		hstr _findVideoClipResource(chstr filename);

	private:
		hstr _videoClipFilename;
		bool _doneEventTriggered;
		unsigned long _previousFrameNumber;
		bool _seeked;

		static harray<aprilui::PropertyDescription> _propertyDescriptions;

	};

}
#endif
