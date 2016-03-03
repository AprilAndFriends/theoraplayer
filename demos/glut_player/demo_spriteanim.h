#pragma once/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

/************************************************************************************
COPYRIGHT INFO: Sprite animation has been borrowed with authors permission from the game:
Game:   Kaptain Brawe ( http://kaptainbrawe.cateia.com/ )
Author: Petar Ivancek

These animations ARE NOT ALLOWED to be used in any manner other then for the purpose
of this demo program.
*************************************************************************************/
#include "demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/TheoraDataSource.h>

extern void spriteanim_init();
extern void spriteanim_destroy();
extern void spriteanim_update(float);
extern void spriteanim_draw();
extern void spriteanim_setDebugTitle(char* out);
extern void spriteanim_OnKeyPress(int key);
extern void spriteanim_OnClick(float x, float y);