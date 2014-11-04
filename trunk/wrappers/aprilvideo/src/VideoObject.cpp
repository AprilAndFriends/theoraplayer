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
#include <aprilui/NullImage.h>
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
	
	VideoObject::VideoObject(chstr name, grect rect) : aprilui::ImageBox(name, rect)
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
		mAlphaPauseTreshold = 0;
		mPrevFrameNumber = 0;
		mSeeked = 0;
		mPrevAlpha = 255;
		mBlendMode = april::BM_DEFAULT;
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
				// pass the exception further as a hltypes::exception so the general system can uderstand it
				throw hl_exception(e.getErrorText());
			}
			std::vector<std::string> lst = gVideoManager->getSupportedDecoders();
			foreach (std::string, it, lst)
			{
				if (*it == "AVFoundation") defaultFileExtension = ".mp4";
			}
		}
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
		if (mAlphaPauseTreshold == 0)
		{
			bool visible = isDerivedVisible();
			return !visible;
		}
		else if (mAlphaPauseTreshold < 0)
		{
			return false;
		}
		else
		{
			int alpha = getDerivedAlpha() * getVisibilityFlag();
			return alpha < mAlphaPauseTreshold;
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
		value = hclamp(value, 0.0f, 1.0f);
	}

	aprilui::Object* VideoObject::createInstance(chstr name, grect rect)
	{
		return new VideoObject(name, rect);
	}
	
	void VideoObject::notifyEvent(chstr type, aprilui::EventArgs* args)
	{
		if (type == aprilui::Event::AttachedToObject)
		{
			if (this->image != mVideoImage)
			{
				this->image = this->dataset->getImage(APRILUI_IMAGE_NAME_NULL);
			}
		}
		ImageBox::notifyEvent(type, args);
	}
	
	void VideoObject::destroyResources()
	{
		if (mVideoImage)
		{
			delete mVideoImage;
			mVideoImage = NULL;
			this->image = this->dataset != NULL ? this->dataset->getImage(APRILUI_IMAGE_NAME_NULL) : NULL;
		}
		if (mTexture)
		{
			delete mTexture;
			mTexture = NULL;
		}
		if (mClip)
		{
			gVideoManager->destroyVideoClip(mClip);
			mClip = NULL;
		}
		if (mAudioPlayer)
		{
			xal::mgr->destroyPlayer(mAudioPlayer);
			mAudioPlayer = NULL;
		}
		if (mSound)
		{
			xal::mgr->destroySound(mSound);
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
		hstr path = this->dataset->getFilePath() + "/video/" + mClipName;
		if (!path.ends_with(".ogg") && !path.ends_with(".ogv") && !path.ends_with(".mp4"))
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
#ifdef _ANDROID
			// android has better performance if using rgbx
			return  april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGBX);
#else
			return  april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGB);
