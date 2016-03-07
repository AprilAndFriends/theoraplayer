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
/// Demonstrates how multiple videos can be rendered at once easily.

#ifndef THEORAPLAYER_DEMOS_MULTIPLE_H
#define THEORAPLAYER_DEMOS_MULTIPLE_H

#include "demo_basecode.h"

namespace multiple
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
