/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// Graphics in media/locv have been borrowed with authors' permission from the game
/// "Legend of Crystal Valley" (http://www.cateia.com/games/games.php?id=43) by "Cateia Games".
/// These graphics ARE NOT ALLOWED to be used in any manner other then for the purpose
/// of this demo program.

#ifdef _DEMO_COMPOSITE
#include <math.h>
#include <theoraplayer/Manager.h>
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoClip.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_composite.h"
#include "tga.h"
#include "util.h"

namespace composite
{
	theoraplayer::VideoClip* clipWater = NULL;
	theoraplayer::VideoClip* clipEve = NULL;
	unsigned int textureIdWater = 0;
	unsigned int textureIdEve = 0;
	unsigned int textureIdMain = 0;
	unsigned int textureIdBack = 0;
	unsigned int textureIdBranch = 0;
	unsigned int textureIdBirds = 0;
	unsigned int textureIdBush = 0;
	unsigned int textureIdClouds = 0;
	unsigned char buffer[256 * 336 * 4];
	float timer = 0.0f;

	void init()
	{
		theoraplayer::manager->setWorkerThreadCount(2);
		clipWater = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("media/locv/locv_water"), theoraplayer::FORMAT_RGB, 4);
		clipWater->setPlaybackSpeed(0.5f);
		clipWater->setAutoRestart(true);
		clipEve = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("media/locv/locv_eve"), theoraplayer::FORMAT_RGBA, 4);
		clipEve->setAutoRestart(true);
		textureIdWater = createTexture(potCeil(clipWater->getWidth()), potCeil(clipWater->getHeight()));
		textureIdEve = createTexture(potCeil(clipEve->getWidth()), potCeil(clipEve->getHeight()), GL_RGBA);
		textureIdMain = loadTexture("media/locv/locv_main.tga");
		textureIdBack = loadTexture("media/locv/locv_back.tga");
		textureIdBranch = loadTexture("media/locv/locv_branch.tga");
		textureIdBirds = loadTexture("media/locv/locv_birds.tga");
		textureIdBush = loadTexture("media/locv/locv_bush.tga");
		textureIdClouds = loadTexture("media/locv/locv_clouds.tga");
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void destroy()
	{
		glDisable(GL_BLEND);
		theoraplayer::manager->destroyVideoClip(clipWater);
		clipWater = NULL;
		theoraplayer::manager->destroyVideoClip(clipEve);
		clipEve = NULL;
		glDeleteTextures(1, &textureIdWater);
		textureIdWater = 0;
		glDeleteTextures(1, &textureIdEve);
		textureIdEve = 0;
		glDeleteTextures(1, &textureIdMain);
		textureIdMain = 0;
		glDeleteTextures(1, &textureIdBack);
		textureIdBack = 0;
		glDeleteTextures(1, &textureIdBranch);
		textureIdBranch = 0;
		glDeleteTextures(1, &textureIdBirds);
		textureIdBirds = 0;
		glDeleteTextures(1, &textureIdBush);
		textureIdBush = 0;
		glDeleteTextures(1, &textureIdClouds);
		textureIdClouds = 0;
	}

	void update(float timeDelta)
	{
		timer += timeDelta;
	}

	void draw()
	{
		// images
		glBindTexture(GL_TEXTURE_2D, textureIdBack);
		drawTexturedQuad(textureIdBack, 0.0f, 0.0f, 1024.0f, 512.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, textureIdClouds);
		drawTexturedQuad(textureIdClouds, 0.0f, 200.0f, 1024.0f, 350.0f, 1.0f, 0.6f, timer / 200.0f, 0.4f);
		glBindTexture(GL_TEXTURE_2D, textureIdBirds);
		drawTexturedQuad(textureIdBirds, ((int)(timer * 30.0f)) % 800 + 300.0f, 50.0f + sin(timer) * 5.0f, 64.0f, 32.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, textureIdMain);
		drawTexturedQuad(textureIdMain, 0.0f, 0.0f, 1024.0f, 768.0f, 1.0f, 0.75f);
		glBindTexture(GL_TEXTURE_2D, textureIdBranch);
		glPushMatrix();
		glTranslatef(1034.0f, -10.0f, 0.0f);
		glRotatef((float)sin(timer) * 1.0f, 0.0f, 0.0f, 1.0f);
		drawTexturedQuad(textureIdBranch, -510.0f, 0.0f, 510.0f, 254.0f, 510.0f / 512.0f, 254.0f / 256.0f, 1.0f / 512.0f, 1.0f / 256.0f);
		glPopMatrix();
		// videos
		glBindTexture(GL_TEXTURE_2D, textureIdWater);
		theoraplayer::VideoFrame* frame = clipWater->fetchNextFrame();
		if (frame != NULL)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->getWidth(), frame->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, frame->getBuffer());
			clipWater->popFrame();
		}
		drawTexturedQuad(textureIdWater, 0.0f, 768.0f - 176.0f, 1024.0f, 176.0f, 1.0f, 176.0f / 256.0f);
		glBindTexture(GL_TEXTURE_2D, textureIdEve);
		frame = clipEve->fetchNextFrame();
		if (frame != NULL)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->getWidth(), frame->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, frame->getBuffer());
			clipEve->popFrame();
		}
		drawTexturedQuad(textureIdEve, 120.0f, 310.0f, 256.0f, 336.0f, 1.0f, 336.0f / 512.0f);
		// overlays
		glBindTexture(GL_TEXTURE_2D, textureIdBush);
		drawTexturedQuad(textureIdBush, 0.0f, 511.0f, 513.0f, 256.0f, 511.0f / 512.0f, 1.0f, 0.0f, 1.0f / 256.0f);
	}

	Demo demo = { init, destroy, update, draw, NULL, NULL, NULL };

}
#endif
