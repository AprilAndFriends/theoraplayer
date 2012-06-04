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

unsigned int ppl_shader;

void setupLighting(void)
{
    GLfloat DiffuseLight[] = {1, 1, 1};
    GLfloat AmbientLight[] = {0.3f, 0.3f, 0.3f};

    glLightfv (GL_LIGHT0, GL_DIFFUSE, DiffuseLight);
    glLightfv (GL_LIGHT0, GL_AMBIENT, AmbientLight);

    GLfloat LightPosition[] = {0, 0, -5, 0};

    glLightfv (GL_LIGHT0, GL_POSITION, LightPosition);
}

unsigned int setupShader(const char* filename, bool vertex_shader)
{
	unsigned int _shader;
	
	char src[20000] = {0};
	FILE* f = fopen(filename, "rb");
	fread(src, 1, 20000, f);
	fclose(f);
	const char* _src = src;
	
	_shader = glCreateShader(vertex_shader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
	glShaderSource(_shader, 1, &_src, NULL);
	glCompileShader(_shader);

	return _shader;
}

void setupShaders()
{
	unsigned int vert, frag;
	ppl_shader = glCreateProgram();

	vert = setupShader("media/parallax/per_pixel_lighting.vert", 1);
	frag = setupShader("media/parallax/per_pixel_lighting.frag", 0);
	glAttachShader(ppl_shader, vert);
	glAttachShader(ppl_shader, frag);
	glLinkProgram(ppl_shader);

}

void draw()
{
	glClearColor(27/255.0f, 188/255.0f, 224/255.0f, 1);
//	glBindTexture(GL_TEXTURE_2D,tex_id);

	glLoadIdentity();
	gluLookAt(sin(anglex)*20, angley, cos(anglex)*20, 0, 0, 0,  0,1,0);

	float v[] = { -1,  1, -1,   1,  1, -1,    1, -1, -1,   -1, -1, -1,
                  -1,  1,  1,   1,  1,  1,    1, -1,  1,   -1, -1,  1,
                  -1,  1, -1,  -1,  1,  1,   -1, -1,  1,   -1, -1, -1,
                   1,  1, -1,   1,  1,  1,    1, -1,  1,    1, -1, -1,
				  -1,  1, -1,   1,  1, -1,    1,  1,  1,   -1,  1,  1,
				  -1, -1, -1,   1, -1, -1,    1, -1,  1,   -1, -1,  1 };
	
	float n[] = {  0,  0, -1,   0,  0, -1,    0,  0, -1,    0,  0, -1,
                   0,  0,  1,   0,  0,  1,    0,  0,  1,    0,  0,  1,
				  -1,  0,  0,  -1,  0,  0,   -1,  0,  0,   -1,  0,  0,
	               1,  0,  0,   1,  0,  0,    1,  0,  0,    1,  0,  0,
				   0,  1,  0,   0,  1,  0,    0,  1,  0,    0,  1,  0,
				   0, -1,  0,   0, -1,  0,    0, -1,  0,    0, -1,  0 };
	
	float t[] = {      0,  0,       1,  0,        1,  1,        0,  1,
					   0,  0,       1,  0,        1,  1,        0,  1,
					   0,  0,       1,  0,        1,  1,        0,  1,
					   0,  0,       1,  0,        1,  1,        0,  1,
					   0,  0,       1,  0,        1,  1,        0,  1,
					   0,  0,       1,  0,        1,  1,        0,  1 };
	
	setupLighting();
	
	glBindTexture(GL_TEXTURE_2D, diffuse_map);
	glScalef(5, 5, 5);
	glUseProgram(ppl_shader);

	glVertexPointer(3, GL_FLOAT, 3 * sizeof(float), v);
	glNormalPointer(GL_FLOAT, 3 * sizeof(float), n);
	glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(float), t);
	glDrawArrays(GL_QUADS, 0, 24);
	
}

void update(float time_increase)
{
	float x,y;
	getCursorPos(&x,&y);
	anglex = -4 * 3.14f * x / window_w;
	angley = -100 * y / window_h + 50;

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
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	diffuse_map = loadTexture("media/parallax/diffuse_map.tga");
	//glEnable(GL_COLOR_MATERIAL);
	getMultiTextureExtensionFuncPointers();
	
#ifndef __APPLE__
	glCreateProgram=(PFNGLCREATEPROGRAMPROC) pglGetProcAddress("glCreateProgram");
	glCreateShader = (PFNGLCREATESHADERPROC) pglGetProcAddress("glCreateShader");
	glLinkProgram=(PFNGLLINKPROGRAMPROC) pglGetProcAddress("glLinkProgram");
	glShaderSource=(PFNGLSHADERSOURCEPROC) pglGetProcAddress("glShaderSource");
	glUseProgram=(PFNGLUSEPROGRAMPROC) pglGetProcAddress("glUseProgram");
	glCompileShader=(PFNGLCOMPILESHADERPROC) pglGetProcAddress("glCompileShader");
#endif
	
	setupShaders();
}

void destroy()
{
	delete mgr;
}
