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

	extern hstr logTag;

	void aprilvideoFnExport init(int workerThreadCount = 1);
	void aprilvideoFnExport destroy();

	void aprilvideoFnExport update(float timeDelta);
	
	bool aprilvideoFnExport isDebugModeEnabled();
	void aprilvideoFnExport setDebugModeEnabled(bool value);

	harray<VideoObject*> aprilvideoFnExport getActiveVideoObjects();

}
#endif
