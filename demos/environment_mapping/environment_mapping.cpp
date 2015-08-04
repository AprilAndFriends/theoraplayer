/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/TheoraDataSource.h>
#include "ObjModel.h"
#include "tga.h"
#include <math.h>

unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="environment_mapping";
bool started=1;
int window_w=800,window_h=600;
float angle = 0;
ObjModel teapot;

void draw()
{
	glBindTexture(GL_TEXTURE_2D,tex_id);
	
	glLoadIdentity();
	gluLookAt(0,2000,1000,  0,0,0,  0,-1,0);

	glRotatef(angle, 0, 0, 1);
	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
	}

	teapot.draw();
}

void update(float time_increase)
{
	angle += time_increase * 20;
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
	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("media/environment_mapping/room256" + resourceExtension), TH_RGB);
	clip->setAutoRestart(1);

	tex_id = createTexture(nextPow2(clip->getWidth()),nextPow2(clip->getHeight()));

	teapot.load("media/environment_mapping/teapot.obj", tex_id, true);

	glClearColor(1, 1, 1, 1);
	glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_COLOR_MATERIAL);
}

void destroy()
{
	delete mgr;
}
