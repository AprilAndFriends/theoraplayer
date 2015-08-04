/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

/************************************************************************************
COPYRIGHT INFO: Graphics in media/locv have been borrowed
			 with authors permission from the game:
Game:   Legend of Crystal Valley ( http://locv.cateia.com/ )
Author: Cateia Games

These grapchics ARE NOT ALLOWED to be used in any manner other then for the purpose
of this demo program.
*************************************************************************************/
#include <math.h>
#include "demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/TheoraDataSource.h>
#include "tga.h"

unsigned int locv_main_tex, locv_back_tex, locv_branch_tex, locv_birds_tex,
             locv_bush_tex, locv_clouds_tex, water_tex, eve_tex;
TheoraVideoManager* mgr;
TheoraVideoClip *water, *eve;
std::string window_name="composite_player";
int window_w = 1024, window_h = 768;
unsigned char buffer[256 * 336 * 4];
float timer = 0;

void draw()
{
	glBindTexture(GL_TEXTURE_2D, locv_back_tex);
	drawTexturedQuad(locv_back_tex, 0, 0, 1024, 512, 1, 1);

	glBindTexture(GL_TEXTURE_2D, locv_clouds_tex);
	drawTexturedQuad(locv_clouds_tex, 0, 200, 1024, 350, 1, 0.6f, timer / 200.0f, 0.4f);

	glBindTexture(GL_TEXTURE_2D, locv_birds_tex);
	drawTexturedQuad(locv_birds_tex, ((int) (timer * 30)) % 800 + 300, 50 + sin(timer) * 5, 64, 32, 1, 1);

	glBindTexture(GL_TEXTURE_2D, locv_main_tex);
	drawTexturedQuad(locv_main_tex, 0, 0, 1024, 768, 1, 0.75f);
	
	glBindTexture(GL_TEXTURE_2D, locv_branch_tex);
	glPushMatrix();
	glTranslatef(1034, -10, 0);
	glRotatef((float) sin(timer) * 1, 0, 0, 1);
	drawTexturedQuad(locv_branch_tex, -510, 0, 510, 254, 510/512.0f, 254/256.0f, 1/512.0f, 1/256.0f);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, water_tex);    
	TheoraVideoFrame* f = water->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, f->getWidth(), f->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, f->getBuffer());
		water->popFrame();
	}
	drawTexturedQuad(water_tex, 0, 768 - 176, 1024, 176, 1.0f, 176.0f / 256.0f);

	glBindTexture(GL_TEXTURE_2D, eve_tex);    
	f = eve->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, f->getWidth(), f->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, f->getBuffer());
		eve->popFrame();
	}
	drawTexturedQuad(eve_tex, 120, 310, 256, 336, 1, 336.0f / 512.0f);
	
	glBindTexture(GL_TEXTURE_2D, locv_bush_tex);
	drawTexturedQuad(locv_bush_tex, 0, 511, 513, 256, 511/512.0f, 1, 0, 1/256.0f);

}

void update(float time_increase)
{
	mgr->update(time_increase);
	timer += time_increase;
}

void OnKeyPress(int key)
{

}

void OnClick(float x,float y)
{

}

void setDebugTitle(char* out)
{
	sprintf(out, "");
}

void init()
{
	mgr = new TheoraVideoManager(2);
	water = mgr->createVideoClip(new TheoraMemoryFileDataSource("media/locv/locv_water" + resourceExtension), TH_RGB, 4);
    water->setPlaybackSpeed(0.5f);
	water->setAutoRestart(1);

	eve = mgr->createVideoClip(new TheoraMemoryFileDataSource("media/locv/locv_eve" + resourceExtension), TH_RGBA, 4);
	eve->setAutoRestart(1);
    
	water_tex = createTexture(nextPow2(water->getWidth()), nextPow2(water->getHeight()));
	eve_tex = createTexture(nextPow2(eve->getWidth()), nextPow2(eve->getHeight()), GL_RGBA);
	locv_main_tex = loadTexture("media/locv/locv_main.tga");
	locv_back_tex = loadTexture("media/locv/locv_back.tga");
	locv_branch_tex = loadTexture("media/locv/locv_branch.tga");
	locv_bush_tex = loadTexture("media/locv/locv_bush.tga");
	locv_clouds_tex = loadTexture("media/locv/locv_clouds.tga");
	locv_birds_tex = loadTexture("media/locv/locv_birds.tga");
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void destroy()
{
	delete mgr;
}
