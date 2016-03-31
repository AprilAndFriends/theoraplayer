/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Contains some basic definitions that are required for all OpenGL related code to work.

#ifndef THEORAPLAYER_DEMOS_DEMO_BASECODE_H
#define THEORAPLAYER_DEMOS_DEMO_BASECODE_H

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

extern int windowWidth;
extern int windowHeight;
extern std::string window_name;

struct Demo
{
	void(*init)();
	void(*destroy)();
	void(*update)(float);
	void(*draw)();
	void(*setDebugTitle)(char*);
	void(*onKeyPress)(int);
	void(*onClick)(float, float);
};

void changeDemo(Demo* demo);

// functions defined in demos
void init();
void destroy();
void update(float);
void draw();
void setDebugTitle(char* newTitle);
void onKeyPress(int key);
void onClick(float x, float y);

void drawColoredQuad(float x, float y, float w, float h, float r, float g, float b, float a);
void drawWiredQuad(float x, float y, float w, float h, float r, float g, float b, float a);
void drawTexturedQuad(unsigned int textureId, float x, float y, float w, float h, float sw, float sh, float sx = 0.0f, float sy = 0.0f);
void toggle_YUV2RGB_shader();
void getMultiTextureExtensionFuncPointers();
void enableShader();
void disableShader();

void getCursorPosition(float& outX, float& outY);

#endif
