/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _DEMO_BASECODE_H
#define _DEMO_BASECODE_H

#include <string.h>
#include <stdio.h>
#include <string>

#ifdef WIN32
	#pragma warning( disable: 4996 ) // MSVC++
	#include <windows.h>
	#include <GL/gl.h>
	#include <GL/glut.h>
#endif
#ifdef __APPLE__
	#include "objcUtil.h"
	#ifdef _IOS
		#import <OpenGLES/ES1/gl.h>
		#import <OpenGLES/ES1/glext.h>
	#else
		#include <OpenGL/gl.h>
		#include <OpenGL/glext.h>
		#include <GLUT/glut.h>
	#endif
#endif
#ifdef _LINUX
	#include <GL/gl.h>
	#include <GL/glut.h>
#endif

#ifdef _WIN32
#include <GL/glext.h>
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
#endif

#ifdef _LINUX
#include <GL/glext.h>
#endif

#include "util.h"
#include "theoraplayer/TheoraVideoManager.h"

extern std::string resourceExtension;
extern float FOVY;
extern bool shader_on;

// functions defined in demos
// --------------------------
extern std::string window_name;
extern int window_w, window_h;
void init();
void destroy();
void update(float);
void draw();
void setDebugTitle(char* out);
void OnKeyPress(int key);
void OnClick(float x,float y);
// --------------------------
void psleep(int milliseconds);
int nextPow2(int x);
void drawColoredQuad(float x,float y,float w,float h,float r,float g,float b,float a);
void drawWiredQuad(float x,float y,float w,float h,float r,float g,float b,float a);
void drawTexturedQuad(unsigned int texID, float x,float y,float w,float h,float sw,float sh,float sx=0,float sy=0);
void toggle_YUV2RGB_shader();
void getMultiTextureExtensionFuncPointers();
void enable_shader();
void disable_shader();

unsigned int createTexture(int w,int h,unsigned int format=GL_RGB);

void getCursorPos(float* xout,float* yout);

#endif
