/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

/************************************************************************************
COPYRIGHT INFO: The room 3D models and lightmap textures and textures are licensed
                under the terms of the GNU General Public License (GPL).
*************************************************************************************/
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
bool started = 1, diffuse_enabled = 1, lighting_enabled = 1;
int window_w=800,window_h=600;

struct xyz
{
	float x, y, z;
};
std::vector<xyz> camera_pos;

ObjModel room;
float anglex=0,angley=0;

void draw()
{
	glClearColor(1, 1, 1, 1);

	glLoadIdentity();
	float x1, y1, z1, x2=-65.147f, y2=80.219f, z2=12.301f;
	static int index = 0;

	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		index = (int)f->getFrameNumber();
		glBindTexture(GL_TEXTURE_2D,tex_id);
		unsigned char* buffer = f->getBuffer();
		int x, len = f->getWidth() * f->getHeight() * 3;
		for (int i = 0; i < len; i++)
		{
			x = (*buffer) * 0.8f + 255 * 0.2f;
			if (x > 255) *buffer = 255;
			else *buffer = x;
			buffer++;
		}
		
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,f->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
	}
	x1 = camera_pos[index].x; y1 = camera_pos[index].y; z1 = camera_pos[index].z;
	gluLookAt(x1, z1, -y1, x2, z2, -y2,  0,1,0);
	//gluLookAt(x1, y1, z1, x2, z2, -y2,  0,1,0);

	glBlendFunc(GL_ONE,GL_ZERO);
	glEnable(GL_CULL_FACE);
	if (diffuse_enabled) glEnable(GL_TEXTURE_2D);
	else glDisable(GL_TEXTURE_2D);
	room.draw();
	glDisable(GL_CULL_FACE);

	if (lighting_enabled)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,tex_id);

		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		
		glDisable(GL_DEPTH_TEST);

		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		
		glBegin(GL_QUADS);

		glTexCoord2f(            0,   4 / 1024.0f); glVertex3f(-1, 1, 0);
		glTexCoord2f(800 / 1024.0f,   4 / 1024.0f); glVertex3f( 1, 1, 0);
		glTexCoord2f(800 / 1024.0f, 604 / 1024.0f); glVertex3f( 1,-1, 0);
		glTexCoord2f(            0, 604 / 1024.0f); glVertex3f(-1,-1, 0);

		glEnd();

		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glEnable(GL_DEPTH_TEST);
	}
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
	if (key == ' ') diffuse_enabled = !diffuse_enabled;
	if (key == 13) lighting_enabled = !lighting_enabled; // 13 = ENTER key
}

void OnClick(float x, float y)
{

}

void setDebugTitle(char* out)
{
	sprintf(out, "press SPACE to toggle diffuse map, ENTER to toggle lighting");
}

void init()
{
	FILE* f = fopen("media/lighting/camera.txt", "r");

	xyz pos;
	while (!feof(f))
	{
		fscanf(f, "%f %f %f", &pos.x, &pos.y, &pos.z);
		camera_pos.push_back(pos);
	}

	fclose(f);
	
	FOVY = 54.495f;
	mgr=new TheoraVideoManager();
	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("media/lighting/lighting" + resourceExtension), TH_RGB);
	clip->setAutoRestart(1);
	//clip->setPlaybackSpeed(0.5f);
	
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
