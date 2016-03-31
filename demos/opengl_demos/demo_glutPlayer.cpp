/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _DEMO_GLUT_PLAYER
#include <theoraplayer/FrameQueue.h>
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoClip.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_glutPlayer.h"
#include "util.h"

namespace glutPlayer
{
	unsigned int textureId = 0;
	theoraplayer::VideoClip* clip = NULL;
	bool started = false;

#ifdef MP4_VIDEO
	theoraplayer::OutputMode outputMode = theoraplayer::FORMAT_BGRX;
	unsigned int textureFormat = GL_BGRA_EXT;
#else
	theoraplayer::OutputMode outputMode = theoraplayer::FORMAT_RGB;
	unsigned int textureFormat = GL_RGB;
#endif

	void init()
	{
		theoraplayer::manager->setWorkerThreadCount(1);
#ifdef BENCHMARK
		benchmark("media/witch_intro.ogv");
		benchmark("media/hotel_intro.ogv");
		benchmark("media/angels_intro.ogv");
		benchmark("media/witch_intro.mp4");
		benchmark("media/hotel_intro.mp4");
		benchmark("media/angels_intro.mp4");
#endif
		clip = theoraplayer::manager->createVideoClip("media/bunny", outputMode, 16);
		//  use this if you want to preload the file into ram and stream from there
		//	clip = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("../media/short"), theoraplayer::FORMAT_RGB);
		clip->setAutoRestart(true);
		textureId = createTexture(potCeil(clip->getWidth()), potCeil(clip->getHeight()), textureFormat);
	}

	void destroy()
	{
		theoraplayer::manager->destroyVideoClip(clip);
		clip = NULL;
		glDeleteTextures(1, &textureId);
		textureId = 0;
	}

	void update(float timeDelta)
	{
		if (!started)
		{
			// let's wait until the system caches up a few frames on startup
			if (clip->getReadyFramesCount() < clip->getPrecachedFramesCount() * 0.5f)
			{
				return;
			}
			started = true;
		}
	}

	void draw()
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
		theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
		if (frame != NULL)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), clip->getHeight(), textureFormat, GL_UNSIGNED_BYTE, frame->getBuffer());
			clip->popFrame();
		}
		float w = clip->getSubFrameWidth();
		float h = clip->getSubFrameHeight();
		float sx = clip->getSubFrameX();
		float sy = clip->getSubFrameY();
		float tw = potCeil(w);
		float th = potCeil(h);
		glEnable(GL_TEXTURE_2D);
		if (shaderActive)
		{
			enableShader();
		}
		drawTexturedQuad(textureId, 0.0f, 0.0f, 800.0f, 570.0f, w / tw, h / th, sx / tw, sy / th);
		if (shaderActive)
		{
			disableShader();
		}
		glDisable(GL_TEXTURE_2D);
		drawColoredQuad(0.0f, 570.0f, 800.0f, 30.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		drawWiredQuad(0.0f, 570.0f, 800.0f, 29.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		float x = clip->getTimePosition() / clip->getDuration();
		drawColoredQuad(3.0f, 573.0f, 794.0f * x, 24.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	void setDebugTitle(char* out)
	{
		int dropped = clip->getDroppedFramesCount();
		int displayed = clip->getDisplayedFramesCount();
		float percent = 100 * ((float)dropped / displayed);
		sprintf(out, " (%dx%d) %d precached, %d displayed, %d dropped (%.1f %%)", clip->getWidth(), clip->getHeight(), clip->getReadyFramesCount(), displayed, dropped, percent);
	}

	void onKeyPress(int key)
	{
		if (key == ' ')
		{
			clip->isPaused() ? clip->play() : clip->pause();
		}
		if (key == 114)
		{
			clip->restart();
			clip->pause();
			clip->play();
		}
		if (key == 5)
		{
			clip->setOutputMode(theoraplayer::FORMAT_RGB);
		}
		else if (key == 6)
		{
			clip->setOutputMode(theoraplayer::FORMAT_YUV);
		}
		else if (key == 7)
		{
			clip->setOutputMode(theoraplayer::FORMAT_GREY3);
		}
	}

	void onClick(float x, float y)
	{
		if (y > 570.0f)
		{
			clip->seek((x / windowWidth) * clip->getDuration());
		}
	}

#ifdef BENCHMARK
	void benchmark(const char* filename)
	{
		int precachedCount = 256;
		int count = precachedCount;
		theoraplayer::VideoClip* clip = theoraplayer::manager->createVideoClip(filename, outputMode, 32);
		clock_t t = clock();
		while (count > 0)
		{
			clip->waitForCache(1.0f, 1000000);
			count -= 32;
			clip->getFrameQueue()->clear();
		}
		float diff = ((float)(clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
		printf("%s: Decoding %d frames took %.1fms (%.2fms average per frame)\n", filename, precachedCount, diff, diff / precachedCount);
		theoraplayer::manager->destroyVideoClip(clip);
		clip = NULL;
	}
#endif

	Demo demo = { init, destroy, update, draw, setDebugTitle, onKeyPress, onClick };

}
#endif
