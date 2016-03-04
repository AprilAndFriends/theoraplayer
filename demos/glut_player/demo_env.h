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
#include "ObjModel.h"
#include "tga.h"
#include <math.h>

extern void env_init();
extern void env_destroy();
extern void env_update(float);
extern void env_draw();
extern void env_setDebugTitle(char* out);
extern void env_OnKeyPress(int key);
extern void env_OnClick(float x, float y);