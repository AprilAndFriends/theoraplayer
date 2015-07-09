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
		mUseAlpha = 0;
		mPrevDoneFlag = 0;
		mLoop = 1;
		mSpeed = 1.0f;
		mClip = NULL;
		mVideoImage = NULL;
		mTexture = NULL;
		mTimer = NULL;
		mSound = NULL;
		mAudioPlayer = NULL;
		mAudioSyncOffset = 0;
		hmutex::ScopeLock lock(&gReferenceMutex);
		gReferences += this;
		lock.release();
		mAlphaPauseThreshold = 0;
		mPrevFrameNumber = 0;
		mSeeked = 0;
		mPrevAlpha = 255;
		mBlendMode = april::BM_DEFAULT;
		mInitialPrecacheTimeout = 0.5f;
#if defined(_ANDROID) || defined(_WINRT)
		mInitialPrecacheFactor = 0.9f; // slower devices, better to precache more
#else
		mInitialPrecacheFactor = 0.5f;
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
		mUseAlpha = other.mUseAlpha;
		mPrevDoneFlag = other.mPrevDoneFlag;
		mLoop = other.mLoop;
		mSpeed = other.mSpeed;
		mClipName = other.mClipName;
		mClip = NULL;
		mBlendMode = other.mBlendMode;
		mVideoImage = NULL;
		mTexture = NULL;
		mTimer = NULL;
		mSound = NULL;
		mAudioPlayer = NULL;
		mAudioSyncOffset = 0;
		hmutex::ScopeLock lock(&gReferenceMutex);
		gReferences += this;
		lock.release();
		mAlphaPauseThreshold = other.mAlphaPauseThreshold;
		mPrevFrameNumber = 0;
		mSeeked = 0;
		mPrevAlpha = 255;
		mInitialPrecacheFactor = other.mInitialPrecacheFactor;
		mInitialPrecacheTimeout = other.mInitialPrecacheTimeout;
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
		return (mClip != NULL && !isPaused() && !mClip->isDone());
	}
	
	bool VideoObject::isPaused()
	{
		if  (mClip == NULL) return true;
		if (mAlphaPauseThreshold == 0)
		{
			bool visible = isDerivedVisible();
			return !visible;
		}
		else if (mAlphaPauseThreshold < 0)
		{
			return false;
		}
		else
		{
			int alpha = getDerivedAlpha() * getVisibilityFlag();
			return alpha < mAlphaPauseThreshold;
		}
	}

	bool VideoObject::isStopped()
	{
		return (mClip == NULL || mClip->isDone());
	}
	
	float VideoObject::getTimePosition()
	{
		return (mClip != NULL ? mClip->getTimePosition() : 0.0f);
	}

	void VideoObject::setInitialPrecacheFactor(float value)
	{
		mInitialPrecacheFactor = hclamp(value, 0.0f, 1.0f);
	}
	void VideoObject::setInitialPrecacheTimeout(float value)
	{
		mInitialPrecacheTimeout = hmax(value, 0.0f);
	}

	aprilui::Object* VideoObject::createInstance(chstr name)
	{
		return new VideoObject(name);
	}
	
	void VideoObject::notifyEvent(chstr type, aprilui::EventArgs* args)
	{
		if (type == aprilui::Event::AttachedToObject)
		{
			if (this->image != (aprilui::BaseImage*)mVideoImage)
			{
				this->image = NULL;
			}
		}
		ImageBox::notifyEvent(type, args);
	}
	
	void VideoObject::destroyResources()
	{
		foreach (aprilui::Image*, it, mVideoImages)
		{
			delete *it;
		}
		mVideoImage = NULL;
		this->image = NULL;
		mVideoImages.clear();

		foreach (aprilui::Texture*, it, mTextures)
		{
			delete *it;
		}
		mTextures.clear();
		mTexture = NULL;

		if (mClip)
		{
			gVideoManager->destroyVideoClip(mClip);
			mClip = NULL;
		}
		if (mAudioPlayer)
		{
			xal::manager->destroyPlayer(mAudioPlayer);
			mAudioPlayer = NULL;
		}
		if (mSound)
		{
			xal::manager->destroySound(mSound);
			mSound = NULL;
		}
		
		if (mTimer)
		{
			delete mTimer;
			mTimer = NULL;
		}
	}
	
	hstr VideoObject::getFullPath()
	{
		hstr path = hrdir::joinPath(hrdir::joinPath(this->dataset->getFilePath(), "video"), mClipName);
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
		if (mClip)
		{
			mClip->play();
		}
	}
	
	void VideoObject::pause()
	{
		if (mClip)
		{
			mClip->pause();
		}
	}
	
	void VideoObject::stop()
	{
		if (mClip)
		{
			mClip->stop();
		}
	}
	
	april::Image::Format VideoObject::_getTextureFormat()
	{
		if (mUseAlpha)
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
		return mClip != NULL;
	}
	
	float VideoObject::getPrecacheFactor()
	{
		return mClip == NULL ? 0 : ((float) mClip->getNumReadyFrames()) / mClip->getNumPrecachedFrames();
	}

	int VideoObject::getNumReadyFrames()
	{
		return mClip == NULL ? 0 : mClip->getNumReadyFrames();
	}

	int VideoObject::getNumPrecachedFrames()
	{
		return mClip == NULL ? 0 : mClip->getNumPrecachedFrames();
	}

	bool VideoObject::hasAlphaChannel()
	{
		return mClip == NULL ? false : mClip->hasAlphaChannel();
	}

	int VideoObject::getClipWidth()
	{
		return mClip == NULL ? 0 : mClip->getWidth();
	}

	int VideoObject::getClipHeight()
	{
		return mClip == NULL ? false : mClip->getHeight();
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
						mClip = gVideoManager->createVideoClip(hrdir::joinPath("res", path).cStr(), mode, precached);
					else
						mClip = gVideoManager->createVideoClip(path.cStr(), mode, precached);
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
					
					mClip = gVideoManager->createVideoClip(source, mode, precached);
					r.close();
					hlog::write(logTag, "Created video clip.");
				}
				else
				{
					mClip = gVideoManager->createVideoClip(new AprilVideoDataSource(path), mode, precached);
				}
			}
		}
		catch (_TheoraGenericException& e)
		{
			throw Exception(e.getErrorText().c_str());
		}
		if (mClip->getWidth() == 0) throw Exception("Failed to load video file: " + path);
		mClip->setAutoRestart(mLoop);
		
		int tw = mClip->getWidth();
		int th = mClip->getHeight();
		april::RenderSystem::Caps caps = april::rendersys->getCaps();
		if (!caps.npotTexturesLimited && !caps.npotTextures)
		{
			tw = hpotceil(tw);
			th = hpotceil(th);
		}

		hlog::write(logTag, "Creating video textures for " + mClipName);
		april::Texture* tex;
		for (int i = 0; i < 2; i++)
		{
			tex = april::rendersys->createTexture(tw, th, april::Color::Clear, textureFormat, april::Texture::TYPE_VOLATILE);
			tex->setAddressMode(april::Texture::ADDRESS_CLAMP);
			mTexture = new aprilui::Texture(tex->getFilename() + "_" + hstr(i + 1), tex);

			mVideoImage = new aprilui::Image(mTexture, "video_img_" + hstr(i + 1), grect(mClip->getSubFrameOffsetX(), mClip->getSubFrameOffsetY(), mClip->getSubFrameWidth(), mClip->getSubFrameHeight()));
			mVideoImage->setBlendMode(mBlendMode);

			mTextures += mTexture;
			mVideoImages += mVideoImage;
		}

		if (waitForCache && mInitialPrecacheFactor > 0.0f)
		{
			float factor = hmax(2.0f / mClip->getNumPrecachedFrames(), mInitialPrecacheFactor);
			float precached = (float) mClip->getNumReadyFrames() / mClip->getNumPrecachedFrames();
			if (precached < factor)
			{
				hlog::writef(logTag, "Waiting for cache (%.1f%% / %.1f%%): %s", precached * 100.0f, factor * 100.0f, path.cStr());
				if (factor > 0)
				{
					precached = mClip->waitForCache(factor, mInitialPrecacheTimeout); // better to wait a while then to display an empty image
				}
				if (precached < factor)
				{
					hlog::writef(logTag, "Initial precache cached %.1f%% frames, target precache factor was %.1f%%", precached * 100.0f, factor * 100.0f);
				}
			}
		}

		if (mAudioName != "")
		{
			hstr category = "video";
			if (mAudioName.contains("/"))
			{
				harray<hstr> folders = hrdir::splitPath(mAudioName);
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
			mSound = xal::manager->createSound(hrdir::joinPath(hrdir::joinPath(this->dataset->getFilePath(), "video"), mAudioName), category);
			if (mSound != NULL)
			{
				mAudioPlayer = xal::manager->createPlayer(mSound->getName());
				mTimer = new AudioVideoTimer(this, mAudioPlayer, mAudioSyncOffset);
			}
		}
		if (mTimer == NULL)
		{
			mTimer = new VideoTimer(this);
		}
		mClip->setTimer(mTimer);
		mClip->setPlaybackSpeed(mSpeed);
		update(0); // to grab the first frame.
	}
	
	void VideoObject::updateFrame()
	{
		if (mClip == NULL && mClipName != "")
		{
			_createClip();
		}
		if (mClip)
		{
			TheoraVideoFrame* f = mClip->getNextFrame();
			bool pop = true, restoringTexture = false;
			if (!mTexture->isLoaded())
			{
				restoringTexture = true;
				hlog::write(logTag, this->mClipName + ": Textures unloaded, reloading");
				int i = 1;
				foreach (aprilui::Texture*, it, mTextures)
				{
					hlog::write(logTag, this->mClipName + ": Reloading texture " + hstr(i));
					(*it)->load();
					i++;
				}
				if (!f)
				{
					hlog::write(logTag, this->mClipName + ": Texture restored, waiting for video frame to decode to fill texture.");
					int nReady = mClip->getNumReadyFrames();
					if (nReady == 0)
					{
						mClip->waitForCache();
					}
					f = mClip->getFrameQueue()->getFirstAvailableFrame();
					pop = false;
				}
				else
				{
					hlog::write(logTag, this->mClipName + ": Texture restored, using current frame to fill restored texture content.");
				}
			}
			if (f)
			{
				gvec2 size;
				size.x = f->getWidth();
				size.y = f->getHeight();
				april::Image::Format textureFormat = _getTextureFormat();
				// switch textures each frame to optimize GPU pipeline
				int index = mVideoImage == mVideoImages[0] ? 1 : 0;
				mTexture = mTextures[index];

				if (restoringTexture)
				{
					if (mTextures[index]->isLoaded())
					{
						hlog::write(logTag, this->mClipName + ": Verified that new texture is loaded.");
					}
					else
					{
						hlog::error(logTag, this->mClipName + ": New texture is not loaded!");
					}
				}

				mVideoImage = mVideoImages[index];
				this->image = mVideoImage;
#ifdef _TEXWRITE_BENCHMARK
				long t = clock();
				int n = 256;
				char msg[1024];
				for (int i = 0; i < n; i++)
				{
					mTexture->getTexture()->write(0, 0, (int)size.x, (int)size.y, 0, 0, f->getBuffer(), (int)size.x, (int)size.y, textureFormat);
				}
				float diff = ((float) (clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
				sprintf(msg, "BENCHMARK: uploading n %dx%d video frames to texture took %.1fms (%.2fms average per frame)\n", (int)size.x, (int)size.y, diff, diff / n);
				hlog::write(logTag, msg);
#else
				mTexture->getTexture()->write(0, 0, (int)size.x, (int)size.y, 0, 0, f->getBuffer(), (int)size.x, (int)size.y, textureFormat);
#endif
				if (pop)
				{
					mClip->popFrame();
				}
				if (mLoop)
				{
					unsigned long number = f->getFrameNumber();
					if (mSeeked) mSeeked = 0;
					else if (number < mPrevFrameNumber)
					{
#ifdef _PLAYBACK_DONE_DEBUG
						hlog::writef(logTag, "PlaybackDone(looping): %s", mClipName.cStr());
#endif
						triggerEvent("PlaybackDone");
					}
					mPrevFrameNumber = number;
				}
				if (restoringTexture)
				{
					hlog::write(logTag, this->mClipName + ": Successfully uploaded video frame to restored texture.");
				}
			}
		}
	}

	aprilui::Texture* VideoObject::getTexture()
	{
		return mTexture;
	}

	const harray<aprilui::Texture*>& VideoObject::getTextures()
	{
		return mTextures;
	}

	void VideoObject::_draw()
	{
		updateFrame();
		ImageBox::_draw();
	}
	
	void VideoObject::_update(float timeDelta)
	{
		ImageBox::_update(timeDelta);
		if (mClip)
		{
			if (mAudioPlayer)
			{
				float pitch = mAudioPlayer->getPitch();
				float desiredPitch = mSpeed;
				if (pitch != desiredPitch)
				{
					mAudioPlayer->setPitch(desiredPitch);
				}
			}

			if (!mLoop)
			{
				bool done = mClip->isDone();
				if (done && mAudioPlayer != NULL && mAudioPlayer->isPlaying())
				{
					done = false;
				}
				if (mPrevDoneFlag == 0 && done == 1)
				{
//#ifdef _PLAYBACK_DONE_DEBUG
					hlog::writef(logTag, "PlaybackDone: %s", mClipName.cStr());
//#endif
					triggerEvent("PlaybackDone");
				}
				mPrevDoneFlag = done;
			}
			mClip->update(timeDelta);

			if (mAlphaPauseThreshold < 0 && !isDerivedVisible() && !isPaused())
			{
				updateFrame();
				if (isDebugModeEnabled())
				{
					hlog::write(logTag, mClipName + ": Video object is not visible, but alpha_pause_threshold is set to -1, updating frame");
				}
			}
		}
	}
	
	void VideoObject::setAlphaThreshold(int threshold)
	{
		mAlphaPauseThreshold = hclamp(threshold, -1, 255); // -1 indicates a situation where the user wants the video playing all the time
	}
	
	bool VideoObject::setProperty(chstr name, chstr value)
	{
		if      (name == "video")
		{
			mClipName = value;
			hstr path = getFullPath();
			if (!hresource::exists(path)) throw Exception("Unable to find video file: " + path);
		}
		else if (name == "video_alpha") mUseAlpha = value;
		else if (name == "alpha_pause_threshold") setAlphaThreshold(value);
		else if (name == "alpha_pause_treshold")
		{
			hlog::warn(logTag, "'alpha_pause_treshold=' is deprecated. Use 'alpha_pause_threshold=' instead."); // DEPRECATED
			this->setAlphaThreshold(value);
		}
		else if (name == "loop")
		{
			mLoop = value;
			if (mClip)
			{
				mClip->setAutoRestart(mLoop);
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
			mSpeed = value;
			if (mClip) mClip->setPlaybackSpeed(mSpeed);
		}
		else if (name == "time")
		{
			if (!mClip && mClipName != "") update(0); // try to create the clip if it hasn't been created already
			if (mClip)
			{
				float time = value;
				mSeeked = 1;
				mClip->seek(time);
			}
			else hlog::warn(logTag, "VideoObject ignoring 'time' param, mClip is NULL");
		}
		else if (name == "audio")
		{
			mAudioName = value;
		}
		else if (name == "alpha")
		{
			aprilui::ImageBox::setProperty(name, value);
		}
		else if (name == "sync_offset")
		{
			mAudioSyncOffset = value;
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
			mBlendMode = mode;
			if (mVideoImage)
			{
				mVideoImage->setBlendMode(mode);
			}
		}
		else if (name == "state")
		{
			if (value == "playing")
			{
				if (mClip && mClip->isPaused())
				{
					mClip->play();
				}
			}
			else if (value == "paused")
			{
				if (mClip && !mClip->isPaused()) mClip->pause();
			}
			else throw Exception("VideoObject: unable to set state property to '" + value + "'.");
		}
		else return aprilui::ImageBox::setProperty(name, value);
		return 1;
	}
	
	hstr VideoObject::getProperty(chstr name)
	{
		if (name == "video")						return mClipName;
		if (name == "video_alpha")					return mUseAlpha ? "1" : "0";
		if (name == "alpha_pause_threshold")		return mAlphaPauseThreshold;
		if (name == "alpha_pause_treshold")
		{
			hlog::warn(logTag, "'alpha_pause_treshold' is deprecated. Use 'alpha_pause_threshold' instead."); // DEPRECATED
			return mAlphaPauseThreshold;
		}
		if (name == "loop")							return mLoop ? "1" : "0";
		if (name == "speed")						return mSpeed;
		if (name == "initial_precache_factor")		return mInitialPrecacheFactor;
		if (name == "initial_precache_timeout")		return mInitialPrecacheTimeout;
		if (name == "time")							return this->getTimePosition();
		if (name == "videoWidth" || name == "videoHeight" || name == "duration")
		{
			if (mClip == NULL && mClipName != "")
			{
				_createClip();
			}
			if (name == "duration")		return mClip ? hstr(mClip->getDuration()) : "0";
			if (name == "videoWidth")	return mClip ? hstr(mClip->getWidth()) : "0";
			if (name == "videoHeight")	return mClip ? hstr(mClip->getHeight()) : "0";
			// should never happen
			return "0";
		}
		if (name == "audio")						return mAudioName;
		if (name == "sync_offset")					return mAudioSyncOffset;
		if (name == "blend_mode")
		{
			if (mVideoImage)
			{
				if (mBlendMode == april::BM_DEFAULT)	return "default";
				if (mBlendMode == april::BM_ALPHA)		return "alpha";
				if (mBlendMode == april::BM_ADD)		return "add";
				if (mBlendMode == april::BM_SUBTRACT)	return "subtract";
				if (mBlendMode == april::BM_OVERWRITE)	return "overwrite";
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
