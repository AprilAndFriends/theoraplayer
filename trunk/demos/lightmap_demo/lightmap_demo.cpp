/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

/************************************************************************************
COPYRIGHT INFO: The room 3D models and lightmap textures and textures are licensed
                under the terms of the GNU General Public License (GPL).
*************************************************************************************/
#define __3D_PROJECTION
#define __ZBUFFER
#include "demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/TheoraDataSource.h>
#include "ObjModel.h"
#include "tga.h"
#include <math.h>

unsigned int tex_id, diffuse_map;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="lightmap_demo";
bool started=1;
int window_w=1024,window_h=768;

ObjModel room;
float anglex=0,angley=0;

void multiTexFunc(float u, float v)
{
	glMultiTexCoord2fARB(GL_TEXTURE0_ARB, u, v);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB, u, v);
}

void draw()
{
	glClearColor(27/255.0f, 188/255.0f, 224/255.0f, 1);
	glBindTexture(GL_TEXTURE_2D,tex_id);

	glLoadIdentity();
	gluLookAt(sin(anglex)*100-100,angley+50,cos(anglex)*100-100,  -100,50,-100,  0,1,0);

	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
	}

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, diffuse_map);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_2D, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glEnable(GL_CULL_FACE);
	room.draw(multiTexFunc);
	glDisable(GL_CULL_FACE);
}

void update(float time_increase)
{
	float x,y;
	getCursorPos(&x,&y);
	anglex=-4*3.14f*x/window_w;
	angley=1500*(y-300)/window_h;

	mgr->update(time_increase);
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
	mgr=new TheoraVideoManager();
	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("media/lightmap/lightmap.ogg"), TH_RGB);
	clip->setAutoRestart(1);
	clip->setPlaybackSpeed(0.5f);
	
	tex_id = createTexture(nextPow2(clip->getWidth()),nextPow2(clip->getHeight()));
	diffuse_map = loadTexture("media/lightmap/diffuse_map.tga");
	
	room.load("media/lightmap/room.obj", tex_id);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_COLOR_MATERIAL);
	getMultiTextureExtensionFuncPointers();
}

void destroy()
{
	delete mgr;
}
