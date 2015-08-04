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
#ifdef WIN32
#include <gl/glext.h>
#endif

unsigned int tex_id, diffuse_map;
TheoraVideoManager* mgr;
TheoraVideoClip *light[3];
std::string window_name="lightmap_demo";
bool started=1, textures_enabled = 1;
int window_w=1024,window_h=768;

ObjModel room;
float anglex=0,angley=0;
unsigned char *light_data[3], *tex_data;
bool update_tex = 0;
bool light_switch[3] = {1, 0, 0};
float light_scale[3][3] = { {1, 1, 1}, {2, 0, 0}, {0, 2, 0}  };
int framew, frameh;

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

	TheoraVideoFrame* f;
	for (int i = 0; i < 3; i++)
	{
		f = light[i]->getNextFrame();
		if (f)
		{
			memcpy(light_data[i], f->getBuffer(), framew * frameh * 3);
			light[i]->popFrame();
			update_tex = 1;
		}
	}
	
	if (update_tex)
	{
		int i, x, y;
		memset(tex_data, light_switch[0] == 0 && light_switch[1] == 0 && light_switch[2] == 0 ? 255 : 0, framew * frameh * 3);
		unsigned char *ptr[3] = { light_data[0], light_data[1], light_data[2] };
		unsigned char *tex;
		unsigned int r, g, b;
		for (i = 0; i < 3; i++)
		{
			if (light_switch[i] == 0) continue;
			tex = tex_data;
			for (y = 0; y < frameh; y++)
			{
				for (x = 0; x < framew; x++)
				{
					r = tex[0] + ptr[i][0] * light_scale[i][0];
					g = tex[1] + ptr[i][1] * light_scale[i][1];
					b = tex[2] + ptr[i][2] * light_scale[i][2];
					tex[0] = r > 255 ? 255 : r;
					tex[1] = g > 255 ? 255 : g;
					tex[2] = b > 255 ? 255 : b;
					tex += 3;
					ptr[i] += 3;
				}	
			}
		}
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framew, frameh, GL_RGB, GL_UNSIGNED_BYTE, tex_data);
		update_tex = 0;
	}
	
	glActiveTextureARB(GL_TEXTURE0_ARB);
	if (textures_enabled)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
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
	if (key == '1' || key == '2' || key == '3')
	{
		int i = key - '1';
		light_switch[i] = !light_switch[i];
		if (!light_switch[i]) light[i]->pause();
		else light[i]->play();
		update_tex = 1;
	}
	if (key == ' ')
		textures_enabled = !textures_enabled;
}

void OnClick(float x,float y)
{

}

void setDebugTitle(char* out)
{
	sprintf(out, "lights: %d, %d, %d (press keys 1,2,3 to toggle lights, space to toggle diffuse texture)", (int) light_switch[0], (int) light_switch[1], (int) light_switch[2]);
}

void init()
{
	mgr=new TheoraVideoManager(3);
	light[0] = mgr->createVideoClip(new TheoraMemoryFileDataSource("media/lightmap/light1" + resourceExtension), TH_RGB);
	light[1] = mgr->createVideoClip(new TheoraMemoryFileDataSource("media/lightmap/light2" + resourceExtension), TH_RGB);
	light[2] = mgr->createVideoClip(new TheoraMemoryFileDataSource("media/lightmap/light3" + resourceExtension), TH_RGB);
	light[0]->setAutoRestart(1); light[0]->setPlaybackSpeed(1.0f);
	light[1]->setAutoRestart(1); light[1]->setPlaybackSpeed(0.935f);
	light[2]->setAutoRestart(1); light[2]->setPlaybackSpeed(0.876f);
	if (!light_switch[0]) light[0]->pause();
	if (!light_switch[1]) light[1]->pause();
	if (!light_switch[2]) light[2]->pause();
	
	
	framew = light[0]->getWidth();
	frameh = light[0]->getHeight();
	int size = framew * frameh * 3;
	light_data[0] = new unsigned char[size];
	light_data[1] = new unsigned char[size];
	light_data[2] = new unsigned char[size];
	tex_data = new unsigned char[size];

	tex_id = createTexture(nextPow2(framew), nextPow2(frameh));
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
	delete [] light_data[0];
	delete [] light_data[1];
	delete [] light_data[2];
	delete [] tex_data;
	delete mgr;
}
