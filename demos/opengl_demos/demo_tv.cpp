/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// The room 3D models and textures are licensed under the terms of the
/// GNU General Public License(GPL).

#ifdef _DEMO_TV
#include <math.h>
#include <theoraplayer/FrameQueue.h>
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoClip.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_tv.h"
#include "ObjModel.h"
#include "tga.h"
#include "util.h"

namespace tv
{
	unsigned int textureId = 0;
	unsigned int textureIdChair1 = 0;
	unsigned int textureIdChair2 = 0;
	unsigned int textureIdRoom = 0;
	unsigned int textureIdTable = 0;
	unsigned int textureIdTv = 0;
	theoraplayer::VideoClip* clip = NULL;
	bool started = true;
	ObjModel chair1;
	ObjModel chair2;
	ObjModel tv;
	ObjModel room;
	ObjModel table;
	float angleX = 0.0f;
	float angleY = 0.0f;
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;

	// TODOth - this demo is broken and needs to be fixed
	void init()
	{
		clip = theoraplayer::manager->createVideoClip("media/bunny", theoraplayer::FORMAT_RGB);
		//  use this if you want to preload the file into ram and stream from there
		//	clip = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("../media/short"), theoraplayer::FORMAT_RGB);
		clip->setAutoRestart(true);
		textureId = createTexture(potCeil(clip->getWidth()), potCeil(clip->getHeight()));
		textureIdChair1 = loadTexture("media/tv_room/chair1.tga");
		textureIdChair2 = loadTexture("media/tv_room/chair2.tga");
		textureIdRoom = loadTexture("media/tv_room/room.tga");
		textureIdTable = loadTexture("media/tv_room/table.tga");
		textureIdTv = loadTexture("media/tv_room/tv.tga");
		chair1.load("media/tv_room/chair1.obj", textureIdChair1);
		chair2.load("media/tv_room/chair2.obj", textureIdChair2);
		room.load("media/tv_room/room.obj", textureIdRoom);
		table.load("media/tv_room/table.obj", textureIdTable);
		tv.load("media/tv_room/tv.obj", textureIdTv);
		glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_COLOR_MATERIAL);
	}

	void destroy()
	{
		theoraplayer::manager->destroyVideoClip(clip);
		clip = NULL;
		glDeleteTextures(1, &textureId);
		glDeleteTextures(1, &textureIdChair1);
		textureIdChair1 = 0;
		glDeleteTextures(1, &textureIdChair2);
		textureIdChair2 = 0;
		glDeleteTextures(1, &textureIdRoom);
		textureIdRoom = 0;
		glDeleteTextures(1, &textureIdTable);
		textureIdTable = 0;
		glDeleteTextures(1, &textureIdTv);
		textureIdTv = 0;
	}

	void update(float timeDelta)
	{
		float x = 0.0f;
		float y = 0.0f;
		getCursorPosition(x, y);
		angleX = -4.0f * 3.14f * x / windowWidth;
		angleY = 1500.0f * (y - 300.0f) / windowHeight;
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
		glLoadIdentity();
		gluLookAt(sin(angleX) * 400.0f - 200.0f, angleY, cos(angleX) * 400.0f, -200.0f, 150.0f, 0.0f, 0.0f, 1.0f, 0.0f);
		theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
		if (frame != NULL)
		{
			unsigned char* data = frame->getBuffer();
			unsigned int n = clip->getWidth() * frame->getHeight();
			r = g = b = 0;
			for (unsigned int i = 0; i < n; ++i)
			{
				r += data[i * 3];
				g += data[i * 3 + 1];
				b += data[i * 3 + 2];
			}
			r /= n;
			g /= n;
			b /= n;
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), frame->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, frame->getBuffer());
			clip->popFrame();
		}
		float w = clip->getWidth();
		float h = clip->getHeight();
		float tw = potCeil(w);
		float th = potCeil(h);
		glEnable(GL_TEXTURE_2D);
		if (shaderActive)
		{
			enableShader();
		}
		glColor3f(1.0f, 1.0f, 1.0f);
		glPushMatrix();
		glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, 0.0f, -415.0f);
		drawTexturedQuad(textureId, -2.0f * 30.0f, 190.0f, 4.0f * 30.0f, -3.0f * 25.0f, w / tw, h / th);
		glPopMatrix();
		if (shaderActive)
		{
			disableShader();
		}
		glColor3f(0.2f + 0.8f * (r / 255.0f), 0.2f + 0.8f * (g / 255.0f), 0.2f + 0.8f * (b / 255.0f));
		chair1.draw();
		chair2.draw();
		table.draw();
		tv.draw();
		glEnable(GL_CULL_FACE);
		room.draw();
		glDisable(GL_CULL_FACE);
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
		}
	}

	Demo demo = { init, destroy, update, draw, setDebugTitle, onKeyPress, onClick };

}
#endif
