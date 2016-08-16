/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Provides a public interface for aprilvideo.

#ifndef APRILVIDEO_H
#define APRILVIDEO_H

#include <hltypes/hstring.h>

#include "aprilvideoExport.h"

namespace aprilvideo
{
	class VideoObject;

	aprilvideoExport extern hstr logTag;

	aprilvideoFnExport void init(int workerThreadCount = 1);
	aprilvideoFnExport void destroy();

	aprilvideoFnExport void update(float timeDelta);
	
	aprilvideoFnExport int getPreloadToRamSizeLimit();
	aprilvideoFnExport bool isDebugModeEnabled();
	aprilvideoFnExport void setDebugModeEnabled(bool value);

	harray<VideoObject*> aprilvideoFnExport getActiveVideoObjects();

}
#endif
