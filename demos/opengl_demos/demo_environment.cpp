/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _DEMO_ENVIRONMENT
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoClip.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_environment.h"
#include "ObjModel.h"
#include "util.h"

namespace environment
{
	unsigned int textureId = 0;
	theoraplayer::VideoClip* clip = NULL;
	float angle = 0.0f;
	ObjModel teapot;

	void init()
	{
		theoraplayer::manager->setWorkerThreadCount(1);
		clip = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("media/environment_mapping/room256"), theoraplayer::FORMAT_RGB);
		clip->setAutoRestart(true);
		textureId = createTexture(potCeil(clip->getWidth()), potCeil(clip->getHeight()));
		teapot.load("media/environment_mapping/teapot.obj", textureId, true);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		// TODOth - fix this
		//glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		//glDepthFunc(GL_LESS);
		glEnable(GL_COLOR_MATERIAL);
	}

	void destroy()
	{
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_COLOR_MATERIAL);
		theoraplayer::manager->destroyVideoClip(clip);
		clip = NULL;
		teapot.unload();
		glDeleteTextures(1, &textureId);
		textureId = 0;
	}

	void update(float timeDelta)
	{
		angle += timeDelta * 20;
	}

	void draw()
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glLoadIdentity();
		// TODOth - fix this
		//gluLookAt(0.0, 2000.0, 1000.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0);
		//glRotatef(angle, 0.0f, 0.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, textureId);
		theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
		if (frame != NULL)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), frame->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, frame->getBuffer());
			clip->popFrame();
		}
		teapot.draw();
	}

	Demo demo = { init, destroy, update, draw, NULL, NULL, NULL };

}
#endif
