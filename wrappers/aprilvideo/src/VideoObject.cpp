/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <april/april.h>
#include <april/Platform.h>
#include <april/SystemDelegate.h>
#include <aprilui/Dataset.h>
#include <aprilui/Image.h>
#include <aprilui/Texture.h>
#include <hltypes/hrdir.h>
#include <hltypes/hfile.h>
#include <theoraplayer/Exception.h>
#include <theoraplayer/FrameQueue.h>
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoFrame.h>
#include <xal/Player.h>
#include <xal/Sound.h>

#include "aprilvideo.h"
#include "AudioVideoTimer.h"
#include "DataSource.h"
#include "Utility.h"
#include "VideoObject.h"

#define DEFAULT_VIDEO_PATH "video"
#define AUDIO_CATEGORY "video"
#define TEXTURES_COUNT 3 // triple buffering

//#define _TEXWRITE_BENCHMARK // uncomment this to benchmark texture upload speed

namespace aprilvideo
{
	HL_ENUM_CLASS_DEFINE(VideoObject::PlaybackState,
	(
		HL_ENUM_DEFINE_NAME(VideoObject::PlaybackState, Undefined, "undefined");
		HL_ENUM_DEFINE_NAME(VideoObject::PlaybackState, Playing, "playing");
		HL_ENUM_DEFINE_NAME(VideoObject::PlaybackState, Stopped, "stopped");
		HL_ENUM_DEFINE_NAME(VideoObject::PlaybackState, Paused, "paused");
	));

	hmap<hstr, aprilui::PropertyDescription> VideoObject::_propertyDescriptions;
	hmap<hstr, aprilui::PropertyDescription::Accessor*> VideoObject::_getters;
	hmap<hstr, aprilui::PropertyDescription::Accessor*> VideoObject::_setters;

	VideoObject::VideoObject(chstr name) : aprilui::ImageBox(name)
	{
		this->videoClipUseAlpha = false;
		this->pauseAlphaThreshold = 0;
		this->looping = true;
		this->initialPrecacheTimeout = 0.5f;
#if defined(__ANDROID__) || defined(_WINRT)
		this->initialPrecacheFactor = 0.9f; // slower devices, better to precache more
#else
		this->initialPrecacheFactor = 0.5f;
#endif
		this->audioSyncOffset = 0.0f;
		this->blendMode = april::BlendMode::Alpha;
		this->colorMode = april::ColorMode::Multiply;
		this->colorModeFactor = 1.0f;
		this->speed = 1.0f;
		this->clip = NULL;
		this->timer = NULL;
		this->currentTexture = NULL;
		this->currentVideoImage = NULL;
		this->sound = NULL;
		this->audioPlayer = NULL;
		this->_doneEventTriggered = false;
		this->_previousFrameNumber = 0UL;
		this->_seeked = false;
		hmutex::ScopeLock lock(&videoObjectsMutex);
		videoObjects += this;
		lock.release();
	}

	VideoObject::VideoObject(const VideoObject& other) : aprilui::ImageBox(other)
	{
		this->videoClipName = other.videoClipName;
		this->videoClipUseAlpha = other.videoClipUseAlpha;
		this->pauseAlphaThreshold = other.pauseAlphaThreshold;
		this->looping = other.looping;
		this->initialPrecacheFactor = other.initialPrecacheFactor;
		this->initialPrecacheTimeout = other.initialPrecacheTimeout;
		this->audioSyncOffset = 0.0f;
		this->blendMode = other.blendMode;
		this->colorMode = other.colorMode;
		this->colorModeFactor = other.colorModeFactor;
		this->speed = other.speed;
		this->clip = NULL;
		this->timer = NULL;
		this->currentTexture = NULL;
		this->currentVideoImage = NULL;
		this->sound = NULL;
		this->audioPlayer = NULL;
		this->_videoClipFilename = other._videoClipFilename;
		this->_videoClipFormatName = other._videoClipFormatName;
		this->_doneEventTriggered = other._doneEventTriggered;
		this->_previousFrameNumber = 0UL;
		this->_seeked = false;
		hmutex::ScopeLock lock(&videoObjectsMutex);
		videoObjects += this;
		lock.release();
	}

	VideoObject::~VideoObject()
	{
		hmutex::ScopeLock lock(&videoObjectsMutex);
		videoObjects -= this;
		lock.release();
		this->_destroyResources();
	}

	aprilui::Object* VideoObject::createInstance(chstr name)
	{
		return new VideoObject(name);
	}

