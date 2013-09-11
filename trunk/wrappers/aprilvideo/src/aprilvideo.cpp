/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#define THEORAUTIL_NOMACROS
#include <theoraplayer/TheoraException.h>
#undef exception_cls
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/TheoraTimer.h>
#include <april/RenderSystem.h>
#include <april/april.h>
#include <april/Platform.h>
#include <april/Texture.h>
#include <aprilui/aprilui.h>
#include <aprilui/Dataset.h>
#include <aprilui/NullImage.h>
#include <aprilui/Texture.h>
#include <xal/AudioManager.h>
#include <xal/Player.h>
#include <xal/Sound.h>
#include "aprilvideo.h"
#include "AprilVideoDataSource.h"

#ifdef _IOS
#define __NPOT_TEXTURE // iOS armv7 and newer devices support non-power-of-two textures so let's use it.
#endif

namespace aprilvideo
{
	static TheoraVideoManager* gVideoManager = NULL;
	static int gRefCount = 0, gNumWorkerThreads = 1;
	hstr logTag = "aprilvideo", defaultFileExtension = ".ogv";

	static int _nextPow2(int x)
	{
		int y;
		for (y = 1; y < x; y *= 2);
		return y;
	}

	class AudioVideoTimer : public TheoraTimer
	{
		float mSyncOffset;
		xal::Player* mPlayer;
		float mT;
		bool mDisabledAudio;
	public:
		AudioVideoTimer(xal::Player* player, float sync_offset)
		{
			mSyncOffset = sync_offset;
			mPlayer = player;
			mT = 0;
			static hstr audiosystem = xal::mgr->getName(); // Disabled" audio system doesn't sync audio & video
			mDisabledAudio = (audiosystem == "Disabled");
		}
		
		void update(float time_increase)
		{
			if (!mDisabledAudio)
			{
				if (mPlayer->isPlaying())
					mTime = mPlayer->getTimePosition() - mSyncOffset;
				else
					mTime += time_increase;
			}
			else
			{
				mT += time_increase;
				mTime = mT - mSyncOffset;
			}
		}
	};
	

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
		gRefCount++;
		mAlphaPauseTreshold = 0;
		mPrevFrameNumber = 0;
		mSeeked = 0;
		mPrevAlpha = 255;
		
				
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
		gRefCount--;
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
			if (mImage != mVideoImage)
			{
				mImage = mDataset->getImage(APRILUI_IMAGE_NAME_NULL);
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
			mImage = mDataset->getImage(APRILUI_IMAGE_NAME_NULL);
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
		hstr path = mDataset->getFilePath() + "/video/" + mClipName;
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
		april::Texture::Format textureFormat;
		destroyResources();
		
		try
		{
			TheoraOutputMode mode;
			if (april::rendersys->getName().starts_with("OpenGL"))
			{
				mode = mUseAlpha ? TH_RGBA : TH_RGBX;
			}
			else
			{
				mode = mUseAlpha ? TH_BGRA : TH_BGRX;
			}
			textureFormat = april::Texture::FORMAT_ARGB;
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
#if defined(_ANDROID) || defined(_WINRT) && defined(_WINARM)
				hresource r(path);
				int size = r.size();
				TheoraDataSource* source;
				
				// additional Android/WinRT ARM optimization: preload file in RAM to speed up decoding, every bit counts on Android/WinRT ARM
				// but only for "reasonably" sized files
				if (size < 64 * 1024 * 1024)
				{
					hlog::write(logTag, "Preloading video file to memory: " + path);
					unsigned char* data = new unsigned char[size];
					r.read_raw(data, size);
					source = new TheoraMemoryFileDataSource(data, size);
				}
				else
					source = new AprilVideoDataSource(path);
				
				mClip = gVideoManager->createVideoClip(source, mode, precached);
				r.close();
				hlog::write(logTag, "Created video clip.");

#else
				mClip = gVideoManager->createVideoClip(new AprilVideoDataSource(path), mode, precached);
#endif
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
		april::Texture* tex = april::rendersys->createTexture(tw, th, textureFormat);
        tex->setAddressMode(april::Texture::ADDRESS_CLAMP);
		mTexture = new aprilui::Texture(tex->getFilename(), tex);
		mVideoImage = new aprilui::Image(mTexture, "video_img", grect(mClip->getSubFrameOffsetX(), mClip->getSubFrameOffsetY(), mClip->getSubFrameWidth(), mClip->getSubFrameHeight()));
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
			if (!xal::mgr->hasCategory("video"))
			{
				xal::mgr->createCategory("video", xal::ON_DEMAND, xal::DISK);
			}
			mSound = xal::mgr->createSound(mDataset->getFilePath() + "/video/" + mAudioName, "video");
			
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
			TheoraVideoFrame* f = mClip->getNextFrame();
			if (f)
			{
				mImage = mVideoImage;
				grect r = mImage->getSrcRect();
				r.w = f->getWidth();
				r.h = f->getHeight();
				mImage->setSrcRect(r);
#if defined(_ANDROID) || defined(_WINRT) && defined(_WINARM)
				mTexture->load();
#endif
				mTexture->getRenderTexture()->write(0, 0, f->getBuffer(), r.w, r.h, 4);
				mClip->popFrame();
				if (mLoop)
				{
					int number = f->getFrameNumber();
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

	
	static void theoraLogMessage(std::string log)
	{
		hlog::write("aprilvideo", log);
	}
	
	void init(int num_worker_threads)
	{
		TheoraVideoManager::setLogFunction(theoraLogMessage);
		gNumWorkerThreads = num_worker_threads;
		APRILUI_REGISTER_OBJECT_TYPE(VideoObject);
	}

	void destroy()
	{
	
	}
}
