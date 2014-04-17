/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
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

#ifdef _IOS
#define __NPOT_TEXTURE // iOS armv7 and newer devices support non-power-of-two textures so let's use it.
#endif

//#define _TEXWRITE_BENCHMARK // uncomment this to benchmark texture upload speed

namespace aprilvideo
{
	extern int gRefCount, gNumWorkerThreads;
	extern TheoraVideoManager* gVideoManager;
	extern hstr defaultFileExtension;
	
	static int _nextPow2(int x)
	{
		int y;
		for (y = 1; y < x; y *= 2);
		return y;
	}
	
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
		++gRefCount;
		mAlphaPauseTreshold = 0;
		mPrevFrameNumber = 0;
		mSeeked = 0;
		mPrevAlpha = 255;
		mBlendMode = april::BM_DEFAULT;
		
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
		--gRefCount;
		destroyResources();
		if (gRefCount <= 0 && gVideoManager)
		{
			hlog::write(logTag, "Destroying Video manager, no more active clips found.");
			delete gVideoManager;
			gVideoManager = NULL;
		}
	}
	
	bool VideoObject::isPlaying()
	{
		return (mClip != NULL && !mClip->isPaused() && !mClip->isDone());
	}
	
	bool VideoObject::isPaused()
	{
		return (mClip != NULL && mClip->isPaused());
	}
	
	bool VideoObject::isStopped()
	{
		return (mClip == NULL || mClip->isDone());
	}
	
	float VideoObject::getTimePosition()
	{
		return (mClip != NULL ? mClip->getTimePosition() : 0.0f);
	}
	
	aprilui::Object* VideoObject::createInstance(chstr name, grect rect)
	{
		return new VideoObject(name, rect);
	}
	
	void VideoObject::notifyEvent(chstr name, void* params)
	{
		if (name == "AttachToObject")
		{
			if (this->image != mVideoImage)
			{
				this->image = this->dataset->getImage(APRILUI_IMAGE_NAME_NULL);
			}
		}
		ImageBox::notifyEvent(name, params);
	}
	
	void VideoObject::destroyResources()
	{
		if (mVideoImage)
		{
			delete mVideoImage;
			mVideoImage = NULL;
			this->image = this->dataset->getImage(APRILUI_IMAGE_NAME_NULL);
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
	
	void VideoObject::createClip()
	{
		hstr path = getFullPath();
		april::Image::Format textureFormat;
		destroyResources();
		
		try
		{
			TheoraOutputMode mode;
			if (mUseAlpha)
			{
				textureFormat = april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGBA);
			}
			else
			{
#ifdef _ANDROID
				// android has better performance if using rgbx
				textureFormat = april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGBX);
#else
				textureFormat = april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGB);
#endif
			}
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
#if defined(_ANDROID) || defined(_WINRT) && defined(_WINARM)
			// Android/WinRT ARM libtheoraplayer uses ARM optimized libtheora which is faster but stil slower than
			// a native hardware accelerated codec. so (for the moment) we use a larger precache to counter it.
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
						source = new TheoraMemoryFileDataSource(data, size);
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
#ifdef __NPOT_TEXTURE
        float tw = w, th = h;
#else
        float tw = _nextPow2(w), th = _nextPow2(h);
#endif
		april::Texture* tex = april::rendersys->createTexture(tw, th, april::Color::Clear, textureFormat, april::Texture::TYPE_VOLATILE);
        tex->setAddressMode(april::Texture::ADDRESS_CLAMP);
		mTexture = new aprilui::Texture(tex->getFilename(), tex);
		mVideoImage = new aprilui::Image(mTexture, "video_img", grect(mClip->getSubFrameOffsetX(), mClip->getSubFrameOffsetY(), mClip->getSubFrameWidth(), mClip->getSubFrameHeight()));
        mVideoImage->setBlendMode(mBlendMode);
#if defined(_ANDROID) || defined(_WINRT) && defined(_WINARM)
		hlog::write(logTag, "Waiting for cache: " + path);
#endif
		mClip->waitForCache(2 / mClip->getNumPrecachedFrames(), 5.0f); // better to wait a while then to display an empty image
		mClip->waitForCache(0.25f, 0.5f);
		
#if defined(_ANDROID) || defined(_WINRT) && defined(_WINARM)
		if (w * h >= 768 * 384) // best to fill the cache on large videos on Android/WinRT ARM to counter a slower codec
			mClip->waitForCache(0.9f, 2.0f);
		
		hlog::write(logTag, "Initial precache cached " + hstr(mClip->getNumPrecachedFrames()) + " frames");
#endif
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
				xal::mgr->createCategory("video", xal::ON_DEMAND, xal::DISK);
			}
			mSound = xal::mgr->createSound(this->dataset->getFilePath() + "/video/" + mAudioName, category);

			mAudioPlayer = xal::mgr->createPlayer(mSound->getName());
			mTimer = new AudioVideoTimer(mAudioPlayer, mAudioSyncOffset);
			mClip->setTimer(mTimer);

			mAudioPlayer->play();
		}
		else if (mSpeed != 1.0f) mClip->setPlaybackSpeed(mSpeed);
		update(0); // to grab the first frame.
	}
	
	void VideoObject::OnDraw()
	{
		if (mClip == NULL && mClipName != "") createClip();
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
				this->image->setSrcRect(r);
				april::Image::Format textureFormat;
				if (mUseAlpha)
				{
					textureFormat = april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGBA);
				}
				else
				{
#ifdef _ANDROID
					// android has better performance if using rgbx
					textureFormat = april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGBX);
