/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// The room 3D models and textures are licensed under the terms of the
/// GNU General Public License(GPL).
/// 
/// @section DESCRIPTION
/// 
/// Demostrates how to simulate light using videos.

#ifndef THEORAPLAYER_DEMOS_LIGHT_H
#define THEORAPLAYER_DEMOS_LIGHT_H

#include "demo_basecode.h"

namespace light
{
	extern Demo demo;

	extern void init();
	extern void destroy();
	extern void update(float timeDelta);
	extern void draw();
	extern void setDebugTitle(char* out);
	extern void onKeyPress(int key);
	extern void onClick(float x, float y);

}
#endif
