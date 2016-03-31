/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// Graphics in media/brawe have been borrowed with authors' permission from the game
/// "Kaptain Brawe" (http://www.cateia.com/games/games.php?id=23) by "Cateia Games" and
/// "Petar Ivancek". These graphics ARE NOT ALLOWED to be used in any manner other then
/// for the purpose of this demo program.

#ifdef _DEMO_SPRITE_ANIMATION
#include <theoraplayer/FrameQueue.h>
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoClip.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_spriteAnimation.h"
#include "ObjModel.h"
#include "tga.h"
#include "util.h"

#define MAX_VIDEOS 8

namespace spriteAnimation
{
	unsigned int textureId = 0;
	theoraplayer::VideoClip* clips[8] = { NULL };
	int cClip = 0;
	unsigned char buffer[203 * 300 * 4] = { 0 };

	void init()
	{
		std::string orientations[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
		for (int i = 0; i < MAX_VIDEOS; i++)
		{
			// Note - this demo isn't using FORMAT_RGBA for now since the frames in this video are not mod 16 aligned.
			clips[i] = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("media/brawe/brawe_" + orientations[i]), theoraplayer::FORMAT_RGB, 8);
			clips[i]->setAutoRestart(true);
			if (i != 0)
			{
				clips[i]->pause();
			}
		}
		textureId = createTexture(256, 512, GL_RGBA);
		glClearColor(0, 0.5f, 0, 1);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void destroy()
	{
		for (int i = 0; i < MAX_VIDEOS; ++i)
		{
			theoraplayer::manager->destroyVideoClip(clips[i]);
			clips[i] = NULL;
		}
		glDeleteTextures(1, &textureId);
		textureId = 0;
	}

	void update(float timeDelta)
	{
		int newindex = -1;
		float x = 0.0f;
		float y = 0.0f;
		getCursorPosition(x, y);
		if (x >= 400 && y <= 300)
		{
			if (x - 400 < (300 - y) / 2)		newindex = 0;
			else if (x - 400 > (300 - y) * 2)	newindex = 2;
			else								newindex = 1;
		}
		else if (x < 400 && y <= 300)
		{
			if (400 - x < (300 - y) / 2)		newindex = 0;
			else if (400 - x >(300 - y) * 2)	newindex = 6;
			else								newindex = 7;
		}
		else if (x >= 400 && y > 300)
		{
			if (x - 400 < (y - 300) / 2)		newindex = 4;
			else if (x - 400 > (y - 300) * 2)	newindex = 2;
			else								newindex = 3;
		}
		else if (x < 400 && y > 300)
		{
			if (400 - x < (y - 300) / 2)		newindex = 4;
			else if (400 - x >(y - 300) * 2)	newindex = 6;
			else								newindex = 5;
		}
		if (newindex >= 0)
		{
			clips[cClip]->pause();
			cClip = newindex;
			clips[cClip]->play();
		}
	}

	void draw()
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
		theoraplayer::VideoFrame* frame = clips[cClip]->fetchNextFrame();
		if (frame != NULL)
		{
			unsigned char* src = frame->getBuffer();
			int i = 0;
			int j = 0;
			int k = 0;
			int x = 0;
			int y = 0;
			int w = frame->getWidth();
			for (y = 0; y < 300; y++)
			{
				for (x = 0; x < 203; x++)
				{
					i = (y * 203 + x) * 4;
					j = ((y + 2) * w + x + 4) * 3;
					k = ((y + 2) * w + x + 205 + 4) * 3;
					buffer[i] = src[j];
					buffer[i + 1] = src[j + 1];
					buffer[i + 2] = src[j + 2];
					buffer[i + 3] = src[k];
				}
			}
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 203, 300, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
			clips[cClip]->popFrame();
		}
		glEnable(GL_TEXTURE_2D);
		drawTexturedQuad(textureId, 298.0f, 150.0f, 203.0f, 300.0f, 203.0f / 256.0f, 300.0f / 512.0f);
	}

	void setDebugTitle(char* out)
	{
		char temp[64] = { 0 };
		for (int i = 0; i < MAX_VIDEOS; i++)
		{
			sprintf(temp, "%d/%d  ", clips[i]->getReadyFramesCount(), clips[i]->getPrecachedFramesCount());
			strcat(out, temp);
		}
		strcat(out, temp);
		strcat(out, " (Kaptain Brawe)");
	}

	Demo demo = { init, destroy, update, draw, setDebugTitle, NULL, NULL };

}
#endif