#else
					textureFormat = april::rendersys->getNativeTextureFormat(april::Image::FORMAT_RGB);
#endif
				}
#ifdef _TEXWRITE_BENCHMARK
				long t = clock();
				int n = 256;
				char msg[1024];
				for (int i = 0; i < n; i++)
				{
					mTexture->getRenderTexture()->write(0, 0, r.w, r.h, 0, 0, f->getBuffer(), r.w, r.h, textureFormat);
				}
				float diff = ((float) (clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
				sprintf(msg, "BENCHMARK: uploading n %dx%d video frames to texture took %.1fms (%.2fms average per frame)\n", (int) r.w, (int )r.h, diff, diff / n);
				hlog::write(logTag, msg);
#else
				mTexture->getRenderTexture()->write(0, 0, r.w, r.h, 0, 0, f->getBuffer(), r.w, r.h, textureFormat);
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
		ImageBox::OnDraw();
	}
	
	void VideoObject::update(float k)
	{
		ImageBox::update(k);
		
		if (mClip)
		{
			if (!mLoop)
			{
				bool done = mClip->isDone();
				if (mPrevDoneFlag == 0 && done == 1) triggerEvent("PlaybackDone");
				mPrevDoneFlag = done;
			}
			mClip->update(k);
			//mClip->decodedAudioCheck();
            if (mPrevAlpha != getAlpha())
            {
                mPrevAlpha = getAlpha();
                bool should_pause = mAlphaPauseTreshold == 0 ? !isVisible() : getAlpha() <= mAlphaPauseTreshold;
                if (should_pause && !mClip->isPaused()) mClip->pause();
                else if (!should_pause && mClip->isPaused()) mClip->play();
			}
		}
	}
	
	void VideoObject::setAlphaTreshold(int treshold)
	{
		mAlphaPauseTreshold = hclamp(treshold, 0, 255);
	}
	
	bool VideoObject::setProperty(chstr name,chstr value)
	{
		if      (name == "video")
		{
			mClipName = value;
			hstr path = getFullPath();
			if (!hresource::exists(path)) throw hl_exception("Unable to find video file: " + path);
		}
		else if (name == "video_alpha") mUseAlpha = value;
		else if (name == "alpha_pause_treshold") setAlphaTreshold(value);
		else if (name == "loop")  mLoop = value;
		else if (name == "speed") mSpeed = value;
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
            else if (value == "substract")	mode = april::BM_SUBTRACT;
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
                if (mClip && mClip->isPaused()) mClip->play();
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
	
	hstr VideoObject::getProperty(chstr name, bool* property_exists)
	{
		*property_exists = true;
		if      (name == "video") return mClipName;
		else if (name == "video_alpha") return mUseAlpha ? "1" : "0";
		else if (name == "alpha_pause_treshold") return mAlphaPauseTreshold;
		else if (name == "loop")  return mLoop ? "1" : "0";
		else if (name == "speed") return mSpeed;
		else if (name == "time") return this->getTimePosition();
		else if (name == "duration") return mClip ? hstr(mClip->getDuration()) : hstr(0);
		else if (name == "audio")  return mAudioName;
		else if (name == "sync_offset")  return mAudioSyncOffset;
        else if (name == "blend_mode")
        {
            if (mVideoImage)
            {
                if		(mBlendMode == april::BM_DEFAULT)	return "default";
                else if (mBlendMode == april::BM_ALPHA)		return "alpha";
                if      (mBlendMode == april::BM_ADD)		return "add";
                else if (mBlendMode == april::BM_SUBTRACT)	return "substract";
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
		*property_exists = false;
		return ImageBox::getProperty(name, property_exists);
	}
}
