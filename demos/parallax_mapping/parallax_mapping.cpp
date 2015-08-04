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

#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
#endif

unsigned int diffuse_map, normal_map, height_map;
TheoraVideoManager* mgr;
std::string window_name="parallax_mapping";
int window_w=1024,window_h=768;
TheoraVideoClip* clip;

float anglex=0,angley=0;

unsigned int ppl_shader, nm_shader, parallax_shader, cur_shader;

void setupLighting()
{
    GLfloat DiffuseLight[] = {1, 1, 1};
    GLfloat AmbientLight[] = {0.3f, 0.3f, 0.3f};

    glLightfv (GL_LIGHT0, GL_DIFFUSE, DiffuseLight);
    glLightfv (GL_LIGHT0, GL_AMBIENT, AmbientLight);

	static float x = 0;
	x+= 0.01f;
    GLfloat LightPosition[] = { 0, (float) sin(x)*3, -5, 0};

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
	GLint shaderCompiled;
	
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &shaderCompiled);
	
	if(shaderCompiled == GL_FALSE)
	{
		int len = 0;
		
		int charsWritten = 0;
		char* infoLog;
		
		glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &len);
		
		if (len > 0)
		{
			infoLog = (char*) malloc(len);
			glGetShaderInfoLog(_shader, len, &charsWritten, infoLog);
			
			printf("%s\n", infoLog);
			
			free(infoLog);
			
		}
	}

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

	nm_shader = glCreateProgram();
	vert = setupShader("media/parallax/normal_mapping.vert", 1);
	frag = setupShader("media/parallax/normal_mapping.frag", 0);
	glAttachShader(nm_shader, vert);
	glAttachShader(nm_shader, frag);
	glLinkProgram(nm_shader);
	
	parallax_shader = glCreateProgram();
	vert = setupShader("media/parallax/parallax_mapping.vert", 1);
	frag = setupShader("media/parallax/parallax_mapping.frag", 0);
	glAttachShader(parallax_shader, vert);
	glAttachShader(parallax_shader, frag);
	glLinkProgram(parallax_shader);

	
	cur_shader = parallax_shader;

}

void draw()
{
	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		unsigned char buf[256 * 256 * 3];
		glBindTexture(GL_TEXTURE_2D,normal_map);
		unsigned char* buffer = f->getBuffer();
	
		for (int y = 0; y < 256; y++)
		{
			memcpy(&buf[y * 256 * 3], buffer + y * 512 * 3, 256 * 3);
		}
		
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,256, 256,GL_RGB,GL_UNSIGNED_BYTE, buf);
		
		glBindTexture(GL_TEXTURE_2D,height_map);

		for (int y = 0; y < 256; y++)
		{
			memcpy(&buf[y * 256 * 3], buffer + y * 512 * 3 + 768, 256 * 3);
		}
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,256, 256,GL_RGB,GL_UNSIGNED_BYTE, buf);

		clip->popFrame();
	}
	
	
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
	
	float tn[] = {  1,  0,  0,   1,  0,  0,   1,  0,  0,    1,  0,  0,
				   -1,  0,  0,  -1,  0,  0,  -1,  0,  0,   -1,  0,  0,
				    0,  0, -1,   0,  0, -1,   0,  0, -1,    0,  0, -1,
					0,  0,  1,   0,  0,  1,   0,  0,  1,    0,  0,  1,
					1,  0,  0,   1,  0,  0,   1,  0,  0,    1,  0,  0,
				   -1,  0,  0,  -1,  0,  0,  -1,  0,  0,   -1,  0,  0 };
	
	float n[] = {  0,  0, -1,   0,  0, -1,    0,  0, -1,    0,  0, -1,
                   0,  0,  1,   0,  0,  1,    0,  0,  1,    0,  0,  1,
				  -1,  0,  0,  -1,  0,  0,   -1,  0,  0,   -1,  0,  0,
	               1,  0,  0,   1,  0,  0,    1,  0,  0,    1,  0,  0,
				   0,  1,  0,   0,  1,  0,    0,  1,  0,    0,  1,  0,
				   0, -1,  0,   0, -1,  0,    0, -1,  0,    0, -1,  0 };
	
	float t[] = {      1,  0,       0,  0,        0,  1,        1,  1,
					   0,  0,       1,  0,        1,  1,        0,  1,
					   0,  0,       1,  0,        1,  1,        0,  1,
					   1,  0,       0,  0,        0,  1,        1,  1,
					   0,  0,       1,  0,        1,  1,        0,  1,
					   0,  0,       1,  0,        1,  1,        0,  1 };
	
	setupLighting();

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D, diffuse_map);
	glEnable(GL_TEXTURE_2D);

	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_2D, normal_map);
	glEnable(GL_TEXTURE_2D);
	
	
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glBindTexture(GL_TEXTURE_2D, height_map);
	glEnable(GL_TEXTURE_2D);

	glUseProgram(cur_shader);

	int texture0Location = glGetUniformLocation(cur_shader, "colorMap");
	glUniform1i(texture0Location, 0);
	
	int texture1Location = glGetUniformLocation(cur_shader, "normalMap");
	glUniform1i(texture1Location, 1);

	int texture2Location = glGetUniformLocation(cur_shader, "heightMap");
	glUniform1i(texture2Location, 2);

	glScalef(5, 5, 5);

	glVertexPointer(3, GL_FLOAT, 3 * sizeof(float), v);
	glNormalPointer(GL_FLOAT, 3 * sizeof(float), n);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(float), t);
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(3, GL_FLOAT, 3 * sizeof(float), tn);
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
	{
		cur_shader = cur_shader == ppl_shader ? nm_shader : ( cur_shader == nm_shader ? parallax_shader : ppl_shader);
	}
}

void OnClick(float x,float y)
{

}

void setDebugTitle(char* out)
{
	sprintf(out, "press space to toggle shaders)");
}

void init()
{
	mgr=new TheoraVideoManager(1);
	//glDisable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("media/parallax/parallax" + resourceExtension), TH_RGB);
	clip->setAutoRestart(1);

	diffuse_map = loadTexture("media/parallax/diffuse_map.tga");
	normal_map = createTexture(256,256);
	height_map = createTexture(256,256);
	

	//glEnable(GL_COLOR_MATERIAL);
	getMultiTextureExtensionFuncPointers();
	
	setupShaders();
}

void destroy()
{
	delete mgr;
}
