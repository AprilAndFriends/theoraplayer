/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _DEMO_AV
#include <theoraplayer/AudioInterfaceFactory.h>
#include <theoraplayer/Manager.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_av.h"
#include "OpenAL_AudioInterface.h"
#include "util.h"

namespace av
{
	unsigned int textureId = 0;
	theoraplayer::VideoClip* clip = NULL;
	bool started = false;
	theoraplayer::AudioInterfaceFactory* audioInterfaceFactor = NULL;

	// TODOth - remove when auto-detection is added
#ifdef MP4_VIDEO
	theoraplayer::OutputMode outputMode = theoraplayer::FORMAT_BGRX;
	unsigned int textureFormat = GL_BGRA_EXT;
#else
	theoraplayer::OutputMode outputMode = theoraplayer::FORMAT_RGB;
	unsigned int textureFormat = GL_RGB;
#endif

	void init()
	{
		audioInterfaceFactor = new OpenAL_AudioInterfaceFactory();
		theoraplayer::manager->setAudioInterfaceFactory(audioInterfaceFactor);
		/*// Test Memory Leaks
		while (true)
		{
			clip = mgr_av->createVideoClip("media/bunny", outputMode, 16);
			clip->seek((rand() % 50) / 10.0f);
			//threadSleep(rand()%1000);
			theoraplayer::manager->update(0.0f);
			theoraplayer::manager->destroyVideoClip(clip);
		}
		//*/
		clip = theoraplayer::manager->createVideoClip("media/sample", outputMode, 16);
		//  use this if you want to preload the file into ram and stream from there
		//	clip = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("../media/short"), theoraplayer::FORMAT_RGB);
		clip->setAutoRestart(true);
		clip->pause();
		textureId = createTexture(potCeil(clip->getWidth()), potCeil(clip->getHeight()), textureFormat);
	}

	void destroy()
	{
		theoraplayer::destroy();
		delete audioInterfaceFactor;
		audioInterfaceFactor = NULL;
	}

	void update(float timeDelta)
	{
		if (!started)
		{
			// let's wait until the system caches up a few frames on startup
			if (clip->getReadyFramesCount() < 2)
			{
				return;
			}
			started = true;
			clip->play();
		}
	}

	void draw()
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
		theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
		if (frame != NULL)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), frame->getHeight(), textureFormat, GL_UNSIGNED_BYTE, frame->getBuffer());
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
		drawTexturedQuad(textureId, 0.0f, 0.0f, 800.0f, 570.0f, (float)w / tw, (float)h / th);
		if (shaderActive)
		{
			disableShader();
		}
		glDisable(GL_TEXTURE_2D);
		drawColoredQuad(0.0f, 570.0f, 800.0f, 30.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		drawWiredQuad(0.0f, 570.0f, 800.0f, 29.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		float offset = clip->getTimePosition() / clip->getDuration();
		drawColoredQuad(3.0f, 573.0f, 794.0f * offset, 24.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	void setDebugTitle(char* out)
	{
		float bufferSize = 0;
		OpenAL_AudioInterface* audioInterface = (OpenAL_AudioInterface*)clip->getAudioInterface();
		if (audioInterface != NULL)
		{
			bufferSize = audioInterface->getQueuedAudioSize();
		}
		int dropped = clip->getDroppedFramesCount();
		sprintf(out, "%d precached, %d dropped, buffered audio: %.2f s", clip->getReadyFramesCount(), dropped, bufferSize);
	}

	void onKeyPress(int key)
	{
		if (key == ' ')
		{
			clip->isPaused() ? clip->play() : clip->pause();
		}
		if (key == 5)
		{
			clip->setOutputMode(theoraplayer::FORMAT_RGB);
		}
		if (key == 6)
		{
			clip->setOutputMode(theoraplayer::FORMAT_YUV);
		}
		if (key == 7)
		{
			clip->setOutputMode(theoraplayer::FORMAT_GREY3);
		}
	}

	void onClick(float x, float y)
	{
		if (y > 570.0f)
		{
			clip->seek((x / windowWidth) * clip->getDuration());
			clip->waitForCache();
		}
	}

	Demo demo = { init, destroy, update, draw, setDebugTitle, onKeyPress, onClick };

}
#endif
