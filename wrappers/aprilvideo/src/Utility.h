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
/// Provides utility definitions.

#ifndef APRILVIDEO_UTILITY_H
#define APRILVIDEO_UTILITY_H

#include <hltypes/harray.h>
#include <hltypes/hmutex.h>

#include "VideoObject.h"

namespace aprilvideo
{
	extern harray<VideoObject*> videoObjects;
	extern hmutex videoObjectsMutex;

}
#endif
