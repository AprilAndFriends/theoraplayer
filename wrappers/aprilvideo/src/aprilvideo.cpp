/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#define THEORAUTIL_NOMACROS
#include <theoraplayer/TheoraException.h>
#undef exception_cls
#include <theoraplayer/TheoraDataSource.h>
#include <theoraplayer/TheoraPlayer.h>
#include <april/RenderSystem.h>
#include <april/Texture.h>
#include <aprilui/aprilui.h>
#include <aprilui/Dataset.h>
#include <aprilui/NullImage.h>
#include <aprilui/Texture.h>
#include "aprilvideo.h"

namespace aprilvideo
{
	static TheoraVideoManager* gVideoManager = NULL;
	static int gRefCount = 0, gNumWorkerThreads = 1;

	static int _nextPow2(int x)
	{
		int y;
		for (y=1;y<x;y*=2);
		return y;
	}
	
	VideoObject::VideoObject(chstr name, grect rect) : aprilui::ImageBox(name, rect)
	{
		mUseAlpha = 0;
		mLoop = 1;
		mClip = NULL;
		mVideoImage = NULL;
		mTexture = NULL;
		gRefCount++;
		if (!gVideoManager) gVideoManager = new TheoraVideoManager(gNumWorkerThreads);
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
				mImage = mDataset->getImage("null");
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
			mImage = mDataset->getImage("null");
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
	}

	VideoObject::~VideoObject()
	{
		gRefCount--;
		destroyResources();
		if (gRefCount <= 0 && gVideoManager)
		{
			delete gVideoManager;
			gVideoManager = NULL;
		}
	}
	
	void VideoObject::update(float k)
	{
		if (mClip == NULL && mClipName != "")
		{
			destroyResources();
			hstr path = mDataset->_getFilePath() + "/video/" + mClipName;
			try
			{
				TheoraOutputMode mode;
				if (april::rendersys->getName() == "OpenGL") mode = mUseAlpha ? TH_RGBA : TH_RGB;
				else                                         mode = mUseAlpha ? TH_BGRA : TH_BGRX;

				mClip = gVideoManager->createVideoClip(path, mode, 16);
			}
			catch (_TheoraGenericException& e)
			{
				throw hl_exception(e.getErrorText());
			}
			if (mClip->getWidth() == 0) throw hl_exception("Failed to load video file: " + path);
			mClip->setAutoRestart(mLoop);
			
			float w = mClip->getWidth(), h = mClip->getHeight();
			april::Texture* tex = april::rendersys->createTexture(_nextPow2(w), _nextPow2(h), april::Texture::FORMAT_ARGB);
			mTexture = new aprilui::Texture(tex->getFilename(), tex);
			mVideoImage = new aprilui::Image(mTexture, "video_img", grect(0, 0, w, h));
			mClip->waitForCache(4 / 16.0f, 0.5f);
		}

		if (mClip)
		{
			gVideoManager->update(k);
			TheoraVideoFrame* f = mClip->getNextFrame();
			if (f)
			{
				mImage = mVideoImage;
				if (april::rendersys->getName() == "DirectX9") mTexture->getRenderTexture()->clear();
				mTexture->getRenderTexture()->blit(0, 0, f->getBuffer(), f->getWidth(), f->getHeight(), 3 + mUseAlpha, 0, 0, f->getWidth(), f->getHeight());
				mClip->popFrame();
			}
		}
	}
	
	bool VideoObject::setProperty(chstr name,chstr value)
	{
		if      (name == "video") mClipName = value;
		else if (name == "alpha") mUseAlpha = value;
		else if (name == "loop")  mLoop = value;
	
		else return aprilui::ImageBox::setProperty(name, value);
		return 1;
	}
	
	hstr VideoObject::getProperty(chstr name, bool* property_exists)
	{
		*property_exists = true;
		if      (name == "video") return mClipName;
		else if (name == "alpha") return mUseAlpha ? "1" : "0";
		else if (name == "loop")  return mLoop ? "1" : "0";
		*property_exists = false;
		return ImageBox::getProperty(name, property_exists);
	}

	void init(int num_worker_threads)
	{
		gNumWorkerThreads = num_worker_threads;
		APRILUI_REGISTER_OBJECT_TYPE(VideoObject);
	}
	
	void destroy()
	{
	
	}
}