#endif
		}
	}
	
	bool VideoObject::_isClipCreated()
	{
		return mClip != NULL;
	}
	
	float VideoObject::getPrecacheFactor()
	{
		if (mClip == NULL)
		{
			return 0;
		}
		return ((float) mClip->getNumReadyFrames()) / mClip->getNumPrecachedFrames();
	}
	
	void VideoObject::_createClip(bool waitForCache)
	{
		hstr path = getFullPath();
		april::Image::Format textureFormat = _getTextureFormat();
		destroyResources();
        
        if (path.ends_with(".mp4"))
        {
            hstr archive = hresource::getArchive();
            if (archive != "")
            {
                path = hrdir::join_path(archive, path);
            }
        }
		
		try
		{
			TheoraOutputMode mode;

			if (textureFormat == april::Image::FORMAT_RGBA)				mode = TH_RGBA;
			else if (textureFormat == april::Image::FORMAT_RGBX)		mode = TH_RGBX;
			else if (textureFormat == april::Image::FORMAT_BGRA)		mode = TH_BGRA;
			else if (textureFormat == april::Image::FORMAT_BGRX)		mode = TH_BGRX;
			else if (textureFormat == april::Image::FORMAT_ARGB)		mode = TH_ARGB;
			else if (textureFormat == april::Image::FORMAT_XRGB)		mode = TH_XRGB;
			else if (textureFormat == april::Image::FORMAT_ABGR)		mode = TH_ABGR;
			else if (textureFormat == april::Image::FORMAT_XBGR)		mode = TH_XBGR;
			else if (textureFormat == april::Image::FORMAT_RGB)			mode = TH_RGB;
			else if (textureFormat == april::Image::FORMAT_BGR)			mode = TH_BGR;
			else if (textureFormat == april::Image::FORMAT_GRAYSCALE)	mode = TH_GREY;
			int ram = april::getSystemInfo().ram;
			int precached = 16;
#if defined(_ANDROID) || defined(_WINRT) && !defined(_WINP8)
			// Android and WinRT libtheoraplayer uses an optimized libtheora which is faster, but still slower than
			// a native hardware accelerated codec. So (for now) we use a larger precache to counter it. Though, WinP8 can't handle this memory-wise.
			if (ram > 512) precached = 32;
#else
			if      (ram < 384) precached = 4;
			else if (ram < 512) precached = 6;
			else if (ram < 1024)
			{
				if (path.contains("lowres")) precached = 16;
				else precached = 8;
			}
#endif
			
			if (path.ends_with("mp4"))
			{
				try
				{
					if (april::window->getName() == "OpenKODE") // because mp4's are opened via apple's api, and that doesn't play nice with OpenKODE dir structure.
						mClip = gVideoManager->createVideoClip("res/" + path, mode, precached);
					else
						mClip = gVideoManager->createVideoClip(path, mode, precached);
				}
				catch (_TheoraGenericException& e)
				{
					// pass the exception further as a hltypes::exception so the general system can uderstand it
					throw hl_exception(e.getErrorText());
				}
			}
			else
			{
				if (!path.ends_with(".mp4") && ram > 256)
				{
					hresource r(path);
					unsigned long size = r.size();
					TheoraDataSource* source;

					// additional performance optimization: preload file in RAM to speed up decoding, every bit counts on Android/WinRT ARM
					// but only for "reasonably" sized files
					if (size < 64 * 1024 * 1024)
					{
						hlog::write(logTag, "Preloading video file to memory: " + path);
						unsigned char* data = new unsigned char[size];
						r.read_raw(data, (int) size);
						source = new TheoraMemoryFileDataSource(data, size, path);
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
			throw hl_exception(e.getErrorText());
		}
		if (mClip->getWidth() == 0) throw hl_exception("Failed to load video file: " + path);
		mClip->setAutoRestart(mLoop);
		
		float w = mClip->getWidth(), h = mClip->getHeight();
		float tw = w, th = h;
		april::RenderSystem::Caps caps = april::rendersys->getCaps();
		if (!caps.npotTexturesLimited && !caps.npotTextures)
		{
			tw = hpotceil(tw);
			th = hpotceil(th);
		}
		april::Texture* tex = april::rendersys->createTexture(tw, th, april::Color::Clear, textureFormat, april::Texture::TYPE_VOLATILE);
        tex->setAddressMode(april::Texture::ADDRESS_CLAMP);
		mTexture = new aprilui::Texture(tex->getFilename(), tex);
		mVideoImage = new aprilui::Image(mTexture, "video_img", grect(mClip->getSubFrameOffsetX(), mClip->getSubFrameOffsetY(), mClip->getSubFrameWidth(), mClip->getSubFrameHeight()));
        mVideoImage->setBlendMode(mBlendMode);
		if (waitForCache && mInitialPrecacheFactor > 0.0f)
		{
			float factor = hmax(2.0f / mClip->getNumPrecachedFrames(), mInitialPrecacheFactor);
			float precached = (float) mClip->getNumReadyFrames() / mClip->getNumPrecachedFrames();
			if (precached < factor)
			{
				hlog::writef(logTag, "Waiting for cache (%.1f%% / %.1f%%): %s", precached * 100.0f, factor * 100.0f, path.c_str());
				if (factor > 0)
				{
					precached = mClip->waitForCache(factor, 5.0f); // better to wait a while then to display an empty image
					if (precached < 0.25f && factor >= 0.25f)
					{
						precached = mClip->waitForCache(0.25f, 0.5f);
					}
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
				harray<hstr> folders = hrdir::split_path(mAudioName);
				hstr path_category = folders[folders.size() - 2];
				if (xal::mgr->hasCategory(path_category)) category = path_category;
			}
			if (category == "video" && !xal::mgr->hasCategory("video"))
			{
#if defined(_WINRT) || defined(_ANDROID)
				xal::mgr->createCategory("video", xal::ON_DEMAND, xal::DISK);
#else
				if (april::getSystemInfo().ram >= 512)
				{
					xal::mgr->createCategory("video", xal::STREAMED, xal::RAM);
				}
				else
				{
					xal::mgr->createCategory("video", xal::STREAMED, xal::DISK);
				}
#endif
			}
			mSound = xal::mgr->createSound(this->dataset->getFilePath() + "/video/" + mAudioName, category);

			mAudioPlayer = xal::mgr->createPlayer(mSound->getName());
			mTimer = new AudioVideoTimer(this, mAudioPlayer, mAudioSyncOffset);
		}
		if (mTimer == NULL) mTimer = new VideoTimer(this);
		mClip->setTimer(mTimer);
		mClip->setPlaybackSpeed(mSpeed);
		update(0); // to grab the first frame.
	}
	
	void VideoObject::updateFrame()
	{
		if (mClip == NULL && mClipName != "") _createClip();
		if (mClip)
		{
			TheoraVideoFrame* f = mClip->getNextFrame();
			bool pop = true;
			if (!mTexture->isLoaded())
			{
				mTexture->load();
				if (!f)
				{
					if (mClip->getNumReadyFrames() == 0)
					{
						mClip->waitForCache();
					}
					f = mClip->getFrameQueue()->getFirstAvailableFrame();
					pop = false;
				}
			}
			if (f)
			{
				this->image = mVideoImage;
				grect r = this->image->getSrcRect();
				r.w = f->getWidth();
				r.h = f->getHeight();
				april::Image::Format textureFormat = _getTextureFormat();
#ifdef _TEXWRITE_BENCHMARK
				long t = clock();
				int n = 256;
				char msg[1024];
				for (int i = 0; i < n; i++)
				{
					mTexture->getTexture()->write(0, 0, r.w, r.h, 0, 0, f->getBuffer(), r.w, r.h, textureFormat);
				}
				float diff = ((float) (clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
				sprintf(msg, "BENCHMARK: uploading n %dx%d video frames to texture took %.1fms (%.2fms average per frame)\n", (int) r.w, (int )r.h, diff, diff / n);
				hlog::write(logTag, msg);
#else
				mTexture->getTexture()->write(0, 0, r.w, r.h, 0, 0, f->getBuffer(), r.w, r.h, textureFormat);
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
						triggerEvent("PlaybackDone");
					}
					mPrevFrameNumber = number;
				}
			}
		}

	}
	
	void VideoObject::_draw()
	{
		updateFrame();
		ImageBox::_draw();
	}
	
	void VideoObject::update(float timeDelta)
	{
		ImageBox::update(timeDelta);
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
				if (done && mAudioPlayer != NULL && mAudioPlayer->isPlaying()) done = false;
				if (mPrevDoneFlag == 0 && done == 1)
				{
					triggerEvent("PlaybackDone");
				}
				mPrevDoneFlag = done;
			}
			mClip->update(timeDelta);

			if (mAlphaPauseTreshold < 0 && !isDerivedVisible() && !isPaused())
			{
				updateFrame();
				if (isDebugModeEnabled())
				{
					hlog::write(logTag, mClipName + ": Video object is not visible, but alpha_pause_treshold is set to -1, updating frame");
				}
			}
		}
	}
	
	void VideoObject::setAlphaTreshold(int treshold)
	{
		mAlphaPauseTreshold = hclamp(treshold, -1, 255); // -1 indicates a situation where the user wants the video playing all the time
	}
	
	bool VideoObject::setProperty(chstr name, chstr value)
	{
		if      (name == "video")
		{
			mClipName = value;
			hstr path = getFullPath();
			if (!hresource::exists(path)) throw hl_exception("Unable to find video file: " + path);
		}
		else if (name == "video_alpha") mUseAlpha = value;
		else if (name == "alpha_pause_treshold") setAlphaTreshold(value);
		else if (name == "loop")
        {
            mLoop = value;
            if (mClip)
            {
                mClip->setAutoRestart(mLoop);
//                if (mLoop && !mClip->g)
            }
        }
		else if (name == "initial_precache_factor")
		{
			setInitialPrecacheFactor(value);
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
                hlog::errorf(logTag, "Unknown VideoObject blend mode: %s", name.c_str());
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
            else throw hl_exception("VideoObject: unable to set state property to '" + value + "'.");
        }
		else return aprilui::ImageBox::setProperty(name, value);
		return 1;
	}
	
	hstr VideoObject::getProperty(chstr name)
	{
		if      (name == "video") return mClipName;
		else if (name == "video_alpha") return mUseAlpha ? "1" : "0";
		else if (name == "alpha_pause_treshold") return mAlphaPauseTreshold;
		else if (name == "loop")  return mLoop ? "1" : "0";
		else if (name == "speed") return mSpeed;
		else if (name == "initial_precache_factor") return mInitialPrecacheFactor;
		else if (name == "time") return this->getTimePosition();
		else if (name == "videoWidth" || name == "videoHeight" || name == "duration")
		{
			if (mClip == NULL && mClipName != "")
			{
				_createClip();
			}
			if		(name == "duration")
			{
				return mClip ? hstr(mClip->getDuration()) : "0";
			}
			else if (name == "videoWidth")
			{
				return mClip ? hstr(mClip->getWidth()) : "0";
			}
			else if (name == "videoHeight")
			{
				return mClip ? hstr(mClip->getHeight()) : "0";
			}
		}
		else if (name == "audio")  return mAudioName;
		else if (name == "sync_offset")  return mAudioSyncOffset;
        else if (name == "blend_mode")
        {
            if (mVideoImage)
            {
                if		(mBlendMode == april::BM_DEFAULT)	return "default";
                else if (mBlendMode == april::BM_ALPHA)		return "alpha";
                if      (mBlendMode == april::BM_ADD)		return "add";
                else if (mBlendMode == april::BM_SUBTRACT)	return "subtract";
				else if (mBlendMode == april::BM_OVERWRITE)	return "overwrite";
                else return "unknown";
            }
            else
            {
                hlog::error(logTag, "Unable to get blend_mode to VideoObject, image is NULL");
                return "";
            }
        }
        else if (name == "state")
        {
			if (this->isPlaying()) return "playing";
			if (this->isPaused()) return "paused";
			if (this->isStopped()) return "stopped";
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
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("alpha_pause_treshold", aprilui::PropertyDescription::INT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("loop", aprilui::PropertyDescription::BOOL);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("initial_precache_factor", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("speed", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("time", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("duration", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("videoWidth", aprilui::PropertyDescription::INT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("videoHeight", aprilui::PropertyDescription::INT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("audio", aprilui::PropertyDescription::STRING);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("syc_offset", aprilui::PropertyDescription::FLOAT);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("blend_mode", aprilui::PropertyDescription::STRING);
			VideoObject::_propertyDescriptions += aprilui::PropertyDescription("state", aprilui::PropertyDescription::STRING);
		}
		return (aprilui::ImageBox::getPropertyDescriptions() + VideoObject::_propertyDescriptions);
	}
}
