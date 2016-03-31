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

#include <stdlib.h>

#include "util.h"

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

bool shaderActive = false;
float FOVY = 45;

#ifndef WIN32
unsigned long GetTickCount()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0)
	{
		return 0;
	}
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
#endif

std::string str(int i)
{
	char s[64] = { 0 };
	sprintf(s, "%d", i);
	return std::string(s);
}

void threadSleep(int milliseconds)
{
#ifdef _WIN32
	Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

int potCeil(int value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	++value;
	return value;
}

unsigned int createTexture(int w, int h, unsigned int format)
{
	unsigned int textureId = 0;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	unsigned char* data = new unsigned char[w * h * 4];
	memset(data, 0, w * h * 4);
	glTexImage2D(GL_TEXTURE_2D, 0, format == GL_RGB ? GL_RGB : GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	delete[] data;
	return textureId;
}
