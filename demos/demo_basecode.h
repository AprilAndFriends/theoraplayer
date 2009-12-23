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
extern std::string window_name;

void init();
void destroy();
void update(float);
void draw();
void setDebugTitle(char* out);


int nextPow2(int x)
{
	int y;
	for (y=1;y<x;y*=2);
	return y;
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

	update((t-time)/1000.0f);

	static unsigned long fps_timer=time,fps_counter=0;
	if (t-fps_timer >= 1000)
	{
		char title[512],debug[256]="";
		
		setDebugTitle(debug);
		sprintf(title,"%s: %d FPS; %s",window_name.c_str(),fps_counter,debug);
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

	gluOrtho2D(-1,1,1,-1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void keyboard(unsigned char key,int x,int y)
{
  if (key == 27) // esc
      throw "Exit Requested";
}


void main(int argc,char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode( GLUT_DOUBLE|GLUT_RGBA);
	//glutInitWindowPosition(0,0);
	glutInitWindowSize(800,600);
	glutCreateWindow(window_name.c_str());
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(display);
	try { glutMainLoop(); }
	catch (void*) {}

	destroy();
}




