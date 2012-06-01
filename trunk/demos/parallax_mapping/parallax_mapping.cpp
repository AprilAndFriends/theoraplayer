/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
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
TheoraVideoClip *light[3];
std::string window_name="parallax_mapping";
bool started=1, textures_enabled = 1;
int window_w=1024,window_h=768;

float anglex=0,angley=0;

void draw()
{
	glClearColor(27/255.0f, 188/255.0f, 224/255.0f, 1);
//	glBindTexture(GL_TEXTURE_2D,tex_id);

	glLoadIdentity();
	gluLookAt(sin(anglex)*20,0,cos(anglex)*20, 0, 0, 0,  0,1,0);

	float v[] = { -1, 1, 0,  1, 1, 0,  1, -1, 0, -1, -1, 0 };
	
	glScalef(5, 5, 5);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 3 * sizeof(float), v);
	glDrawArrays(GL_QUADS, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	
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
	if (key == ' ')
		textures_enabled = !textures_enabled;
}

void OnClick(float x,float y)
{

}

void setDebugTitle(char* out)
{
	sprintf(out, "press space to toggle diffuse texture)");
}

void init()
{
	mgr=new TheoraVideoManager(3);
	//glDisable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//glEnable(GL_COLOR_MATERIAL);
	getMultiTextureExtensionFuncPointers();
}

void destroy()
{
	delete mgr;
}
