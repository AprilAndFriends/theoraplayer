#pragma once
/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include "OpenAL_AudioInterface.h"

extern void av_init();
extern void av_destroy();
extern void av_update(float);
extern void av_draw();
extern void av_setDebugTitle(char* out);
extern void av_OnKeyPress(int key);
extern void av_OnClick(float x, float y);