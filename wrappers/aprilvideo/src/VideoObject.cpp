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

	harray<aprilui::PropertyDescription> VideoObject::_propertyDescriptions;

	VideoObject::VideoObject(chstr name) : aprilui::ImageBox(name)
	{
		this->videoClipUseAlpha = false;
		this->pauseAlphaThreshold = 0;
		this->looping = true;
		this->initialPrecacheTimeout = 0.5f;
#if defined(_ANDROID) || defined(_WINRT)
		this->initialPrecacheFactor = 0.9f; // slower devices, better to precache more
#else
		this->initialPrecacheFactor = 0.5f;
#endif
		this->audioSyncOffset = 0.0f;
		this->blendMode = april::BlendMode::Alpha;
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

	void VideoObject::setVideoClipName(chstr value)
	{
		this->videoClipName = value;
		this->_findVideoClipResource(this->videoClipName);
		if (this->_videoClipFilename == "")
		{
			throw Exception("Unable to find video file: " + this->videoClipName);
		}
	}

	void VideoObject::setPauseAlphaThreshold(int value)
	{
		// TODOth - this hack should be reconsidered or refactored
		this->pauseAlphaThreshold = hclamp(value, -1, 255); // -1 indicates a situation where the user wants the video playing all the time
	}

	void VideoObject::setLooping(bool value)
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

	void VideoObject::setInitialPrecacheFactor(float value)
	{
		this->initialPrecacheFactor = hclamp(value, 0.0f, 1.0f);
	}

	void VideoObject::setInitialPrecacheTimeout(float value)
	{
		this->initialPrecacheTimeout = hmax(value, 0.0f);
	}

	void VideoObject::setSpeed(float value)
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

	void VideoObject::setTimePosition(float value)
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

	harray<aprilui::PropertyDescription> VideoObject::getPropertyDescriptions() const
	{
		if (VideoObject::_propertyDescriptions.size() == 0)
		{
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("video_clip_name", aprilui::PropertyDescription::Type::String);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("video_clip_use_alpha", aprilui::PropertyDescription::Type::Bool);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("pause_alpha_threshold", aprilui::PropertyDescription::Type::Int);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("looping", aprilui::PropertyDescription::Type::Bool);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("initial_precache_factor", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("initial_precache_timeout", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("sound_name", aprilui::PropertyDescription::Type::String);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("audio_sync_offset", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("blend_mode", aprilui::PropertyDescription::Type::String);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("speed", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("time_position", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("video_clip_width", aprilui::PropertyDescription::Type::Int);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("video_clip_height", aprilui::PropertyDescription::Type::Int);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("video_clip_duration", aprilui::PropertyDescription::Type::Float);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("playback_state", aprilui::PropertyDescription::Type::String);
		}
		return (aprilui::ImageBox::getPropertyDescriptions() + VideoObject::_propertyDescriptions);
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
		if (name == "video_clip_name")				return this->getVideoClipName();
		if (name == "video_clip_use_alpha")			return this->isVideoClipUseAlpha();
		if (name == "pause_alpha_threshold")		return this->getPauseAlphaThreshold();
		if (name == "looping")						return this->isLooping();
		if (name == "initial_precache_factor")		return this->getInitialPrecacheFactor();
		if (name == "initial_precache_timeout")		return this->getInitialPrecacheTimeout();
		if (name == "sound_name")					return this->getSoundName();
		if (name == "audio_sync_offset")			return this->getAudioSyncOffset();
		if (name == "blend_mode")
		{
			if (this->blendMode == april::BlendMode::Add)		return "add";
			if (this->blendMode == april::BlendMode::Subtract)	return "subtract";
			if (this->blendMode == april::BlendMode::Overwrite)	return "overwrite";
			return "default";
		}
		if (name == "speed")						return this->getSpeed();
		if (name == "time_position")				return this->getTimePosition();
		if (name == "video_clip_width")				return this->getVideoClipWidth();
		if (name == "video_clip_height")			return this->getVideoClipHeight();
		if (name == "video_clip_duration")			return this->getVideoClipDuration();
		if (name == "playback_state")				return this->getPlaybackState().getName();
		// DEPRECATED
		if (name == "video")
		{
			hlog::warn(logTag, "'video' is deprecated. Use 'video_clip_name' instead.");
			return this->getVideoClipName();
		}
		if (name == "video_alpha")
		{
			hlog::warn(logTag, "'video_alpha' is deprecated. Use 'video_clip_use_alpha' instead.");
			return this->isVideoClipUseAlpha();
		}
		if (name == "alpha_pause_treshold")
		{
			hlog::warn(logTag, "'alpha_pause_treshold' is deprecated. Use 'pause_alpha_threshold' instead.");
			return this->getPauseAlphaThreshold();
		}
		if (name == "alpha_pause_threshold")
		{
			hlog::warn(logTag, "'alpha_pause_treshold' is deprecated. Use 'pause_alpha_threshold' instead.");
			return this->getPauseAlphaThreshold();
		}
		if (name == "loop")
		{
			hlog::warn(logTag, "'loop' is deprecated. Use 'looping' instead.");
			return this->isLooping();
		}
		if (name == "audio")
		{
			hlog::warn(logTag, "'audio' is deprecated. Use 'sound_name' instead.");
			return this->getSoundName();
		}
		if (name == "sync_offset")
		{
			hlog::warn(logTag, "'sync_offset' is deprecated. Use 'audio_sync_offset' instead.");
			return this->getAudioSyncOffset();
		}
		if (name == "videoWidth")
		{
			hlog::warn(logTag, "'videoWidth' is deprecated. Use 'video_clip_width' instead.");
			return this->getVideoClipWidth();
		}
		if (name == "videoHeight")
		{
			hlog::warn(logTag, "'videoHeight' is deprecated. Use 'video_clip_height' instead.");
			return this->getVideoClipHeight();
		}
		if (name == "duration")
		{
			hlog::warn(logTag, "'duration' is deprecated. Use 'video_clip_duration' instead.");
			return this->getVideoClipDuration();
		}
		if (name == "time")
		{
			hlog::warn(logTag, "'time' is deprecated. Use 'time_position' instead.");
			return this->getTimePosition();
		}
		if (name == "state")
		{
			hlog::warn(logTag, "'state' is deprecated. Use 'playback_state' instead.");
			return this->getPlaybackState().getName();
		}
		return ImageBox::getProperty(name);
	}
	
	bool VideoObject::setProperty(chstr name, chstr value)
	{
		if		(name == "video_clip_name")				this->setVideoClipName(value);
		else if (name == "video_clip_use_alpha")		this->setVideoClipUseAlpha(value);
		else if (name == "pause_alpha_threshold")		this->setPauseAlphaThreshold(value);
		else if (name == "looping")						this->setLooping(value);
		else if (name == "initial_precache_factor")		this->setInitialPrecacheFactor(value);
		else if (name == "initial_precache_timeout")	this->setInitialPrecacheTimeout(value);
		else if (name == "sound_name")					this->setSoundName(value);
		else if (name == "audio_sync_offset")			this->setAudioSyncOffset(value);
		else if (name == "blend_mode")
		{
			april::BlendMode mode;
			if (value == "default")
			{
				hlog::warn(logTag, "'blend_mode=default' is deprecated. Use 'blend_mode=alpha' instead."); // DEPRECATED
				mode = april::BlendMode::Alpha;
			}
			else if (value == "alpha")		mode = april::BlendMode::Alpha;
			else if (value == "add")		mode = april::BlendMode::Add;
			else if (value == "subtract")	mode = april::BlendMode::Subtract;
			else if (value == "overwrite")	mode = april::BlendMode::Overwrite;
			else
			{
				hlog::errorf(logTag, "Unknown VideoObject blend mode: %s", name.cStr());
				return true;
			}
			this->blendMode = mode;
			if (this->currentVideoImage != NULL)
			{
				this->currentVideoImage->setBlendMode(mode);
			}
		}
		else if (name == "speed")						this->setSpeed(value);
		else if (name == "time_position")				this->setTimePosition(value);
		else if (name == "playback_state")				this->setPlaybackState(PlaybackState::fromName(value));
		// DEPRECATED
		else if (name == "video")
		{
			hlog::warn(logTag, "'video=' is deprecated. Use 'video_clip_name=' instead.");
			this->setVideoClipName(value);
		}
		else if (name == "video_alpha")
		{
			hlog::warn(logTag, "'video_alpha=' is deprecated. Use 'video_clip_use_alpha=' instead.");
			this->setVideoClipUseAlpha(value);
		}
		else if (name == "alpha_pause_threshold")
		{
			hlog::warn(logTag, "'alpha_pause_threshold=' is deprecated. Use 'pause_alpha_threshold=' instead.");
			this->setPauseAlphaThreshold(value);
		}
		else if (name == "alpha_pause_treshold")
		{
			hlog::warn(logTag, "'alpha_pause_treshold=' is deprecated. Use 'pause_alpha_threshold=' instead.");
			this->setPauseAlphaThreshold(value);
		}
		else if (name == "loop")
		{
			hlog::warn(logTag, "'loop=' is deprecated. Use 'looping=' instead.");
			this->setLooping(value);
		}
		else if (name == "audio")
		{
			hlog::warn(logTag, "'audio=' is deprecated. Use 'sound_name=' instead.");
			this->setSoundName(value);
		}
		else if (name == "sync_offset")
		{
			hlog::warn(logTag, "'sync_offset=' is deprecated. Use 'audio_sync_offset=' instead.");
			this->setAudioSyncOffset(value);
		}
		else if (name == "time")
		{
			hlog::warn(logTag, "'time=' is deprecated. Use 'time_position=' instead.");
			this->setTimePosition(value);
		}
		else if (name == "state")
		{
			hlog::warn(logTag, "'state=' is deprecated. Use 'playback_state=' instead.");
			this->setPlaybackState(PlaybackState::fromName(value));
		}
		else return aprilui::ImageBox::setProperty(name, value);
		return true;
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
			bool pop = true;
			bool restoringTexture = false;
			if (!this->currentTexture->isLoaded())
			{
				restoringTexture = true;
				hlog::write(logTag, this->videoClipName + ": Textures unloaded, reloading");
				int i = 1;
				foreach (aprilui::Texture*, it, this->textures)
				{
					hlog::write(logTag, this->videoClipName + ": Reloading texture " + hstr(i));
					(*it)->load();
					++i;
				}
				if (frame == NULL)
				{
					hlog::write(logTag, this->videoClipName + ": Texture restored, waiting for video frame to decode to fill texture.");
					if (this->clip->getReadyFramesCount() == 0)
					{
						this->clip->waitForCache();
					}
					frame = this->clip->getFrameQueue()->getFirstAvailableFrame();
					pop = false;
				}
				else
				{
					hlog::write(logTag, this->videoClipName + ": Texture restored, using current frame to fill restored texture content.");
				}
			}
			if (frame != NULL)
			{
				int frameWidth = frame->getStride();
				int frameHeight = frame->getHeight();
				if (frame->hasAlphaChannel())
				{
					frameWidth /= 2;
				}
				april::Image::Format textureFormat = this->_getTextureFormat();
				// switch textures each frame to optimize GPU pipeline
				int index = (this->videoImages.indexOf(this->currentVideoImage) + 1) % this->videoImages.size();
				this->currentTexture = this->textures[index];
				this->currentVideoImage = this->videoImages[index];
				this->currentVideoImage->setBlendMode(this->blendMode);
				if (restoringTexture)
				{
					if (this->textures[index]->isLoaded())
					{
						hlog::write(logTag, this->videoClipName + ": Verified that new texture is loaded.");
					}
					else
					{
						hlog::error(logTag, this->videoClipName + ": New texture is not loaded!");
					}
				}
				this->image = this->currentVideoImage;
#ifdef _TEXWRITE_BENCHMARK
				long t = clock();
				int n = 256;
				char message[1024] = { '\0' };
				for (int i = 0; i < n; ++i)
				{
					this->currentTexture->getTexture()->write(0, 0, frameWidth, frameHeight, 0, 0, frame->getBuffer(), frameWidth, frameHeight, textureFormat);
				}
				float diff = ((float)(clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
				sprintf(message, "BENCHMARK: uploading n %dx%d video frames to texture took %.1fms (%.2fms average per frame)\n", frameWidth, frameHeight, diff, diff / n);
				hlog::write(logTag, message);
#else
				this->currentTexture->getTexture()->write(0, 0, frameWidth, frameHeight, 0, 0, frame->getBuffer(), frameWidth, frameHeight, textureFormat);
#endif
				if (pop)
				{
					this->clip->popFrame();
				}
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
				if (restoringTexture)
				{
					hlog::write(logTag, this->videoClipName + ": Successfully uploaded video frame to restored texture.");
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
#elif defined(_ANDROID)
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
			
#ifdef _IOS
			if (sysInfo.architectureBits == 32)
			{
				hlog::write(logTag, "Low end device detected, sending memory warning before creating a new video instance");
				april::SystemDelegate* delegate = april::window->getSystemDelegate();
				if (delegate != NULL)
				{
					delegate->onLowMemoryWarning();
				}
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
					if (size < getPreloadToRamSizeLimit() * 1024 * 1024)
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
		hlog::write(logTag, "Creating video textures for " + this->videoClipName);
		april::Texture* aprilTexture = NULL;
		for_iter (i, 0, TEXTURES_COUNT)
		{
			aprilTexture = april::rendersys->createTexture(tw, th, april::Color::Clear, textureFormat, april::Texture::Type::Volatile);
			hstr filename = aprilTexture->getFilename() + "_" + hstr(i + 1);
			this->currentTexture = new aprilui::Texture(filename, filename, aprilTexture);
			this->currentVideoImage = new aprilui::Image(this->currentTexture, "aprilvideo_video_clip_image_" + hstr(i + 1),
				grect((float)this->clip->getSubFrameX(), (float)this->clip->getSubFrameY(), (float)this->clip->getSubFrameWidth(), (float)this->clip->getSubFrameHeight()));
			this->currentVideoImage->setBlendMode(this->blendMode);
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
#if defined(_WINRT) || defined(_ANDROID)
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
