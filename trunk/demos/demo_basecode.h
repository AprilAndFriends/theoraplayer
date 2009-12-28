/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2009 Kresimir Spes (kreso@cateia.com)

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the 
Free Software Foundation; either version 2 of the License, or (at your option) 
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
*************************************************************************************/
#include <windows.h> // todo: make platform independent
#include <stdio.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glut.h>

#include <string>
#pragma warning( disable: 4996 ) // MSVC++
extern std::string window_name;
extern int window_w,window_h;

bool shader_on=0;
#define USE_SHADERS
#ifdef USE_SHADERS
#include <gl\glext.h>
PFNGLCREATEPROGRAMPROC glCreateProgram=0;
PFNGLCREATESHADERPROC glCreateShader=0;
PFNGLLINKPROGRAMPROC glLinkProgram=0;
PFNGLSHADERSOURCEPROC glShaderSource=0;
PFNGLUSEPROGRAMPROC glUseProgram=0;
PFNGLCOMPILESHADERPROC glCompileShader=0;
PFNGLATTACHSHADERPROC glAttachShader=0;
unsigned int program,shader;
#endif



void init();
void destroy();
void update(float);
void draw();
void setDebugTitle(char* out);
void OnKeyPress(int key);
void OnClick(float x,float y);

void psleep(int milliseconds)
{
	Sleep(milliseconds);
}

int nextPow2(int x)
{
	int y;
	for (y=1;y<x;y*=2);
	return y;
}

void drawColoredQuad(float x,float y,float w,float h,float r,float g,float b,float a)
{
	glColor4f(r,g,b,a);
	glBegin (GL_QUADS);
	glVertex3f(x,  y,  0.0f);
	glVertex3f(x+w,y,  0.0f);
	glVertex3f(x+w,y+h,0.0f);
	glVertex3f(x,  y+h,0.0f);
	glEnd();
	glColor4f(1,1,1,1);
}

void drawWiredQuad(float x,float y,float w,float h,float r,float g,float b,float a)
{
	glColor4f(r,g,b,a);
	glBegin (GL_LINE_LOOP);
	glVertex3f(x,  y,  0.0f);
	glVertex3f(x+w,y,  0.0f);
	glVertex3f(x+w,y+h,0.0f);
	glVertex3f(x,  y+h,0.0f);
	glEnd();
	glColor4f(1,1,1,1);
}

void drawTexturedQuad(float x,float y,float w,float h,float sw,float sh)
{
	glBegin (GL_QUADS);
	glTexCoord2f(0,  0); glVertex3f(x,  y,  0.0f);
	glTexCoord2f(sw, 0); glVertex3f(x+w,y,  0.0f);
	glTexCoord2f(sw,sh); glVertex3f(x+w,y+h,0.0f);
	glTexCoord2f(0, sh); glVertex3f(x,  y+h,0.0f);
	glEnd();
}

void toggle_YUV2RGB_shader()
{
#ifdef USE_SHADERS
	if (!glCreateProgram)
	{
		if (!strstr((char*) glGetString(GL_EXTENSIONS),"GL_ARB_fragment_shader"))
		{
			printf("Unable to turn on yuv2rgb shader, your OpenGL driver doesn't support GLSL shaders!\n");
			return;
		}
		glCreateProgram=(PFNGLCREATEPROGRAMPROC) wglGetProcAddress("glCreateProgram");
		glCreateShader = (PFNGLCREATESHADERPROC) wglGetProcAddress("glCreateShader");
		glLinkProgram=(PFNGLLINKPROGRAMPROC) wglGetProcAddress("glLinkProgram");
		glShaderSource=(PFNGLSHADERSOURCEPROC) wglGetProcAddress("glShaderSource");
		glUseProgram=(PFNGLUSEPROGRAMPROC) wglGetProcAddress("glUseProgram");
		glCompileShader=(PFNGLCOMPILESHADERPROC) wglGetProcAddress("glCompileShader");
		glAttachShader=(PFNGLATTACHSHADERPROC) wglGetProcAddress("glAttachShader");

		const char* 
		shader_code="uniform sampler2D diffuseMap;\
                     void main(void)\
                     {\
                         vec3 yuv = texture2D(diffuseMap, gl_TexCoord[0].st).xyz;\
                         float y,u,v,r,g,b;\
                         y=1.1643*(yuv.x-0.0625);\
                         u=yuv.y-0.5;\
                         v=yuv.z-0.5;\
                         r=y+1.5958*v;\
                         g=y-0.39173*u-0.81290*v;\
                         b=y+2.017*u;\
                         gl_FragColor = vec4(r,g,b,1.0);\
                     }";

		

		program = glCreateProgram();
		shader = glCreateShader(GL_FRAGMENT_SHADER);	
		glShaderSource(shader,1,&shader_code,NULL);
		glCompileShader(shader);
		glAttachShader(program,shader);
		glLinkProgram(program);

	}
	shader_on=!shader_on;
	
#endif
}

void enable_shader()
{
#ifdef USE_SHADERS
	glUseProgram(program);
#endif
}
void disable_shader()
{
#ifdef USE_SHADERS
	glUseProgram(0);
#endif
}



unsigned int createTexture(int w,int h)
{
	unsigned int tex_id;
	glGenTextures(1,&tex_id);
	glBindTexture(GL_TEXTURE_2D,tex_id);
	unsigned char* b=new unsigned char[w*h*3];
	memset(b,0,w*h*3);

	glTexImage2D(GL_TEXTURE_2D,0,3,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,b);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	delete b;
	return tex_id;
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	draw();

	glutSwapBuffers();

	static unsigned long time=GetTickCount();
	unsigned long t=GetTickCount();

	float diff=(t-time)/1000.0f;
	if (diff > 0.25f)
		diff=0.05f; // prevent spikes (usually happen on app load)
	update(diff);

	static unsigned long fps_timer=time,fps_counter=0;
	if (t-fps_timer >= 250)
	{
		char title[512],debug[256]="";
		
		setDebugTitle(debug);
		sprintf(title,"%s: %d FPS; %s",window_name.c_str(),fps_counter*4,debug);
		glutSetWindowTitle(title);
		fps_counter=0;
		fps_timer=t;
	}
	else fps_counter++;

	time=t;

}

void reshape(int w,int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glViewport(0, 0, w, h);

	gluOrtho2D(0,800,600,0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void keyboard(unsigned char key,int x,int y)
{
    if (key == 27) // esc
        throw "Exit Requested";
    else OnKeyPress(key);
}

void keyboard_special(int key,int x,int y)
{
	if (key == 10) toggle_YUV2RGB_shader(); // F10
    OnKeyPress(key);
}

void mouse(int button,int state, int x, int y)
{
	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON)
	{
		float mx=(float) x/glutGet(GLUT_WINDOW_WIDTH);
		float my=(float) y/glutGet(GLUT_WINDOW_HEIGHT);
		OnClick(mx*window_w,my*window_h);
	}
}


void main(int argc,char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode( GLUT_DOUBLE|GLUT_RGBA);
	//glutInitWindowPosition(0,0);
	glutInitWindowSize(window_w,window_h);
	glutCreateWindow(window_name.c_str());
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutSpecialFunc(keyboard_special);
	glutIdleFunc(display);
	try { glutMainLoop(); }
	catch (void*) {}

	destroy();
}




