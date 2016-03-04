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
#include <theoraplayer/DataSource.h>

extern void multiple_init();
extern void multiple_destroy();
extern void multiple_update(float);
extern void multiple_draw();
extern void multiple_setDebugTitle(char* out);
extern void multiple_OnKeyPress(int key);
extern void multiple_OnClick(float x, float y);