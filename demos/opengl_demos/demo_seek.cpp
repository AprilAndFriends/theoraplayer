/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _DEMO_SEEK
#include <theoraplayer/FrameQueue.h>
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoClip.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_seek.h"
#include "util.h"

// TODOth - is this demo still used?

namespace seek
{
	unsigned int textureId = 0;
	theoraplayer::VideoClip* clip = NULL;
	bool needsSeek = true;
	int cFrame = 0;
	int wrongSeeks = 0;
	float delay = 0.0f;

	void init()
	{
		clip = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("media/bunny"), theoraplayer::FORMAT_RGB, 4);
		clip->setAutoRestart(true);
		textureId = createTexture(potCeil(clip->getWidth()), potCeil(clip->getHeight()));
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
		if (needsSeek)
		{
			delay += timeDelta;
			if (delay >= 0.0f)
			{
				delay = 0.0f;
				printf("Requesting seek to frame %d\n", cFrame);
				clip->seekToFrame(cFrame);
				needsSeek = false;
			}
		}
	}

	void draw()
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
		if (!needsSeek)
		{
			theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
			if (frame != NULL)
			{
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), frame->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, frame->getBuffer());
				needsSeek = true;
				if (frame->getFrameNumber() != cFrame)
				{
					++wrongSeeks;
				}
				++cFrame;
				if (cFrame >= clip->getFramesCount())
				{
					cFrame = 0;
				}
				printf("Displayed frame %d\n", (int)frame->getFrameNumber());
				clip->popFrame();
			}
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
		sprintf(out, " (%dx%d@%d) %d wrong seeks", clip->getWidth(), clip->getHeight(), (int)clip->getFps(), wrongSeeks);
	}

	Demo demo = { init, destroy, update, draw, setDebugTitle, NULL, NULL };

}
#endif
