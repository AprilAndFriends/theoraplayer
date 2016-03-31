/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#define THEORAUTIL_NOMACROS
#include <theoraplayer/TheoraException.h>
#include <theoraplayer/TheoraFrameQueue.h>
#undef exception_cls
#include <theoraplayer/TheoraPlayer.h>
#include <april/april.h>
#include <april/Platform.h>
#include <aprilui/Dataset.h>
#include <aprilui/Image.h>
#include <aprilui/Texture.h>
#include <hltypes/hrdir.h>
#include <xal/Player.h>
#include <xal/Sound.h>

#include "aprilvideo.h"
#include "DataSource.h"
#include "VideoObject.h"
#include "AudioVideoTimer.h"

//#define _TEXWRITE_BENCHMARK // uncomment this to benchmark texture upload speed

namespace aprilvideo
{
	harray<aprilui::PropertyDescription> VideoObject::_propertyDescriptions;

	extern int gNumWorkerThreads;
	extern harray<VideoObject*> gReferences;
	extern hmutex gReferenceMutex;
	extern TheoraVideoManager* gVideoManager;
	extern hstr defaultFileExtension;
	
	VideoObject::VideoObject(chstr name) : aprilui::ImageBox(name)
	{
		this->useAlpha = 0;
		this->prevDoneFlag = 0;
		this->loop = 1;
		this->speed = 1.0f;
		this->clip = NULL;
		this->videoImage = NULL;
		this->texture = NULL;
		this->timer = NULL;
		this->sound = NULL;
		this->audioPlayer = NULL;
		this->audioSyncOffset = 0;
		hmutex::ScopeLock lock(&gReferenceMutex);
		gReferences += this;
		lock.release();
		this->alphaPauseThreshold = 0;
		this->prevFrameNumber = 0;
		this->seeked = 0;
		this->prevAlpha = 255;
		this->blendMode = april::BM_DEFAULT;
		this->initialPrecacheTimeout = 0.5f;
#if defined(_ANDROID) || defined(_WINRT)
		this->initialPrecacheFactor = 0.9f; // slower devices, better to precache more
#else
		this->initialPrecacheFactor = 0.5f;
#endif
		
		if (!gVideoManager)
		{
			try
			{
				gVideoManager = new TheoraVideoManager(gNumWorkerThreads);
			}
			catch (_TheoraGenericException& e)
			{
				// pass the exception further as a hexception so the general system can understand it
				throw Exception(e.getErrorText().c_str());
			}
			std::vector<std::string> lst = gVideoManager->getSupportedDecoders();
			foreach (std::string, it, lst)
			{
				if (*it == "AVFoundation") defaultFileExtension = ".mp4";
			}
		}
	}

	VideoObject::VideoObject(const VideoObject& other) : aprilui::ImageBox(other)
	{
		this->useAlpha = other.useAlpha;
		this->prevDoneFlag = other.prevDoneFlag;
		this->loop = other.loop;
		this->speed = other.speed;
		this->clipName = other.clipName;
		this->clip = NULL;
		this->blendMode = other.blendMode;
		this->videoImage = NULL;
		this->texture = NULL;
		this->timer = NULL;
		this->sound = NULL;
		this->audioPlayer = NULL;
		this->audioSyncOffset = 0;
		hmutex::ScopeLock lock(&gReferenceMutex);
		gReferences += this;
		lock.release();
		this->alphaPauseThreshold = other.alphaPauseThreshold;
		this->prevFrameNumber = 0;
		this->seeked = 0;
		this->prevAlpha = 255;
		this->initialPrecacheFactor = other.initialPrecacheFactor;
		this->initialPrecacheTimeout = other.initialPrecacheTimeout;
	}

	VideoObject::~VideoObject()
	{
		hmutex::ScopeLock lock(&gReferenceMutex);
		gReferences.remove(this);
		lock.release();
		destroyResources();
	}
	
	bool VideoObject::isPlaying()
	{
		return (this->clip != NULL && !isPaused() && !this->clip->isDone());
	}
	
	bool VideoObject::isPaused()
	{
		if (this->clip == NULL) return true;
		if (this->alphaPauseThreshold == 0)
		{
			bool visible = isDerivedVisible();
			return !visible;
		}
		else if (this->alphaPauseThreshold < 0)
		{
			return false;
		}
		else
		{
			int alpha = getDerivedAlpha() * getVisibilityFlag();
			return alpha < this->alphaPauseThreshold;
		}
	}

