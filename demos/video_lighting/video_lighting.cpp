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
std::string window_name="video_lighting";
bool started=1;
int window_w=800,window_h=600;

ObjModel room;
float anglex=0,angley=0;

void draw()
{
	glClearColor(1, 1, 1, 1);

	glLoadIdentity();
	float x1=-224.299f, y1=206.815f, z1=31.883f, x2=-65.147f, y2=80.219f, z2=12.301f;
	gluLookAt(x1, z1, -y1, x2, z2, -y2,  0,1,0);

	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_ONE,GL_ZERO);
	glEnable(GL_CULL_FACE);
	room.draw();
	glDisable(GL_CULL_FACE);
	
	glBindTexture(GL_TEXTURE_2D,tex_id);
	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
	}
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	glDisable(GL_DEPTH_TEST);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	
	glBegin(GL_QUADS);

	glTexCoord2f(0,           0);           glVertex3f(-1, 1, 0);
	glTexCoord2f(800/1024.0f, 0);           glVertex3f( 1, 1, 0);
	glTexCoord2f(800/1024.0f, 600/1024.0f); glVertex3f( 1,-1, 0);
	glTexCoord2f(0,           600/1024.0f); glVertex3f(-1,-1, 0);

	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
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
	FOVY = 54.495f;
	mgr=new TheoraVideoManager();
	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("media/lighting/lighting.ogg"), TH_RGB);
	clip->setAutoRestart(1);
	clip->setPlaybackSpeed(0.5f);
	
	tex_id = createTexture(nextPow2(clip->getWidth()),nextPow2(clip->getHeight()));
	diffuse_map = loadTexture("media/lighting/diffuse_map.tga");
	
	room.load("media/lighting/room.obj", diffuse_map);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LESS);
	glEnable(GL_COLOR_MATERIAL);
}

void destroy()
{
	delete mgr;
}
