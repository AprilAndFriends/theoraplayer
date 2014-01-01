/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

/************************************************************************************
COPYRIGHT INFO: Sprite animation has been borrowed with authors permission from the game:
Game:   Kaptain Brawe ( http://kaptainbrawe.cateia.com/ )
Author: Petar Ivancek

These animations ARE NOT ALLOWED to be used in any manner other then for the purpose
of this demo program.
*************************************************************************************/
#include "demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/TheoraDataSource.h>

unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clips[8];
std::string window_name="sprite_animation";
bool started=1;
int window_w=800,window_h=600,cClip=0;
unsigned char buffer[203*300*4];

void draw()
{
	glBindTexture(GL_TEXTURE_2D,tex_id);

	TheoraVideoFrame* f=clips[cClip]->getNextFrame();
	if (f)
	{
		unsigned char* src=f->getBuffer();
		int i,j,k,x,y,w=f->getWidth();
		for (y=0;y<300;y++)
		{
			for (x=0;x<203;x++)
			{
				i=(y*203+x)*4;
				j=((y+2)*w+x+4)*3;
				k=((y+2)*w+x+205+4)*3;
				buffer[i  ]=src[j];
				buffer[i+1]=src[j+1];
				buffer[i+2]=src[j+2];
				buffer[i+3]=src[k];
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,203,300,GL_RGBA,GL_UNSIGNED_BYTE,buffer);
		clips[cClip]->popFrame();
	}

	glEnable(GL_TEXTURE_2D);
	drawTexturedQuad(tex_id,298,150,203,300,203.0f/256.0f,300.0f/512.0f);
}

void update(float time_increase)
{
	int newindex=-1;
	float x,y;
	getCursorPos(&x,&y);
	if (x >= 400 && y <= 300)
	{
		if      (x-400 < (300-y)/2) newindex=0;
		else if (x-400 > (300-y)*2) newindex=2;
		else                        newindex=1;
	}
	else if (x < 400 && y <= 300)
	{
		if      (400-x < (300-y)/2) newindex=0;
		else if (400-x > (300-y)*2) newindex=6;
		else                        newindex=7;
	}
	else if (x >= 400 && y > 300)
	{
		if      (x-400 < (y-300)/2) newindex=4;
		else if (x-400 > (y-300)*2) newindex=2;
		else                        newindex=3;
	}
	else if (x < 400 && y > 300)
	{
		if      (400-x < (y-300)/2) newindex=4;
		else if (400-x > (y-300)*2) newindex=6;
		else                        newindex=5;
	}
	if (newindex >= 0)
	{
		clips[cClip]->pause();
		cClip=newindex;
		clips[cClip]->play();
	}
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
	char temp[32];
	for (int i=0;i<8;i++)
	{
		sprintf(temp,"%d/%d  ",clips[i]->getNumReadyFrames(),clips[i]->getNumPrecachedFrames());
		strcat(out,temp);
	}
	strcat(out,temp);
	strcat(out," (Kaptain Brawe)");
}

void init()
{
	mgr=new TheoraVideoManager();
	std::string orientations[]={"N","NE","E","SE","S","SW","W","NW"};
	for (int i=0;i<8;i++)
	{
		// Note - this demo isn't using TH_RGBA for now since the frames in this video are not mod 16 aligned.
		clips[i]=mgr->createVideoClip(new TheoraMemoryFileDataSource("media/brawe/brawe_"+orientations[i]+"" + resourceExtension),TH_RGB,8);
		clips[i]->setAutoRestart(1);
		if (i != 0) clips[i]->pause();
	}

	tex_id=createTexture(256,512,GL_RGBA);

	glClearColor(0,0.5f,0,1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void destroy()
{
	delete mgr;
}
