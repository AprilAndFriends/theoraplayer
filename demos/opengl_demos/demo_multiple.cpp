/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_multiple.h"
#include "util.h"

#define MAX_VIDEOS 4 // used for readability

theoraplayer::VideoClip* clips_multiple[MAX_VIDEOS] = { 0 };
unsigned int textureIds_multiple[MAX_VIDEOS] = { 0 };

// TODOth - should do this dynamically
#ifdef MP4_VIDEO
theoraplayer::TheoraOutputMode outputMode_multiple = theoraplayer::TH_BGRX;
unsigned int textureFormat_multiple = GL_BGRA_EXT;
unsigned int uploadFormat_multiple = GL_BGRA_EXT;
#else
theoraplayer::TheoraOutputMode outputMode_multiple = theoraplayer::TH_RGB;
unsigned int textureFormat_multiple = GL_RGB;
unsigned int uploadFormat_multiple = GL_RGB;
#endif

void drawVideo(int x, int y, unsigned int textureId, theoraplayer::VideoClip* clip)
{
	//glLoadIdentity();
	//glTranslatef(x,y,0);
	glBindTexture(GL_TEXTURE_2D, textureId);
	theoraplayer::VideoFrame* frame = clip->getNextFrame();
	if (frame != NULL)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), frame->getHeight(), uploadFormat_multiple, GL_UNSIGNED_BYTE, frame->getBuffer());
		clip->popFrame();
	}
	int w = clip->getWidth();
	int h = clip->getHeight();
	int tw = potCeil(w);
	int th = potCeil(h);

	glEnable(GL_TEXTURE_2D);
	if (shaderActive)
	{
		enableShader();
	}
	drawTexturedQuad(textureId, x, y, 395, 285, w / tw, h / th);
	if (shaderActive)
	{
		disableShader();
	}
	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(x, y + 285.0f, 395.0f, 15.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	drawWiredQuad(x, y + 285.0f, 395.0f, 14.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	float p = clip->getTimePosition() / clip->getDuration();
	drawColoredQuad(x + 1.5f, y + 287.0f / 2, 784 * p * 0.5f, 11.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

void multiple_draw()
{
	drawVideo(0, 0, textureIds_multiple[0], clips_multiple[0]);
	drawVideo(400, 0, textureIds_multiple[1], clips_multiple[1]);
	drawVideo(0, 300, textureIds_multiple[2], clips_multiple[2]);
	drawVideo(400, 300, textureIds_multiple[3], clips_multiple[3]);
}

void multiple_update(float timeDelta)
{
	theoraplayer::manager->update(timeDelta);
}

void multiple_setDebugTitle(char* out)
{
	char temp[64] = { 0 };
	for (int i = 0; i < MAX_VIDEOS; ++i)
	{
		sprintf(temp, "%d/%d ", clips_multiple[i]->getNumReadyFrames(), clips_multiple[i]->getNumPrecachedFrames());
		strcat(out, temp);
	}
	sprintf(temp, "(%d worker threads)", theoraplayer::manager->getWorkerThreadCount());
	strcat(out, temp);
}

void playPause(int index)
{
	clips_multiple[index]->isPaused() ? clips_multiple[index]->play() : clips_multiple[index]->pause();
}

void multiple_onKeyPress(int key)
{
	if (key == '1')
	{
		theoraplayer::manager->setWorkerThreadCount(1);
	}
	if (key == '2')
	{
		theoraplayer::manager->setWorkerThreadCount(2);
	}
	if (key == '3')
	{
		theoraplayer::manager->setWorkerThreadCount(3);
	}
	if (key == '4')
	{
		theoraplayer::manager->setWorkerThreadCount(4);
	}
	if (key >= 1 && key <= 4)
	{
		playPause(key - 1); // Function keys are used for play/pause
	}
	if (key == 5 || key == 6 || key == 7)
	{
		theoraplayer::TheoraOutputMode mode;
		if (key == 5)
		{
			mode = theoraplayer::TH_RGB;
		}
		if (key == 6)
		{
			mode = theoraplayer::TH_YUV;
		}
		if (key == 7)
		{
			mode = theoraplayer::TH_GREY3;
		}
		for (int i = 0; i < MAX_VIDEOS; ++i)
		{
			clips_multiple[i]->setOutputMode(mode);
		}
	}
}

void multiple_onClick(float x, float y)
{
}

void multiple_init()
{
	printf("---\nUSAGE: press buttons 1,2,3 or 4 to change the number of worker threads\n---\n");

	std::string files[] = { "media/bunny" + resourceExtension,
		"media/konqi" + resourceExtension,
		"media/room" + resourceExtension,
		"media/titan" + resourceExtension };
	theoraplayer::manager->setWorkerThreadCount(MAX_VIDEOS);
	theoraplayer::manager->setDefaultPrecachedFramesCount(16);
	for (int i = 0; i < MAX_VIDEOS; ++i)
	{
		clips_multiple[i] = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource(files[i]), outputMode_multiple);

		clips_multiple[i]->setAutoRestart(1);
		textureIds_multiple[i] = createTexture(potCeil(clips_multiple[i]->getWidth()), potCeil(clips_multiple[i]->getHeight()), textureFormat_multiple);
	}
}

void multiple_destroy()
{
}
