#pragma once
/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

/************************************************************************************
COPYRIGHT INFO: The room 3D models and textures are licensed under the terms of the
GNU General Public License (GPL).
*************************************************************************************/
#include "demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/DataSource.h>
#include "ObjModel.h"
#include "tga.h"
#include <math.h>

extern void tv_init();
extern void tv_destroy();
extern void tv_update(float);
extern void tv_draw();
extern void tv_setDebugTitle(char* out);
extern void tv_OnKeyPress(int key);
extern void tv_OnClick(float x, float y);