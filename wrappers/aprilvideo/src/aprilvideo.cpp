/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include <hltypes/hlog.h>
#include <theoraplayer/TheoraPlayer.h>
#include <aprilui/aprilui.h>
#include "aprilvideo.h"
#include "VideoObject.h"

namespace aprilvideo
{
	TheoraVideoManager* gVideoManager = NULL;
	int gNumWorkerThreads = 1;
	harray<VideoObject*> gReferences;
	hmutex gReferenceMutex;
	hstr logTag = "aprilvideo", defaultFileExtension = ".ogv";
	bool debugMode = 0;

	bool isDebugModeEnabled()
	{
		return debugMode;
	}
	
	void setDebugModeEnabled(bool enabled)
	{
		debugMode = enabled;
	}
	
	static void theoraLogMessage(std::string log)
	{
#ifdef _DEBUG
		if (hstr(log.c_str()).contains("dropped"))
			hlog::debug(logTag, log.c_str());
		else
			hlog::write(logTag, log.c_str());
#else
		hlog::write(logTag, log.c_str());
#endif
	}

	void init(int num_worker_threads)
	{
		TheoraVideoManager::setLogFunction(theoraLogMessage);
		gNumWorkerThreads = num_worker_threads;
		APRILUI_REGISTER_OBJECT_TYPE(VideoObject);
	}

	void destroy()
	{
		if (gVideoManager)
		{
			delete gVideoManager;
			gVideoManager = NULL;
		}
	}
	
	harray<VideoObject*> getActiveVideoObjects()
	{
		hmutex::ScopeLock lock(&gReferenceMutex);
		return gReferences;
	}
}