	bool VideoObject::_isVideoPaused()
	{
		return (this->timer != NULL && this->timer->isPaused());
	}

	bool VideoObject::isStopped()
	{
		return (this->clip == NULL || this->clip->isDone());
	}
	
	float VideoObject::getTimePosition()
	{
		return (this->clip != NULL ? this->clip->getTimePosition() : 0.0f);
	}

	void VideoObject::setInitialPrecacheFactor(float value)
	{
		this->initialPrecacheFactor = hclamp(value, 0.0f, 1.0f);
	}
	void VideoObject::setInitialPrecacheTimeout(float value)
	{
		this->initialPrecacheTimeout = hmax(value, 0.0f);
	}

	aprilui::Object* VideoObject::createInstance(chstr name)
	{
		return new VideoObject(name);
	}
	
	void VideoObject::notifyEvent(chstr type, aprilui::EventArgs* args)
	{
		if (type == aprilui::Event::AttachedToObject)
		{
			if (this->image != (aprilui::BaseImage*)this->videoImage)
			{
				this->image = NULL;
			}
		}
		ImageBox::notifyEvent(type, args);
	}
	
	void VideoObject::destroyResources()
	{
		foreach (aprilui::Image*, it, this->videoImages)
		{
			delete *it;
		}
		this->videoImage = NULL;
		this->image = NULL;
		this->videoImages.clear();

		foreach (aprilui::Texture*, it, this->textures)
		{
			delete *it;
		}
		this->textures.clear();
		this->texture = NULL;

		if (this->clip)
		{
			gVideoManager->destroyVideoClip(this->clip);
			this->clip = NULL;
		}
		if (this->audioPlayer)
		{
			xal::manager->destroyPlayer(this->audioPlayer);
			this->audioPlayer = NULL;
		}
		if (this->sound)
		{
			xal::manager->destroySound(this->sound);
			this->sound = NULL;
		}
		
		if (this->timer)
		{
			delete this->timer;
			this->timer = NULL;
		}
	}
	
	hstr VideoObject::getFullPath()
	{
		hstr path = hrdir::joinPath(hrdir::joinPath(this->dataset->getFilePath(), "video"), this->clipName);
		if (!path.endsWith(".ogg") && !path.endsWith(".ogv") && !path.endsWith(".mp4"))
		{
			if (hresource::exists(path + defaultFileExtension))
			{
				path += defaultFileExtension;
			}
			else
				path += ".ogv";
		}
		
		return path;
	}
	
	void VideoObject::play()
	{
		if (this->clip)
		{
			this->clip->play();
		}
	}
	
	void VideoObject::pause()
	{
		if (this->clip)
		{
			this->clip->pause();
		}
	}
	
	void VideoObject::stop()
	{
		if (this->clip)
		{
			this->clip->stop();
		}
	}
	
