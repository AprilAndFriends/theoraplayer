/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include <hltypes/hlog.h>
#include <theoraplayer/TheoraPlayer.h>
#include <aprilui/aprilui.h>
#include "aprilvideo.h"
#include "VideoObject.h"

#ifdef _IOS
#define __NPOT_TEXTURE // iOS armv7 and newer devices support non-power-of-two textures so let's use it.
#endif

namespace aprilvideo
{
	TheoraVideoManager* gVideoManager = NULL;
	int gRefCount = 0, gNumWorkerThreads = 1;
	hstr logTag = "aprilvideo", defaultFileExtension = ".ogv";

	static void theoraLogMessage(std::string log)
	{
#ifdef _DEBUG
		if (hstr(log).contains("dropped"))
			hlog::debug("aprilvideo", log);
		else
			hlog::write("aprilvideo", log);
#else
		hlog::write("aprilvideo", log);
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
	
	}
}
