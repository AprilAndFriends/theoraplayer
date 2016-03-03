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
/// Defines generic helper functions

#ifndef THEORAPLAYER_H
#define THEORAPLAYER_H

// TODOth - maybe move these headers elsewhere

#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"

#include "theoraplayerExport.h"
#include "VideoFrame.h"

namespace theoraplayer
{
	theoraplayerFnExport void init();
	theoraplayerFnExport void destroy();

	theoraplayerFnExport void registerFormatLoader();

}
#endif

