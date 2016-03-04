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
COPYRIGHT INFO: Graphics in media/locv have been borrowed
with authors permission from the game:
Game:   Legend of Crystal Valley ( http://locv.cateia.com/ )
Author: Cateia Games

These grapchics ARE NOT ALLOWED to be used in any manner other then for the purpose
of this demo program.
*************************************************************************************/
#include <math.h>
#include "demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/DataSource.h>
#include "tga.h"

extern void comp_init();
extern void comp_destroy();
extern void comp_update(float);
extern void comp_draw();
extern void comp_setDebugTitle(char* out);
extern void comp_OnKeyPress(int key);
extern void comp_OnClick(float x, float y);