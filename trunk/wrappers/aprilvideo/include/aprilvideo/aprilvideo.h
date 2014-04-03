/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#ifndef APRILVIDEO_H
#define APRILVIDEO_H

#include <hltypes/hstring.h>

#include "aprilvideoExport.h"

namespace aprilvideo
{
	extern hstr logTag;

	void aprilVideoFnExport init(int num_worker_threads = 1);
	void aprilVideoFnExport destroy();

}
#endif