	april::Image::Format VideoObject::_getTextureFormat()
	{
		if (this->useAlpha)
		{
			return april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGBA);
		}
		else
		{
			// android and iOS has better performance if using rgbx
			return  april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGBX);
		}
	}
	
	bool VideoObject::_isClipCreated()
	{
		return this->clip != NULL;
	}
	
	float VideoObject::getPrecacheFactor()
	{
		return this->clip == NULL ? 0 : ((float) this->clip->getNumReadyFrames()) / this->clip->getNumPrecachedFrames();
	}

	int VideoObject::getNumReadyFrames()
	{
		return this->clip == NULL ? 0 : this->clip->getNumReadyFrames();
	}

	int VideoObject::getNumPrecachedFrames()
	{
		return this->clip == NULL ? 0 : this->clip->getNumPrecachedFrames();
	}

	bool VideoObject::hasAlphaChannel()
	{
		return this->clip == NULL ? false : this->clip->hasAlphaChannel();
	}

	int VideoObject::getClipWidth()
	{
		return this->clip == NULL ? 0 : this->clip->getWidth();
	}

	int VideoObject::getClipHeight()
	{
		return this->clip == NULL ? false : this->clip->getHeight();
	}

	void VideoObject::_createClip(bool waitForCache)
	{
		hstr path = getFullPath();
		april::Image::Format textureFormat = _getTextureFormat();
		destroyResources();
		
		if (path.endsWith(".mp4"))
		{
			hstr archive = hresource::getArchive();
			if (archive != "")
			{
				path = hrdir::joinPath(archive, path);
			}
		}
		
		try
		{
			TheoraOutputMode mode = TH_RGBA;

			if (textureFormat == april::Image::FORMAT_RGBA)				mode = TH_RGBA;
			else if (textureFormat == april::Image::FORMAT_RGBX)		mode = TH_RGBX;
			else if (textureFormat == april::Image::FORMAT_BGRA)		mode = TH_BGRA;
			else if (textureFormat == april::Image::FORMAT_BGRX)		mode = TH_BGRX;
			else if (textureFormat == april::Image::FORMAT_ARGB)		mode = TH_ARGB;
			else if (textureFormat == april::Image::FORMAT_XRGB)		mode = TH_XRGB;
			else if (textureFormat == april::Image::FORMAT_ABGR)		mode = TH_ABGR;
			else if (textureFormat == april::Image::FORMAT_XBGR)		mode = TH_XBGR;
			else if (textureFormat == april::Image::FORMAT_RGB)			mode = TH_RGBX;
			else if (textureFormat == april::Image::FORMAT_BGR)			mode = TH_BGRX;
			else if (textureFormat == april::Image::FORMAT_GRAYSCALE)	mode = TH_GREY;
			int ram = april::getSystemInfo().ram;
			int precached = 16;
#if defined(_ANDROID) || defined(_WINRT) && !defined(_WINP8)
			// Android and WinRT libtheoraplayer uses an optimized libtheora which is faster, but still slower than
			// a native hardware accelerated codec. So (for now) we use a larger precache to counter it. Though, WinP8 can't handle this memory-wise.
			if (ram > 512) precached = 32;
#else
			if      (ram < 384) precached = 6;
			else if (ram < 512) precached = 8;
			else if (ram < 1024)
			{
				if (path.contains("lowres")) precached = 16;
				else precached = 8;
			}
#endif
			
			if (path.endsWith("mp4"))
			{
				try
				{
					if (april::window->getName() == "OpenKODE") // because mp4's are opened via apple's api, and that doesn't play nice with OpenKODE dir structure.
						this->clip = gVideoManager->createVideoClip(hrdir::joinPath("res", path).cStr(), mode, precached);
					else
						this->clip = gVideoManager->createVideoClip(path.cStr(), mode, precached);
				}
				catch (_TheoraGenericException& e)
				{
					// pass the exception further as a hexception so the general system can understand it
					throw Exception(e.getErrorText().c_str());
				}
			}
			else
			{
				if (!path.endsWith(".mp4") && ram > 256)
				{
					hresource r;
					r.open(path);
					unsigned long size = (unsigned long) r.size();
					TheoraDataSource* source;

					// additional performance optimization: preload file in RAM to speed up decoding, every bit counts on Android/WinRT ARM
					// but only for "reasonably" sized files
					if (size < 64 * 1024 * 1024)
					{
						hlog::write(logTag, "Preloading video file to memory: " + path);
						unsigned char* data = new unsigned char[size];
						r.readRaw(data, (int) size);
						source = new TheoraMemoryFileDataSource(data, size, path.cStr());
					}
					else
					{
						source = new AprilVideoDataSource(path);
					}
					
					this->clip = gVideoManager->createVideoClip(source, mode, precached);
					r.close();
					hlog::write(logTag, "Created video clip.");
				}
				else
				{
					this->clip = gVideoManager->createVideoClip(new AprilVideoDataSource(path), mode, precached);
				}
			}
		}
		catch (_TheoraGenericException& e)
		{
			throw Exception(e.getErrorText().c_str());
		}
		if (this->clip->getWidth() == 0) throw Exception("Failed to load video file: " + path);
		this->clip->setAutoRestart(this->loop);
		
		int tw = this->clip->getWidth();
		int th = this->clip->getHeight();
		april::RenderSystem::Caps caps = april::rendersys->getCaps();
		if (!caps.npotTexturesLimited && !caps.npotTextures)
		{
			tw = hpotCeil(tw);
			th = hpotCeil(th);
		}

		hlog::write(logTag, "Creating video textures for " + this->clipName);
		april::Texture* tex = NULL;
		for (int i = 0; i < 2; i++)
		{
			tex = april::rendersys->createTexture(tw, th, april::Color::Clear, textureFormat, april::Texture::TYPE_VOLATILE);
			this->texture = new aprilui::Texture(tex->getFilename() + "_" + hstr(i + 1), tex);

			this->videoImage = new aprilui::Image(this->texture, "video_img_" + hstr(i + 1), grect(this->clip->getSubFrameOffsetX(), this->clip->getSubFrameOffsetY(), this->clip->getSubFrameWidth(), this->clip->getSubFrameHeight()));
			this->videoImage->setBlendMode(this->blendMode);

			this->textures += this->texture;
			this->videoImages += this->videoImage;
		}

		if (waitForCache && this->initialPrecacheFactor > 0.0f)
		{
			float factor = hmax(2.0f / this->clip->getNumPrecachedFrames(), this->initialPrecacheFactor);
			float precached = (float) this->clip->getNumReadyFrames() / this->clip->getNumPrecachedFrames();
			if (precached < factor)
			{
				hlog::writef(logTag, "Waiting for cache (%.1f%% / %.1f%%): %s", precached * 100.0f, factor * 100.0f, path.cStr());
				if (factor > 0)
				{
					precached = this->clip->waitForCache(factor, this->initialPrecacheTimeout); // better to wait a while then to display an empty image
				}
				if (precached < factor)
				{
					hlog::writef(logTag, "Initial precache cached %.1f%% frames, target precache factor was %.1f%%", precached * 100.0f, factor * 100.0f);
				}
			}
		}

		if (this->audioName != "")
		{
			hstr category = "video";
			if (this->audioName.contains("/"))
			{
				harray<hstr> folders = hrdir::splitPath(this->audioName);
				hstr path_category = folders[folders.size() - 2];
				if (xal::manager->hasCategory(path_category)) category = path_category;
			}
			if (category == "video" && !xal::manager->hasCategory("video"))
			{
#if defined(_WINRT) || defined(_ANDROID)
				xal::manager->createCategory("video", xal::ON_DEMAND, xal::DISK);
#else
				if (april::getSystemInfo().ram >= 512)
				{
					xal::manager->createCategory("video", xal::STREAMED, xal::RAM);
				}
				else
				{
					xal::manager->createCategory("video", xal::STREAMED, xal::DISK);
				}
#endif
			}
			this->sound = xal::manager->createSound(hrdir::joinPath(hrdir::joinPath(this->dataset->getFilePath(), "video"), this->audioName), category);
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
		update(0.0f); // to grab the first frame.
	}
	
	void VideoObject::updateFrame()
	{
		if (this->clip == NULL && this->clipName != "")
		{
			_createClip();
		}
		if (this->clip)
		{
			TheoraVideoFrame* f = this->clip->getNextFrame();
			bool pop = true, restoringTexture = false;
			if (!this->texture->isLoaded())
			{
				restoringTexture = true;
				hlog::write(logTag, this->clipName + ": Textures unloaded, reloading");
				int i = 1;
				foreach (aprilui::Texture*, it, this->textures)
				{
					hlog::write(logTag, this->clipName + ": Reloading texture " + hstr(i));
					(*it)->load();
					i++;
				}
				if (!f)
				{
					hlog::write(logTag, this->clipName + ": Texture restored, waiting for video frame to decode to fill texture.");
					int nReady = this->clip->getNumReadyFrames();
					if (nReady == 0)
					{
						this->clip->waitForCache();
					}
					f = this->clip->getFrameQueue()->getFirstAvailableFrame();
					pop = false;
				}
				else
				{
					hlog::write(logTag, this->clipName + ": Texture restored, using current frame to fill restored texture content.");
				}
			}
			if (f)
			{
				gvec2 size;
				size.x = f->getWidth();
				size.y = f->getHeight();
				april::Image::Format textureFormat = _getTextureFormat();
				// switch textures each frame to optimize GPU pipeline
				int index = this->videoImage == this->videoImages[0] ? 1 : 0;
				this->texture = this->textures[index];

				if (restoringTexture)
				{
					if (this->textures[index]->isLoaded())
					{
						hlog::write(logTag, this->clipName + ": Verified that new texture is loaded.");
					}
					else
					{
						hlog::error(logTag, this->clipName + ": New texture is not loaded!");
					}
				}

				this->videoImage = this->videoImages[index];
				this->image = this->videoImage;
#ifdef _TEXWRITE_BENCHMARK
				long t = clock();
				int n = 256;
				char msg[1024];
				for (int i = 0; i < n; i++)
				{
					this->texture->getTexture()->write(0, 0, (int)size.x, (int)size.y, 0, 0, f->getBuffer(), (int)size.x, (int)size.y, textureFormat);
				}
				float diff = ((float) (clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
				sprintf(msg, "BENCHMARK: uploading n %dx%d video frames to texture took %.1fms (%.2fms average per frame)\n", (int)size.x, (int)size.y, diff, diff / n);
				hlog::write(logTag, msg);
#else
				this->texture->getTexture()->write(0, 0, (int)size.x, (int)size.y, 0, 0, f->getBuffer(), (int)size.x, (int)size.y, textureFormat);
#endif
				if (pop)
				{
					this->clip->popFrame();
				}
				if (this->loop)
				{
					unsigned long number = f->getFrameNumber();
					if (this->seeked) this->seeked = 0;
					else if (number < this->prevFrameNumber)
					{
#ifdef _PLAYBACK_DONE_DEBUG
						hlog::writef(logTag, "PlaybackDone(looping): %s", this->clipName.cStr());
#endif
						triggerEvent("PlaybackDone");
					}
					this->prevFrameNumber = number;
				}
				if (restoringTexture)
				{
					hlog::write(logTag, this->clipName + ": Successfully uploaded video frame to restored texture.");
				}
			}
		}
	}

	aprilui::Texture* VideoObject::getTexture()
	{
		return this->texture;
	}

	const harray<aprilui::Texture*>& VideoObject::getTextures()
	{
		return this->textures;
	}

	void VideoObject::_draw()
	{
		updateFrame();
		ImageBox::_draw();
	}
	
	void VideoObject::_update(float timeDelta)
	{
		ImageBox::_update(timeDelta);
		if (this->clip)
		{
			if (this->audioPlayer)
			{
				float pitch = this->audioPlayer->getPitch();
				float desiredPitch = this->speed;
				if (pitch != desiredPitch)
				{
					this->audioPlayer->setPitch(desiredPitch);
				}
			}

			if (!this->loop)
			{
				bool done = this->clip->isDone();
				if (done && this->audioPlayer != NULL && this->audioPlayer->isPlaying())
				{
					done = false;
				}
				if (this->prevDoneFlag == 0 && done == 1)
				{
//#ifdef _PLAYBACK_DONE_DEBUG
					hlog::writef(logTag, "PlaybackDone: %s", this->clipName.cStr());
//#endif
					triggerEvent("PlaybackDone");
				}
				this->prevDoneFlag = done;
			}
			this->clip->update(timeDelta);

			if (this->alphaPauseThreshold < 0 && !isDerivedVisible() && !isPaused())
			{
				updateFrame();
				if (isDebugModeEnabled())
				{
					hlog::write(logTag, this->clipName + ": Video object is not visible, but alpha_pause_threshold is set to -1, updating frame");
				}
			}
		}
	}
	
	void VideoObject::setAlphaThreshold(int threshold)
	{
		this->alphaPauseThreshold = hclamp(threshold, -1, 255); // -1 indicates a situation where the user wants the video playing all the time
	}
	
	bool VideoObject::setProperty(chstr name, chstr value)
	{
		if      (name == "video")
		{
			this->clipName = value;
			hstr path = getFullPath();
			if (!hresource::exists(path)) throw Exception("Unable to find video file: " + path);
		}
		else if (name == "video_alpha") this->useAlpha = value;
		else if (name == "alpha_pause_threshold") setAlphaThreshold(value);
		else if (name == "alpha_pause_treshold")
		{
			hlog::warn(logTag, "'alpha_pause_treshold=' is deprecated. Use 'alpha_pause_threshold=' instead."); // DEPRECATED
			this->setAlphaThreshold(value);
		}
		else if (name == "loop")
		{
			this->loop = value;
			if (this->clip)
			{
				this->clip->setAutoRestart(this->loop);
			}
		}
		else if (name == "initial_precache_factor")
		{
			setInitialPrecacheFactor(value);
		}
		else if (name == "initial_precache_timeout")
		{
			setInitialPrecacheTimeout(value);
		}
		else if (name == "speed")
		{
			this->speed = value;
			if (this->clip) this->clip->setPlaybackSpeed(this->speed);
		}
		else if (name == "time")
		{
			if (!this->clip && this->clipName != "") update(0); // try to create the clip if it hasn't been created already
			if (this->clip)
			{
				float time = value;
				this->seeked = 1;
				this->clip->seek(time);
			}
			else hlog::warn(logTag, "VideoObject ignoring 'time' param, this->clip is NULL");
		}
		else if (name == "audio")
		{
			this->audioName = value;
		}
		else if (name == "alpha")
		{
			aprilui::ImageBox::setProperty(name, value);
		}
		else if (name == "sync_offset")
		{
			this->audioSyncOffset = value;
		}
		else if (name == "blend_mode")
		{
			april::BlendMode mode;
			if		(value == "default")	mode = april::BM_DEFAULT;
			else if (value == "alpha")		mode = april::BM_ALPHA;
			else if (value == "add")		mode = april::BM_ADD;
			else if (value == "subtract")	mode = april::BM_SUBTRACT;
			else if (value == "overwrite")	mode = april::BM_OVERWRITE;
			else
			{
				hlog::errorf(logTag, "Unknown VideoObject blend mode: %s", name.cStr());
				return 1;
			}
			this->blendMode = mode;
			if (this->videoImage)
			{
				this->videoImage->setBlendMode(mode);
			}
		}
		else if (name == "state")
		{
			if (value == "playing")
			{
				if (this->clip && this->clip->isPaused())
				{
					this->clip->play();
				}
			}
			else if (value == "paused")
			{
				if (this->clip && !this->clip->isPaused()) this->clip->pause();
			}
			else if (value == "stopped")
			{
				if (this->clip)
				{
					this->clip->stop();
				}
			}
			else throw Exception("VideoObject: unable to set state property to '" + value + "'.");
		}
		else return aprilui::ImageBox::setProperty(name, value);
		return 1;
	}
	
	hstr VideoObject::getProperty(chstr name)
	{
		if (name == "video")						return this->clipName;
		if (name == "video_alpha")					return this->useAlpha ? "1" : "0";
		if (name == "alpha_pause_threshold")		return this->alphaPauseThreshold;
		if (name == "alpha_pause_treshold")
		{
			hlog::warn(logTag, "'alpha_pause_treshold' is deprecated. Use 'alpha_pause_threshold' instead."); // DEPRECATED
			return this->alphaPauseThreshold;
		}
		if (name == "loop")							return this->loop ? "1" : "0";
		if (name == "speed")						return this->speed;
		if (name == "initial_precache_factor")		return this->initialPrecacheFactor;
		if (name == "initial_precache_timeout")		return this->initialPrecacheTimeout;
		if (name == "time")							return this->getTimePosition();
		if (name == "videoWidth" || name == "videoHeight" || name == "duration")
		{
			if (this->clip == NULL && this->clipName != "")
			{
				_createClip();
			}
			if (name == "duration")		return this->clip ? hstr(this->clip->getDuration()) : "0";
			if (name == "videoWidth")	return this->clip ? hstr(this->clip->getWidth()) : "0";
			if (name == "videoHeight")	return this->clip ? hstr(this->clip->getHeight()) : "0";
			// should never happen
			return "0";
		}
		if (name == "audio")						return this->audioName;
		if (name == "sync_offset")					return this->audioSyncOffset;
		if (name == "blend_mode")
		{
			if (this->videoImage)
			{
				if (this->blendMode == april::BM_DEFAULT)	return "default";
				if (this->blendMode == april::BM_ALPHA)		return "alpha";
				if (this->blendMode == april::BM_ADD)		return "add";
				if (this->blendMode == april::BM_SUBTRACT)	return "subtract";
				if (this->blendMode == april::BM_OVERWRITE)	return "overwrite";
				return "unknown";
			}
			hlog::error(logTag, "Unable to get blend_mode to VideoObject, image is NULL");
			return "";
		}
		if (name == "state")
		{
			if (this->isPlaying())	return "playing";
			if (this->isPaused())	return "paused";
			if (this->isStopped())	return "stopped";
			return "unknown";
		}
		return ImageBox::getProperty(name);
	}
	
	harray<aprilui::PropertyDescription> VideoObject::getPropertyDescriptions()
	{
		if (VideoObject::_propertyDescriptions.size() == 0)
		{
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("video", aprilui::PropertyDescription::STRING);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("video_alpha", aprilui::PropertyDescription::BOOL);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("alpha_pause_threshold", aprilui::PropertyDescription::INT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("loop", aprilui::PropertyDescription::BOOL);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("initial_precache_factor", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("initial_precache_timeout", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("speed", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("time", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("duration", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("videoWidth", aprilui::PropertyDescription::INT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("videoHeight", aprilui::PropertyDescription::INT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("audio", aprilui::PropertyDescription::STRING);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("sync_offset", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("blend_mode", aprilui::PropertyDescription::STRING);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("state", aprilui::PropertyDescription::STRING);
		}
		return (aprilui::ImageBox::getPropertyDescriptions() + VideoObject::_propertyDescriptions);
	}
}