	hmap<hstr, aprilui::PropertyDescription>& VideoObject::getPropertyDescriptions() const
	{
		if (VideoObject::_propertyDescriptions.size() == 0)
		{
			VideoObject::_propertyDescriptions = aprilui::ImageBox::getPropertyDescriptions();
			VideoObject::_propertyDescriptions["video_clip_name"] = aprilui::PropertyDescription("video_clip_name", aprilui::PropertyDescription::Type::String);
			VideoObject::_propertyDescriptions["video_clip_use_alpha"] = aprilui::PropertyDescription("video_clip_use_alpha", aprilui::PropertyDescription::Type::Bool);
			VideoObject::_propertyDescriptions["pause_alpha_threshold"] = aprilui::PropertyDescription("pause_alpha_threshold", aprilui::PropertyDescription::Type::Int);
			VideoObject::_propertyDescriptions["looping"] = aprilui::PropertyDescription("looping", aprilui::PropertyDescription::Type::Bool);
			VideoObject::_propertyDescriptions["initial_precache_factor"] = aprilui::PropertyDescription("initial_precache_factor", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions["initial_precache_timeout"] = aprilui::PropertyDescription("initial_precache_timeout", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions["sound_name"] = aprilui::PropertyDescription("sound_name", aprilui::PropertyDescription::Type::String);
			VideoObject::_propertyDescriptions["audio_sync_offset"] = aprilui::PropertyDescription("audio_sync_offset", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions["blend_mode"] = aprilui::PropertyDescription("blend_mode", aprilui::PropertyDescription::Type::Enum);
			VideoObject::_propertyDescriptions["color_mode"] = aprilui::PropertyDescription("color_mode", aprilui::PropertyDescription::Type::Enum);
			VideoObject::_propertyDescriptions["color_mode_factor"] = aprilui::PropertyDescription("color_mode_factor", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions["speed"] = aprilui::PropertyDescription("speed", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions["time_position"] = aprilui::PropertyDescription("time_position", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions["video_clip_width"] = aprilui::PropertyDescription("video_clip_width", aprilui::PropertyDescription::Type::Int);
			VideoObject::_propertyDescriptions["video_clip_height"] = aprilui::PropertyDescription("video_clip_height", aprilui::PropertyDescription::Type::Int);
			VideoObject::_propertyDescriptions["video_clip_duration"] = aprilui::PropertyDescription("video_clip_duration", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions["playback_state"] = aprilui::PropertyDescription("playback_state", aprilui::PropertyDescription::Type::String);
		}
		return VideoObject::_propertyDescriptions;
	}

	hmap<hstr, aprilui::PropertyDescription::Accessor*>& VideoObject::_getGetters() const
	{
		if (VideoObject::_getters.size() == 0)
		{
			VideoObject::_getters = aprilui::ImageBox::_getGetters();
			VideoObject::_getters["video_clip_name"] = new aprilui::PropertyDescription::Get<VideoObject, hstr>(&VideoObject::getVideoClipName);
			VideoObject::_getters["video_clip_use_alpha"] = new aprilui::PropertyDescription::Get<VideoObject, bool>(&VideoObject::isVideoClipUseAlpha);
			VideoObject::_getters["pause_alpha_threshold"] = new aprilui::PropertyDescription::Get<VideoObject, int>(&VideoObject::getPauseAlphaThreshold);
			VideoObject::_getters["looping"] = new aprilui::PropertyDescription::Get<VideoObject, bool>(&VideoObject::isLooping);
			VideoObject::_getters["initial_precache_factor"] = new aprilui::PropertyDescription::Get<VideoObject, float>(&VideoObject::getInitialPrecacheFactor);
			VideoObject::_getters["initial_precache_timeout"] = new aprilui::PropertyDescription::Get<VideoObject, float>(&VideoObject::getInitialPrecacheTimeout);
			VideoObject::_getters["sound_name"] = new aprilui::PropertyDescription::Get<VideoObject, hstr>(&VideoObject::getSoundName);
			VideoObject::_getters["audio_sync_offset"] = new aprilui::PropertyDescription::Get<VideoObject, float>(&VideoObject::getAudioSyncOffset);
			//VideoObject::_getters["blend_mode"] = new aprilui::PropertyDescription::GetEnum<VideoObject>(&VideoObject::getBlendMode);
			//VideoObject::_getters["color_mode"] = new aprilui::PropertyDescription::GetEnum<VideoObject>(&VideoObject::getColorMode);
			VideoObject::_getters["color_mode_factor"] = new aprilui::PropertyDescription::Get<VideoObject, float>(&VideoObject::getColorModeFactor);
			VideoObject::_getters["speed"] = new aprilui::PropertyDescription::Get<VideoObject, float>(&VideoObject::getSpeed);
			VideoObject::_getters["time_position"] = new aprilui::PropertyDescription::Get<VideoObject, float>(&VideoObject::getTimePosition);
			VideoObject::_getters["video_clip_width"] = new aprilui::PropertyDescription::Get<VideoObject, int>(&VideoObject::getVideoClipWidth);
			VideoObject::_getters["video_clip_height"] = new aprilui::PropertyDescription::Get<VideoObject, int>(&VideoObject::getVideoClipHeight);
			VideoObject::_getters["video_clip_duration"] = new aprilui::PropertyDescription::Get<VideoObject, float>(&VideoObject::getVideoClipDuration);
			//VideoObject::_getters["playback_state"] = new aprilui::PropertyDescription::Get<VideoObject, hstr>(&VideoObject::getPlaybackState);
		}
		return VideoObject::_getters;
	}

	hmap<hstr, aprilui::PropertyDescription::Accessor*>& VideoObject::_getSetters() const
	{
		if (VideoObject::_setters.size() == 0)
		{
			VideoObject::_setters = aprilui::ImageBox::_getSetters();
			VideoObject::_setters["video_clip_name"] = new aprilui::PropertyDescription::Set<VideoObject, hstr>(&VideoObject::setVideoClipName);
			VideoObject::_setters["video_clip_use_alpha"] = new aprilui::PropertyDescription::Set<VideoObject, bool>(&VideoObject::setVideoClipUseAlpha);
			VideoObject::_setters["pause_alpha_threshold"] = new aprilui::PropertyDescription::Set<VideoObject, int>(&VideoObject::setPauseAlphaThreshold);
			VideoObject::_setters["looping"] = new aprilui::PropertyDescription::Set<VideoObject, bool>(&VideoObject::setLooping);
			VideoObject::_setters["initial_precache_factor"] = new aprilui::PropertyDescription::Set<VideoObject, float>(&VideoObject::setInitialPrecacheFactor);
			VideoObject::_setters["initial_precache_timeout"] = new aprilui::PropertyDescription::Set<VideoObject, float>(&VideoObject::setInitialPrecacheTimeout);
			VideoObject::_setters["sound_name"] = new aprilui::PropertyDescription::Set<VideoObject, hstr>(&VideoObject::setSoundName);
			VideoObject::_setters["audio_sync_offset"] = new aprilui::PropertyDescription::Set<VideoObject, float>(&VideoObject::setAudioSyncOffset);
			//VideoObject::_setters["blend_mode"] = new aprilui::PropertyDescription::SetEnum<VideoObject>(&VideoObject::setBlendMode);
			//VideoObject::_setters["color_mode"] = new aprilui::PropertyDescription::SetEnum<VideoObject>(&VideoObject::setColorMode);
			VideoObject::_setters["color_mode_factor"] = new aprilui::PropertyDescription::Set<VideoObject, float>(&VideoObject::setColorModeFactor);
			VideoObject::_setters["speed"] = new aprilui::PropertyDescription::Set<VideoObject, float>(&VideoObject::setSpeed);
			VideoObject::_setters["time_position"] = new aprilui::PropertyDescription::Set<VideoObject, float>(&VideoObject::setTimePosition);
			//VideoObject::_setters["playback_state"] = new aprilui::PropertyDescription::Set<VideoObject, hstr>(&VideoObject::setPlaybackState);
		}
		return VideoObject::_setters;
	}

	void VideoObject::setVideoClipName(chstr value)
	{
		this->videoClipName = value;
		this->_findVideoClipResource(this->videoClipName);
		if (this->_videoClipFilename == "")
		{
			throw Exception("Unable to find video file: " + this->videoClipName);
		}
	}

	void VideoObject::setPauseAlphaThreshold(const int& value)
	{
		// TODOth - this hack should be reconsidered or refactored
		this->pauseAlphaThreshold = hclamp(value, -1, 255); // -1 indicates a situation where the user wants the video playing all the time
	}

	void VideoObject::setLooping(const bool& value)
	{
		if (this->looping != value)
		{
			this->looping = value;
			if (this->clip != NULL)
			{
				this->clip->setAutoRestart(this->looping);
			}
		}
	}

	void VideoObject::setInitialPrecacheFactor(const float& value)
	{
		this->initialPrecacheFactor = hclamp(value, 0.0f, 1.0f);
	}

	void VideoObject::setInitialPrecacheTimeout(const float& value)
	{
		this->initialPrecacheTimeout = hmax(value, 0.0f);
	}

	void VideoObject::setColorModeFactor(const float& value)
	{
		this->colorModeFactor = value;
		if (this->currentVideoImage != NULL)
		{
			this->currentVideoImage->setColorModeFactor(value);
		}
	}

	void VideoObject::setSpeed(const float& value)
	{
		if (this->speed != value)
		{
			this->speed = value;
			if (this->clip != NULL)
			{
				this->clip->setPlaybackSpeed(this->speed);
			}
		}
	}

	float VideoObject::getTimePosition() const
	{
		return (this->clip != NULL ? this->clip->getTimePosition() : 0.0f);
	}

	void VideoObject::setTimePosition(const float& value)
	{
		if (this->clip == NULL && this->videoClipName != "")
		{
			this->update(0.0f); // try to create the clip if it hasn't been created already
		}
		if (this->clip != NULL)
		{
			this->_seeked = true;
			this->clip->seek(value);
		}
		else
		{
			hlog::warn(logTag, "VideoObject::timePosition ignored, no VideoClip has been loaded yet.");
		}
	}

	int VideoObject::getReadyFramesCount() const
	{
		return (this->clip != NULL ? this->clip->getReadyFramesCount() : 0);
	}

	int VideoObject::getPrecachedFramesCount() const
	{
		return (this->clip != NULL ? this->clip->getPrecachedFramesCount() : 0);
	}

	bool VideoObject::hasVideoClipAlphaChannel() const
	{
		return (this->clip != NULL && this->clip->hasAlphaChannel());
	}

	int VideoObject::getVideoClipWidth()
	{
		this->_tryCreateVideoClip();
		return (this->clip != NULL ? this->clip->getWidth() : 0);
	}

	int VideoObject::getVideoClipHeight()
	{
		this->_tryCreateVideoClip();
		return (this->clip != NULL ? this->clip->getHeight() : 0);
	}

	float VideoObject::getVideoClipDuration()
	{
		this->_tryCreateVideoClip();
		return (this->clip != NULL ? this->clip->getDuration() : 0.0f);
	}

	float VideoObject::getPrecacheFactor() const
	{
		return (this->clip != NULL ? ((float)this->clip->getReadyFramesCount() / this->clip->getPrecachedFramesCount()) : 0.0f);
	}

	bool VideoObject::isPlaying() const
	{
		return (this->clip != NULL && !this->isPaused() && !this->clip->isDone());
	}

	bool VideoObject::isStopped() const
	{
		return (this->clip == NULL || this->clip->isDone());
	}

	bool VideoObject::isPaused() const
	{
		if (this->clip == NULL)
		{
			return true;
		}
		if (this->pauseAlphaThreshold == 0)
		{
			return (!this->isDerivedVisible());
		}
		if (this->pauseAlphaThreshold < 0)
		{
			return false;
		}
		if (!this->getVisibilityFlag())
		{
			return false;
		}
		return (this->getDerivedAlpha() < this->pauseAlphaThreshold);
	}

	bool VideoObject::isVideoClipPaused() const
	{
		return (this->timer != NULL && this->timer->isPaused());
	}

	bool VideoObject::isVideoClipCreated() const
	{
		return (this->clip != NULL);
	}

	VideoObject::PlaybackState VideoObject::getPlaybackState() const
	{
		if (this->isPlaying())
		{
			return PlaybackState::Playing;
		}
		if (this->isPaused())
		{
			return PlaybackState::Paused;
		}
		if (this->isStopped())
		{
			return PlaybackState::Stopped;
		}
		return PlaybackState::Undefined;
	}

	void VideoObject::setPlaybackState(PlaybackState value)
	{
		if (value == PlaybackState::Playing)
		{
			if (this->clip != NULL && this->clip->isPaused())
			{
				this->clip->play();
			}
			return;
		}
		if (value == PlaybackState::Paused)
		{
			if (this->clip && !this->clip->isPaused())
			{
				this->clip->pause();
			}
			return;
		}
		if (value == PlaybackState::Stopped)
		{
			if (this->clip != NULL)
			{
				this->clip->stop();
			}
			return;
		}
		throw Exception("VideoObject: Unable to set state property to '" + value.getName() + "'.");
	}

	april::Image::Format VideoObject::_getTextureFormat() const
	{
		april::Image::Format format = april::Image::Format::RGBX;
		if (this->videoClipUseAlpha)
		{
			format = april::Image::Format::RGBA;
		}
		return april::rendersys->getNativeTextureFormat(format);
	}

	void VideoObject::_draw()
	{
		this->updateFrame();
		ImageBox::_draw();
	}
	
	void VideoObject::_update(float timeDelta)
	{
		ImageBox::_update(timeDelta);
		if (this->clip != NULL)
		{
			if (this->audioPlayer != NULL)
			{
				float pitch = this->audioPlayer->getPitch();
				float desiredPitch = this->speed;
				if (pitch != desiredPitch)
				{
					this->audioPlayer->setPitch(desiredPitch);
				}
			}
			if (!this->looping)
			{
				bool done = this->clip->isDone();
				if (done && this->audioPlayer != NULL && this->audioPlayer->isPlaying())
				{
					done = false;
				}
				if (!this->_doneEventTriggered && done)
				{
					hlog::writef(logTag, "PlaybackDone: %s", this->videoClipName.cStr());
					this->triggerEvent("PlaybackDone");
				}
				this->_doneEventTriggered = done;
			}
			if (this->pauseAlphaThreshold < 0 && !this->isDerivedVisible() && !this->isPaused())
			{
				this->updateFrame();
				if (aprilvideo::isDebugModeEnabled())
				{
					hlog::write(logTag, this->videoClipName + ": Video object is not visible, but alpha_pause_threshold is set to -1, updating frame");
				}
			}
		}
	}
	
	hstr VideoObject::getProperty(chstr name)
	{
		if (name == "blend_mode")
		{
			if (this->blendMode == april::BlendMode::Add)		return "add";
			if (this->blendMode == april::BlendMode::Subtract)	return "subtract";
			if (this->blendMode == april::BlendMode::Overwrite)	return "overwrite";
			return "alpha";
		}
		if (name == "color_mode")
		{
			if (this->colorMode == april::ColorMode::Multiply)		return "multiply";
			if (this->colorMode == april::ColorMode::AlphaMap)		return "alpha_map";
			if (this->colorMode == april::ColorMode::Lerp)			return "lerp";
			if (this->colorMode == april::ColorMode::Desaturate)	return "desaturate";
			if (this->colorMode == april::ColorMode::Sepia)			return "sepia";
			return "";
		}
		if (name == "playback_state")	return this->getPlaybackState().getName();
		// DEPRECATED
		if (name == "video")
		{
			hlog::error(logTag, "'video' is deprecated. Use 'video_clip_name' instead.");
			return this->getVideoClipName();
		}
		if (name == "video_alpha")
		{
			hlog::error(logTag, "'video_alpha' is deprecated. Use 'video_clip_use_alpha' instead.");
			return this->isVideoClipUseAlpha();
		}
		if (name == "alpha_pause_treshold")
		{
			hlog::error(logTag, "'alpha_pause_treshold' is deprecated. Use 'pause_alpha_threshold' instead.");
			return this->getPauseAlphaThreshold();
		}
		if (name == "alpha_pause_threshold")
		{
			hlog::error(logTag, "'alpha_pause_treshold' is deprecated. Use 'pause_alpha_threshold' instead.");
			return this->getPauseAlphaThreshold();
		}
		if (name == "loop")
		{
			hlog::error(logTag, "'loop' is deprecated. Use 'looping' instead.");
			return this->isLooping();
		}
		if (name == "audio")
		{
			hlog::error(logTag, "'audio' is deprecated. Use 'sound_name' instead.");
			return this->getSoundName();
		}
		if (name == "sync_offset")
		{
			hlog::error(logTag, "'sync_offset' is deprecated. Use 'audio_sync_offset' instead.");
			return this->getAudioSyncOffset();
		}
		if (name == "videoWidth")
		{
			hlog::error(logTag, "'videoWidth' is deprecated. Use 'video_clip_width' instead.");
			return this->getVideoClipWidth();
		}
		if (name == "videoHeight")
		{
			hlog::error(logTag, "'videoHeight' is deprecated. Use 'video_clip_height' instead.");
			return this->getVideoClipHeight();
		}
		if (name == "duration")
		{
			hlog::error(logTag, "'duration' is deprecated. Use 'video_clip_duration' instead.");
			return this->getVideoClipDuration();
		}
		if (name == "time")
		{
			hlog::error(logTag, "'time' is deprecated. Use 'time_position' instead.");
			return this->getTimePosition();
		}
		if (name == "state")
		{
			hlog::error(logTag, "'state' is deprecated. Use 'playback_state' instead.");
			return this->getPlaybackState().getName();
		}
		return ImageBox::getProperty(name);
	}
	
	bool VideoObject::setProperty(chstr name, chstr value)
	{
		if (name == "blend_mode")
		{
			april::BlendMode mode;
			if (value == "default")
			{
				hlog::error(logTag, "'blend_mode=default' is deprecated. Use 'blend_mode=alpha' instead."); // DEPRECATED
				mode = april::BlendMode::Alpha;
			}
			else if (value == "alpha")		mode = april::BlendMode::Alpha;
			else if (value == "add")		mode = april::BlendMode::Add;
			else if (value == "subtract")	mode = april::BlendMode::Subtract;
			else if (value == "overwrite")	mode = april::BlendMode::Overwrite;
			else
			{
				hlog::errorf(logTag, "Unknown VideoObject blend mode: %s", name.cStr());
				return false;
			}
			this->blendMode = mode;
			if (this->currentVideoImage != NULL)
			{
				this->currentVideoImage->setBlendMode(mode);
			}
			return true;
		}
		if (name == "color_mode")
		{
			april::ColorMode mode;
			if (value == "multiply")		mode = april::ColorMode::Multiply;
			else if (value == "alpha_map")	mode = april::ColorMode::AlphaMap;
			else if (value == "lerp")		mode = april::ColorMode::Lerp;
			else if (value == "desaturate")	mode = april::ColorMode::Desaturate;
			else if (value == "sepia")		mode = april::ColorMode::Sepia;
			else
			{
				hlog::errorf(logTag, "Unknown VideoObject color mode: %s", name.cStr());
				return false;
			}
			this->colorMode = mode;
			if (this->currentVideoImage != NULL)
			{
				this->currentVideoImage->setColorMode(mode);
			}
			return true;
		}
		if (name == "playback_state")
		{
			this->setPlaybackState(PlaybackState::fromName(value));
			return true;
		}
		// DEPRECATED
		if (name == "video")
		{
			hlog::error(logTag, "'video=' is deprecated. Use 'video_clip_name=' instead.");
			this->setVideoClipName(value);
			return true;
		}
		if (name == "video_alpha")
		{
			hlog::error(logTag, "'video_alpha=' is deprecated. Use 'video_clip_use_alpha=' instead.");
			this->setVideoClipUseAlpha(value);
			return true;
		}
		if (name == "alpha_pause_threshold")
		{
			hlog::error(logTag, "'alpha_pause_threshold=' is deprecated. Use 'pause_alpha_threshold=' instead.");
			this->setPauseAlphaThreshold(value);
			return true;
		}
		if (name == "alpha_pause_treshold")
		{
			hlog::error(logTag, "'alpha_pause_treshold=' is deprecated. Use 'pause_alpha_threshold=' instead.");
			this->setPauseAlphaThreshold(value);
			return true;
		}
		if (name == "loop")
		{
			hlog::error(logTag, "'loop=' is deprecated. Use 'looping=' instead.");
			this->setLooping(value);
			return true;
		}
		if (name == "audio")
		{
			hlog::error(logTag, "'audio=' is deprecated. Use 'sound_name=' instead.");
			this->setSoundName(value);
			return true;
		}
		if (name == "sync_offset")
		{
			hlog::error(logTag, "'sync_offset=' is deprecated. Use 'audio_sync_offset=' instead.");
			this->setAudioSyncOffset(value);
			return true;
		}
		if (name == "time")
		{
			hlog::error(logTag, "'time=' is deprecated. Use 'time_position=' instead.");
			this->setTimePosition(value);
			return true;
		}
		if (name == "state")
		{
			hlog::error(logTag, "'state=' is deprecated. Use 'playback_state=' instead.");
			this->setPlaybackState(PlaybackState::fromName(value));
			return true;
		}
		return aprilui::ImageBox::setProperty(name, value);
	}
	
	void VideoObject::notifyEvent(chstr type, aprilui::EventArgs* args)
	{
		if (type == aprilui::Event::AttachedToObject)
		{
			if (this->image != (aprilui::BaseImage*)this->currentVideoImage)
			{
				this->image = NULL;
			}
		}
		ImageBox::notifyEvent(type, args);
	}

	void VideoObject::play()
	{
		if (this->clip != NULL)
		{
			this->clip->play();
		}
	}
	
	void VideoObject::stop()
	{
		if (this->clip != NULL)
		{
			this->clip->stop();
		}
	}

	void VideoObject::pause()
	{
		if (this->clip != NULL)
		{
			this->clip->pause();
		}
	}

	void VideoObject::updateFrame()
	{
		this->_tryCreateVideoClip();
		if (this->clip != NULL)
		{
			theoraplayer::VideoFrame* frame = this->clip->fetchNextFrame();
			if (frame != NULL)
			{
				april::Image::Format textureFormat = this->_getTextureFormat();
				// switch textures each frame to optimize GPU pipeline
				int index = (this->videoImages.indexOf(this->currentVideoImage) + 1) % this->videoImages.size();
				this->currentTexture = this->textures[index];
				this->currentVideoImage = this->videoImages[index];
				this->currentVideoImage->setBlendMode(this->blendMode);
				this->currentVideoImage->setColorMode(this->colorMode);
				this->currentVideoImage->setColorModeFactor(this->colorModeFactor);
				this->image = this->currentVideoImage;
				int frameWidth = frame->getStride();
				int frameHeight = frame->getHeight();
				if (frame->hasAlphaChannel())
				{
					frameWidth /= 2;
				}
#ifdef _TEXWRITE_BENCHMARK
				long t = clock();
				int n = 256;
				char message[1024] = { '\0' };
				for_iter (i, 0, n)
				{
					this->currentTexture->getTexture()->write(0, 0, frameWidth, frameHeight, 0, 0, frame->getBuffer(), frameWidth, frameHeight, textureFormat);
				}
				float diff = ((float)(clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
				sprintf(message, "BENCHMARK: uploading n %dx%d video frames to texture took %.1fms (%.2fms average per frame)\n", frameWidth, frameHeight, diff, diff / n);
				hlog::write(logTag, message);
#else
				this->currentTexture->getTexture()->write(0, 0, frameWidth, frameHeight, 0, 0, frame->getBuffer(), frameWidth, frameHeight, textureFormat);
#endif
				this->clip->popFrame();
				if (this->looping)
				{
					unsigned long number = frame->getFrameNumber();
					if (this->_seeked)
					{
						this->_seeked = false;
					}
					else if (number < this->_previousFrameNumber)
					{
#ifdef _PLAYBACK_DONE_DEBUG
						hlog::writef(logTag, "PlaybackDone(looping): %s", this->videoClipName.cStr());
#endif
						this->triggerEvent("PlaybackDone");
					}
					this->_previousFrameNumber = number;
				}
			}
		}
	}

	void VideoObject::createVideoClip(bool waitForCache)
	{
		april::Image::Format textureFormat = this->_getTextureFormat();
		april::SystemInfo sysInfo = april::getSystemInfo();
		this->_destroyResources();
		hstr path = this->_videoClipFilename;
		if (path.endsWith(".mp4"))
		{
			hstr archive = hresource::getMountedArchives().tryGet("", "");
			if (archive != "")
			{
				path = hrdir::joinPath(archive, path);
			}
		}
		bool usePotStride = false;
		april::RenderSystem::Caps caps = april::rendersys->getCaps();
		if (!caps.npotTexturesLimited && !caps.npotTextures)
		{
			usePotStride = true;
		}
		theoraplayer::DataSource* source = NULL;
		try
		{
			theoraplayer::OutputMode mode = theoraplayer::FORMAT_RGBA;
			if		(textureFormat == april::Image::Format::RGBA)		mode = theoraplayer::FORMAT_RGBA;
			else if (textureFormat == april::Image::Format::RGBX)		mode = theoraplayer::FORMAT_RGBX;
			else if (textureFormat == april::Image::Format::BGRA)		mode = theoraplayer::FORMAT_BGRA;
			else if (textureFormat == april::Image::Format::BGRX)		mode = theoraplayer::FORMAT_BGRX;
			else if (textureFormat == april::Image::Format::ARGB)		mode = theoraplayer::FORMAT_ARGB;
			else if (textureFormat == april::Image::Format::XRGB)		mode = theoraplayer::FORMAT_XRGB;
			else if (textureFormat == april::Image::Format::ABGR)		mode = theoraplayer::FORMAT_ABGR;
			else if (textureFormat == april::Image::Format::XBGR)		mode = theoraplayer::FORMAT_XBGR;
			else if (textureFormat == april::Image::Format::RGB)		mode = theoraplayer::FORMAT_RGBX;
			else if (textureFormat == april::Image::Format::BGR)		mode = theoraplayer::FORMAT_BGRX;
			else if (textureFormat == april::Image::Format::Greyscale)	mode = theoraplayer::FORMAT_GREY;
			int ram = sysInfo.ram;
			int precached = 16;
#if defined(_WINRT) && !defined(_WINP8)
			// WinRT libtheoraplayer uses an optimized libtheora which is faster, but still slower than a native hardware accelerated codec.
			// So (for now) we use a larger precache to counter it. Though, WinP8 can't handle this memory-wise.
			if (ram > 512)
			{
				precached = 32;
			}
#elif defined(__ANDROID__)
			// Android requires a bit more sensitive precaching than other platforms
			precached = 8;
			if (ram < 512)
			{
				precached = 4;
			}
			else if (ram < 1024)
			{
				precached = 6;
			}
			else if (ram < 1536)
			{
				precached = (path.contains("lowres") ? 16 : 8);
			}
#else // iOS and others
			if (ram < 384)
			{
				precached = 4;
			}
			else if (ram < 512)
			{
				precached = 6;
			}
			else if (ram < 1024)
			{
				precached = (path.contains("lowres") ? 16 : 8);
			}
#endif
			if (path.endsWith(".mp4"))
			{
				hstr mp4Path = path;
				if (hresource::getMountedArchives().size() > 0)
				{
					// Mp4 files can't be read from the zip file, paste them in the res/ folder
					harray<hstr> pathElements = hrdir::splitPath(this->_videoClipFilename);
					hstr filename = pathElements.last();
					mp4Path = filename;
				}
				else if (april::window->getName() == "OpenKODE") // because mp4's are opened via apple's api, and that doesn't play nice with OpenKODE dir structure.
				{
					mp4Path = hrdir::joinPath("res", path);
				}
				this->clip = theoraplayer::manager->createVideoClip(mp4Path.cStr(), mode, precached, usePotStride);
			}
			// additional performance optimization: preload file in RAM to speed up decoding, every CPU cycle counts on certain platforms
			// but only for "reasonably" sized files
			else
			{
				if (!path.endsWith(".mp4") && ram > 512)
				{
					hresource file;
					file.open(path);
					int size = (int)file.size();
					if (size < aprilvideo::getPreloadToRamSizeLimit() * 1024 * 1024)
					{
						hlog::write(logTag, "Preloading video file to memory: " + path);
						unsigned char* data = new unsigned char[size];
						file.readRaw(data, size);
						file.close();
						theoraplayer::MemoryDataSource* memoryDataSource = new theoraplayer::MemoryDataSource(data, size, this->_videoClipFormatName.cStr(), path.cStr());
						source = memoryDataSource;
						this->clip = theoraplayer::manager->createVideoClip(source, mode, precached, usePotStride);
					}
					else
					{
						file.close();
					}
				}
				if (source == NULL)
				{
					source = new DataSource(this->_videoClipFormatName, path);
					this->clip = theoraplayer::manager->createVideoClip(source, mode, precached, usePotStride);
				}
			}
			hlog::write(logTag, "Created video clip.");
		}
		catch (theoraplayer::_Exception& e)
		{
			if (source != NULL)
			{
				delete source;
			}
			// pass the exception further as a hexception so the general system can understand it
			throw Exception(hstr(e.getMessage().c_str()));
		}
		if (this->clip == NULL || this->clip->getWidth() == 0)
		{
			throw Exception("Failed to load video file: " + path);
		}
		this->clip->setAutoRestart(this->looping);
		int tw = this->clip->getWidth();
		int th = this->clip->getHeight();
		if (usePotStride)
		{
			tw = hpotCeil(tw);
			th = hpotCeil(th);
		}
#ifdef _IOS
		if (sysInfo.architectureBits == 32 && tw * th >= 384 * 384)
		{
			hlog::write(logTag, "Low end device detected, queueing memory warning before creating a new video instance.");
			april::window->queueLowMemoryWarning();
		}
#endif
		hlog::write(logTag, "Creating video textures for " + this->videoClipName);
		april::Texture* aprilTexture = NULL;
		hstr filename;
		for_iter (i, 0, TEXTURES_COUNT)
		{
			aprilTexture = april::rendersys->createTexture(tw, th, april::Color::Clear, textureFormat);
			filename = aprilTexture->getFilename() + "_" + hstr(i + 1);
			this->currentTexture = new aprilui::Texture(filename, filename, aprilTexture);
			this->currentVideoImage = new aprilui::Image(this->currentTexture, "aprilvideo_video_clip_image_" + hstr(i + 1),
				grectf((float)this->clip->getSubFrameX(), (float)this->clip->getSubFrameY(), (float)this->clip->getSubFrameWidth(), (float)this->clip->getSubFrameHeight()));
			this->currentVideoImage->setBlendMode(this->blendMode);
			this->currentVideoImage->setColorMode(this->colorMode);
			this->currentVideoImage->setColorModeFactor(this->colorModeFactor);
			this->textures += this->currentTexture;
			this->videoImages += this->currentVideoImage;
		}
		if (waitForCache && this->initialPrecacheFactor > 0.0f)
		{
			float factor = hmax(2.0f / this->clip->getPrecachedFramesCount(), this->initialPrecacheFactor);
			float precached = (float)this->clip->getReadyFramesCount() / this->clip->getPrecachedFramesCount();
			if (precached < factor)
			{
				hlog::writef(logTag, "Waiting for cache (%.1f%% / %.1f%%): %s", precached * 100.0f, factor * 100.0f, path.cStr());
				if (factor > 0.0f)
				{
					precached = this->clip->waitForCache(factor, this->initialPrecacheTimeout); // better to wait a while then to display an empty image
				}
				if (precached < factor)
				{
					hlog::writef(logTag, "Initial precache cached %.1f%% frames, target precache factor was %.1f%%", precached * 100.0f, factor * 100.0f);
				}
			}
		}
		if (this->soundName != "")
		{
			hstr category = AUDIO_CATEGORY;
			if (this->soundName.contains("/"))
			{
				harray<hstr> folders = hrdir::splitPath(this->soundName);
				hstr pathCategory = folders[folders.size() - 2];
				if (xal::manager->hasCategory(pathCategory))
				{
					category = pathCategory;
				}
			}
			if (category == AUDIO_CATEGORY && !xal::manager->hasCategory(AUDIO_CATEGORY))
			{
#if defined(_WINRT) || defined(__ANDROID__)
				xal::manager->createCategory(AUDIO_CATEGORY, xal::BufferMode::OnDemand, xal::SourceMode::Disk);
#else
				if (sysInfo.ram >= 512)
				{
					xal::manager->createCategory(AUDIO_CATEGORY, xal::BufferMode::Streamed, xal::SourceMode::Ram);
				}
				else
				{
					xal::manager->createCategory(AUDIO_CATEGORY, xal::BufferMode::Streamed, xal::SourceMode::Disk);
				}
#endif
			}
			// maybe sound was already created somewhere
			if (xal::manager->hasSound(this->soundName))
			{
				this->sound = xal::manager->getSound(this->soundName);
			}
			else
			{
				hstr audioFilename = xal::manager->findAudioFile(hrdir::joinPath(hrdir::joinPath(this->dataset->getFilePath(), AUDIO_CATEGORY), this->soundName, true));
				if (audioFilename != "")
				{
					this->sound = xal::manager->createSound(audioFilename, category);
				}
			}
			if (this->sound != NULL)
			{
				this->audioPlayer = xal::manager->createPlayer(this->sound->getName());
				this->timer = new AudioVideoTimer(this, this->audioPlayer, this->audioSyncOffset);
			}
		}
		if (this->timer == NULL)
		{
			this->timer = new VideoTimer(this);
		}
		this->clip->setTimer(this->timer);
		this->clip->setPlaybackSpeed(this->speed);
		this->update(0.0f); // to grab the first frame.
	}
	
	void VideoObject::_tryCreateVideoClip()
	{
		if (this->clip == NULL && this->videoClipName != "")
		{
			this->createVideoClip();
		}
	}

	void VideoObject::_destroyResources()
	{
		foreach (aprilui::Image*, it, this->videoImages)
		{
			delete *it;
		}
		this->videoImages.clear();
		this->currentVideoImage = NULL;
		this->image = NULL;
		foreach (aprilui::Texture*, it, this->textures)
		{
			delete *it;
		}
		this->textures.clear();
		this->currentTexture = NULL;
		if (this->clip != NULL)
		{
			theoraplayer::manager->destroyVideoClip(this->clip);
			this->clip = NULL;
		}
		if (this->audioPlayer != NULL)
		{
			xal::manager->destroyPlayer(this->audioPlayer);
			this->audioPlayer = NULL;
		}
		if (this->sound != NULL)
		{
			xal::manager->destroySound(this->sound);
			this->sound = NULL;
		}
		if (this->timer != NULL)
		{
			delete this->timer;
			this->timer = NULL;
		}
	}

	void VideoObject::_findVideoClipResource(chstr filename)
	{
		hstr path = hrdir::normalize(hrdir::joinPath(hrdir::joinPath(this->dataset->getFilePath(), DEFAULT_VIDEO_PATH), filename));
		std::vector<theoraplayer::VideoClip::Format> theoraplayerFormats = theoraplayer::getVideoClipFormats();
		harray<theoraplayer::VideoClip::Format> formats(&theoraplayerFormats[0], (int)theoraplayerFormats.size());
		hstr result;
		foreach (theoraplayer::VideoClip::Format, it, formats)
		{
			result = path;
			if (!path.endsWith((*it).extension.c_str()))
			{
				result = path + (*it).extension.c_str();
			}
			if (hresource::exists(result))
			{
				this->_videoClipFormatName = (*it).name.c_str();
				this->_videoClipFilename = result;
				return;
			}
		}
	}

}
