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
#include <theoraplayer/TheoraDataSource.h>

extern void seek_init();
extern void seek_destroy();
extern void seek_update(float);
extern void seek_draw();
extern void seek_setDebugTitle(char* out);
extern void seek_OnKeyPress(int key);
extern void seek_OnClick(float x, float y);