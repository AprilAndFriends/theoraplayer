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
		HL_ENUM_CLASS_PREFIX_DECLARE(aprilvideoExport, PlaybackState,
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
		
		hmap<hstr, aprilui::PropertyDescription>& getPropertyDescriptions() const;

		HL_DEFINE_GET(hstr, videoClipName, VideoClipName);
		void setVideoClipName(chstr value);
		HL_DEFINE_ISSET(videoClipUseAlpha, VideoClipUseAlpha);
		HL_DEFINE_GET(int, pauseAlphaThreshold, PauseAlphaThreshold);
		void setPauseAlphaThreshold(const int& value);
		HL_DEFINE_IS(looping, Looping);
		void setLooping(const bool& value);
		HL_DEFINE_GET(float, initialPrecacheFactor, InitialPrecacheFactor);
		void setInitialPrecacheFactor(const float& value);
		HL_DEFINE_GET(float, initialPrecacheTimeout, InitialPrecacheTimeout);
		void setInitialPrecacheTimeout(const float& value);
		HL_DEFINE_GETSET(hstr, soundName, SoundName);
		HL_DEFINE_GETSET(float, audioSyncOffset, AudioSyncOffset);
		HL_DEFINE_GET(float, speed, Speed);
		virtual void setSpeed(const float& value);
		float getTimePosition() const;
		void setTimePosition(const float& value);
		int getReadyFramesCount() const;
		int getPrecachedFramesCount() const;
		int getVideoClipWidth();
		int getVideoClipHeight();
		float getVideoClipDuration();
		bool hasVideoClipAlphaChannel() const;
		float getPrecacheFactor() const;
		HL_DEFINE_GET(aprilui::Texture*, currentTexture, CurrentTexture);
		HL_DEFINE_GET(harray<aprilui::Texture*>, textures, Textures);
		bool isPlaying() const;
		bool isStopped() const;
		virtual bool isPaused() const;
		bool isVideoClipPaused() const;
		bool isVideoClipCreated() const;
		PlaybackState getPlaybackState() const;
		void setPlaybackState(PlaybackState value);

		hstr getProperty(chstr name);
		bool setProperty(chstr name, chstr value);

		void notifyEvent(chstr type, aprilui::EventArgs* args);

		void play();
		void stop();
		void pause();
		void updateFrame();
		void createVideoClip(bool waitForCache = true);

		HL_DEPRECATED("Deprecated API. Use getCurrentTexture() instead.")
			inline aprilui::Texture* getTexture() const			{ return this->getCurrentTexture(); }
		HL_DEPRECATED("Deprecated API. Use createVideoClip() instead.")
			inline void _createClip(bool waitForCache = true)	{ this->createVideoClip(waitForCache); }
		HL_DEPRECATED("Deprecated API. Use isVideoClipPaused() instead.")
			inline bool _isVideoPaused() const					{ return this->isVideoClipPaused(); }
		HL_DEPRECATED("Deprecated API. Use isVideoClipCreated() instead.")
			inline bool _isClipCreated() const					{ return this->isVideoClipCreated(); }
		HL_DEPRECATED("Deprecated API. Use getVideoClipWidth() instead.")
			inline int getClipWidth()							{ return this->getVideoClipWidth(); }
		HL_DEPRECATED("Deprecated API. Use getVideoClipHeight() instead.")
			inline int getClipHeight()							{ return this->getVideoClipHeight(); }
		HL_DEPRECATED("Deprecated API. Use getReadyFramesCount() instead.")
			inline int getNumReadyFrames() const				{ return this->getReadyFramesCount(); }
		HL_DEPRECATED("Deprecated API. Use getPrecachedFramesCount() instead.")
			inline int getNumPrecachedFrames() const			{ return this->getPrecachedFramesCount(); }
		HL_DEPRECATED("Deprecated API. Use getPauseAlphaThreshold() instead.")
			inline int getAlphaThreshold() const				{ return this->getPauseAlphaThreshold(); }
		HL_DEPRECATED("Deprecated API. Use setPauseAlphaThreshold() instead.")
			inline void setAlphaThreshold(int value)			{ this->setPauseAlphaThreshold(value); }
		HL_DEPRECATED("Deprecated API. Use getPauseAlphaThreshold() instead.")
			inline int getAlphaTreshold() const					{ return this->getPauseAlphaThreshold(); }
		HL_DEPRECATED("Deprecated API. Use setPauseAlphaThreshold() instead.")
			inline void setAlphaTreshold(int value)				{ this->setPauseAlphaThreshold(value); }

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

		hmap<hstr, aprilui::PropertyDescription::Accessor*>& _getGetters() const;
		hmap<hstr, aprilui::PropertyDescription::Accessor*>& _getSetters() const;

		april::Image::Format _getTextureFormat() const;

		void _update(float timeDelta);
		void _draw();
		
		void _tryCreateVideoClip();
		void _destroyResources();
		void _findVideoClipResource(chstr filename);

	private:
		hstr _videoClipFilename;
		hstr _videoClipFormatName;
		bool _doneEventTriggered;
		unsigned long _previousFrameNumber;
		bool _seeked;

		static hmap<hstr, aprilui::PropertyDescription> _propertyDescriptions;
		static hmap<hstr, aprilui::PropertyDescription::Accessor*> _getters;
		static hmap<hstr, aprilui::PropertyDescription::Accessor*> _setters;

	};

}
#endif
