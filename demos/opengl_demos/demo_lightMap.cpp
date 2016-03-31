/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _DEMO_LIGHT_MAP
#include <theoraplayer/FrameQueue.h>
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoClip.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_lightMap.h"
#include "ObjModel.h"
#include "tga.h"
#include "util.h"

namespace lightMap
{
	struct xyz
	{
		float x;
		float y;
		float z;
	};

	unsigned int textureIdLightMap = 0;
	unsigned int textureIdDiffuseMap = 0;
	theoraplayer::VideoClip* clip = NULL;
	bool started = false;
	bool diffuseEnabled = true;
	bool lightingEnabled = true;
	std::vector<xyz> cameraPosition;
	ObjModel roomLight;
	float angleX = 0.0f;
	float angleY = 0.0f;

	void init()
	{
		FILE* file = fopen("media/lighting/camera.txt", "r");
		xyz pos;
		while (!feof(file))
		{
			fscanf(file, "%f %f %f", &pos.x, &pos.y, &pos.z);
			cameraPosition.push_back(pos);
		}
		fclose(file);
		FOVY = 54.495f;
		theoraplayer::manager->setWorkerThreadCount(1);
		clip = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("media/lighting/lighting"), theoraplayer::FORMAT_RGB);
		clip->setAutoRestart(true);
		textureIdLightMap = createTexture(potCeil(clip->getWidth()), potCeil(clip->getHeight()));
		textureIdDiffuseMap = loadTexture("media/lighting/diffuse_map.tga");
		roomLight.load("media/lighting/room.obj", textureIdDiffuseMap);
		glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glDepthFunc(GL_LESS);
		glEnable(GL_COLOR_MATERIAL);
	}

	void destroy()
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_COLOR_MATERIAL);
		theoraplayer::manager->destroyVideoClip(clip);
		clip = NULL;
		glDeleteTextures(1, &textureIdLightMap);
		textureIdLightMap = 0;
		glDeleteTextures(1, &textureIdDiffuseMap);
		textureIdDiffuseMap = 0;
	}

	void update(float timeDelta)
	{
		float x = 0.0f;
		float y = 0.0f;
		getCursorPosition(x, y);
		angleX = -4.0f * 3.14f * x / windowWidth;
		angleY = 1500.0f * (y - 300.0f) / windowHeight;
	}

	void draw()
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glLoadIdentity();
		float x1 = 0.0f;
		float y1 = 0.0f;
		float z1 = 0.0f;
		float x2 = -65.147f;
		float y2 = 80.219f;
		float z2 = 12.301f;
		static int index = 0;
		theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
		if (frame != NULL)
		{
			index = (int)frame->getFrameNumber();
			unsigned char* buffer = frame->getBuffer();
			int x = 0;
			int length = frame->getWidth() * frame->getHeight() * 3;
			for (int i = 0; i < length; ++i)
			{
				x = (*buffer) * 0.8f + 255 * 0.2f;
				*buffer = (x > 255 ? 255 : x);
				++buffer;
			}
			glBindTexture(GL_TEXTURE_2D, textureIdLightMap);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->getWidth(), frame->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, frame->getBuffer());
			clip->popFrame();
		}
		// TODOth - crashes here when demo ends, find out why (index = -1)
		x1 = cameraPosition[index].x;
		y1 = cameraPosition[index].y;
		z1 = cameraPosition[index].z;
		gluLookAt(x1, z1, -y1, x2, z2, -y2, 0.0f, 1.0f, 0.0f);
		glBlendFunc(GL_ONE, GL_ZERO);
		glEnable(GL_CULL_FACE);
		diffuseEnabled ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
		roomLight.draw();
		glDisable(GL_CULL_FACE);
		if (lightingEnabled)
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, textureIdLightMap);
			glPushMatrix();
			glLoadIdentity();
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glDisable(GL_DEPTH_TEST);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 4.0f / 1024.0f);
			glVertex3f(-1.0f, 1.0f, 0.0f);
			glTexCoord2f(800.0f / 1024.0f, 4.0f / 1024.0f);
			glVertex3f(1.0f, 1.0f, 0.0f);
			glTexCoord2f(800.0f / 1024.0f, 604.0f / 1024.0f);
			glVertex3f(1.0f, -1.0f, 0.0f);
			glTexCoord2f(0.0f, 604.0f / 1024.0f);
			glVertex3f(-1.0f, -1.0f, 0.0f);
			glEnd();
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
			glEnable(GL_DEPTH_TEST);
		}
	}

	void setDebugTitle(char* out)
	{
		sprintf(out, "press SPACE to toggle diffuse map, ENTER to toggle lighting");
	}

	void onKeyPress(int key)
	{
		if (key == ' ')
		{
			diffuseEnabled = !diffuseEnabled;
		}
		if (key == 13)
		{
			lightingEnabled = !lightingEnabled; // 13 = ENTER key
		}
	}

	Demo demo = { init, destroy, update, draw, setDebugTitle, onKeyPress, NULL };

}
#endif
