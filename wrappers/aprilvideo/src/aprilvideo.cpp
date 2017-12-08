/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <april/Platform.h>
#include <aprilui/aprilui.h>
#include <hltypes/hlog.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/Manager.h>
#ifdef _IOS
#include <clipavfoundation/clipavfoundation.h>
#endif
#include "aprilvideo.h"
#include "Utility.h"
#include "VideoObject.h"

namespace aprilvideo
{
	hstr logTag = "aprilvideo";
	bool debugModeEnabled = false;
	harray<VideoObject*> videoObjects;
	hmutex videoObjectsMutex;

	static void _theoraLogMessage(const std::string& message)
	{
#ifdef _DEBUG
		if (hstr(message.c_str()).contains("dropped"))
		{
			hlog::debug(logTag, message.c_str());
		}
		else
#endif
		{
			hlog::write(logTag, message.c_str());
		}
	}

	void init(int workerThreadCount)
	{
		theoraplayer::init(workerThreadCount);
		theoraplayer::setLogFunction(&_theoraLogMessage);
		APRILUI_REGISTER_OBJECT_TYPE(VideoObject);
	}

	void destroy()
	{
		theoraplayer::destroy();
	}

	void update(float timeDelta)
	{
		theoraplayer::manager->update(timeDelta);
	}

	int getPreloadToRamSizeLimit()
	{
		int limit = 64;
		int ram = april::getSystemInfo().ram;
#ifdef _ANDROID
		limit = 16;
		if (ram < 1024)
		{
			limit = 8;
		}
#elif defined(_WINRT) && defined(_WINP8)
		limit = 32;
		if (ram < 1024)
		{
			limit = 16;
		}
#endif
		return limit;
	}
	
	bool isDebugModeEnabled()
	{
		return debugModeEnabled;
	}

	void setDebugModeEnabled(bool value)
	{
		debugModeEnabled = value;
	}

	harray<VideoObject*> getActiveVideoObjects()
	{
		hmutex::ScopeLock lock(&videoObjectsMutex);
		return videoObjects;
	}

}
