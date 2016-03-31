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
/// Contains some basic definitions that are required for all OpenGL related code to work.

#ifndef THEORAPLAYER_DEMOS_UTIL_H
#define THEORAPLAYER_DEMOS_UTIL_H

#include <string>

#include "demo_basecode.h"

extern float FOVY;
extern bool shaderActive;

#ifndef WIN32
unsigned long GetTickCount();
#endif
std::string str(int i);
void threadSleep(int milliseconds);
int potCeil(int x);
unsigned int createTexture(int w, int h, unsigned int format = GL_RGB);

#endif
