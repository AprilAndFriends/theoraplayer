/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _DEMO_MULTIPLE
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_multiple.h"
#include "util.h"

#define MAX_VIDEOS 4 // used for readability

namespace multiple
{
	theoraplayer::VideoClip* clips[MAX_VIDEOS] = { 0 };
	unsigned int textureIds[MAX_VIDEOS] = { 0 };

	// TODOth - should do this dynamically
#ifdef MP4_VIDEO
	theoraplayer::OutputMode outputMode = theoraplayer::FORMAT_BGRX;
	unsigned int textureFormat = GL_BGRA_EXT;
	unsigned int uploadFormat = GL_BGRA_EXT;
#else
	theoraplayer::OutputMode outputMode = theoraplayer::FORMAT_RGB;
	unsigned int textureFormat = GL_RGB;
	unsigned int uploadFormat = GL_RGB;
#endif

	void init()
	{
		printf("---\nUSAGE: press buttons 1,2,3 or 4 to change the number of worker threads\n---\n");
		std::string files[] = { "media/bunny", "media/konqi", "media/room", "media/titan" };
		theoraplayer::manager->setWorkerThreadCount(MAX_VIDEOS);
		theoraplayer::manager->setDefaultPrecachedFramesCount(16);
		for (int i = 0; i < MAX_VIDEOS; ++i)
		{
			clips[i] = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource(files[i]), outputMode);
			clips[i]->setAutoRestart(true);
			textureIds[i] = createTexture(potCeil(clips[i]->getWidth()), potCeil(clips[i]->getHeight()), textureFormat);
		}
	}

	void destroy()
	{
		for (int i = 0; i < MAX_VIDEOS; ++i)
		{
			theoraplayer::manager->destroyVideoClip(clips[i]);
			clips[i] = NULL;
		}
		glDeleteTextures(MAX_VIDEOS, textureIds);
		memset(textureIds, 0, sizeof(textureIds));
	}

	void _drawVideo(int x, int y, unsigned int textureId, theoraplayer::VideoClip* clip)
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
		theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
		if (frame != NULL)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), frame->getHeight(), uploadFormat, GL_UNSIGNED_BYTE, frame->getBuffer());
			clip->popFrame();
		}
		float w = clip->getSubFrameWidth();
		float h = clip->getSubFrameHeight();
		float sx = clip->getSubFrameX();
		float sy = clip->getSubFrameY();
		float tw = potCeil(w);
		float th = potCeil(h);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureId);
		if (shaderActive)
		{
			enableShader();
		}
		drawTexturedQuad(textureId, x, y, 395.0f, 285.0f, w / tw, h / th, sx / tw, sy / th);
		if (shaderActive)
		{
			disableShader();
		}
		glDisable(GL_TEXTURE_2D);
		drawColoredQuad(x, y + 285.0f, 395.0f, 15.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		drawWiredQuad(x, y + 285.0f, 395.0f, 14.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		float p = clip->getTimePosition() / clip->getDuration();
		drawColoredQuad(x + 1.5f, y + 286.0f, 784.0f * p * 0.5f, 11.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	void draw()
	{
		glLoadIdentity();
		_drawVideo(0, 0, textureIds[0], clips[0]);
		_drawVideo(400, 0, textureIds[1], clips[1]);
		_drawVideo(0, 300, textureIds[2], clips[2]);
		_drawVideo(400, 300, textureIds[3], clips[3]);
	}

	void setDebugTitle(char* newTitle)
	{
		char temp[64] = { 0 };
		for (int i = 0; i < MAX_VIDEOS; ++i)
		{
			sprintf(temp, "%d/%d ", clips[i]->getReadyFramesCount(), clips[i]->getPrecachedFramesCount());
			strcat(newTitle, temp);
		}
		sprintf(temp, "(%d worker threads)", theoraplayer::manager->getWorkerThreadCount());
		strcat(newTitle, temp);
	}

	void _playPause(int index)
	{
		clips[index]->isPaused() ? clips[index]->play() : clips[index]->pause();
	}

	void onKeyPress(int key)
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
		// TODOth - this doesn't seem to work and should also be fixed in other places
		if (key >= 1 && key <= 4)
		{
			_playPause(key - 1); // Function keys are used for play/pause
		}
		// TODOth - this doesn't seem to work and should also be fixed in other places
		if (key == 5 || key == 6 || key == 7)
		{
			theoraplayer::OutputMode mode;
			if (key == 5)
			{
				mode = theoraplayer::FORMAT_RGB;
			}
			if (key == 6)
			{
				mode = theoraplayer::FORMAT_YUV;
			}
			if (key == 7)
			{
				mode = theoraplayer::FORMAT_GREY3;
			}
			for (int i = 0; i < MAX_VIDEOS; ++i)
			{
				clips[i]->setOutputMode(mode);
			}
		}
	}

	Demo demo = { init, destroy, NULL, draw, setDebugTitle, onKeyPress, NULL };

}
#endif
