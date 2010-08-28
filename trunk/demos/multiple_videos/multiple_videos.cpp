/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "../demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/TheoraDataSource.h>

TheoraVideoClip* clips[4];
unsigned int textures[4];
TheoraVideoManager* mgr;
std::string window_name="multiple_videos";
int window_w=1024,window_h=768;

void drawVideo(int x,int y,unsigned int tex_id,TheoraVideoClip* clip)
{
	glLoadIdentity();
	glTranslatef(x,y,0);
	glBindTexture(GL_TEXTURE_2D,tex_id);

	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
	}
	
	float w=clip->getWidth(),h=clip->getHeight();
	float tw=nextPow2(w),th=nextPow2(h);
	
	glEnable(GL_TEXTURE_2D);
	if (shader_on) enable_shader();
	drawTexturedQuad(0,0,395,295,w/tw,h/th);
	if (shader_on) disable_shader();

	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(0,570/2,395,30/2,0,0,0,1);
	drawWiredQuad(0,570/2,395,30/2,1,1,1,1);

	float p=clip->getTimePosition()/clip->getDuration();
	drawColoredQuad(1.5f,573/2,784*p/2,24/2,1,1,1,1);
}

void draw()
{
	drawVideo(0  ,0  ,textures[0],clips[0]);
	drawVideo(400,0  ,textures[1],clips[1]);
	drawVideo(0  ,300,textures[2],clips[2]);
	drawVideo(400,300,textures[3],clips[3]);
}

void update(float time_increase)
{
	mgr->update(time_increase);
}

void setDebugTitle(char* out)
{
	char temp[32];
	for (int i=0;i<4;i++)
	{
		sprintf(temp,"%d/%d  ",clips[i]->getNumReadyFrames(),clips[i]->getNumPrecachedFrames());
		strcat(out,temp);
	}
	sprintf(temp,"(%d worker threads)",mgr->getNumWorkerThreads());
	strcat(out,temp);
}

void playPause(int index)
{
	if (clips[index]->isPaused()) clips[index]->play();
	else                          clips[index]->pause();
}

void OnKeyPress(int key)
{
	if (key == '1') mgr->setNumWorkerThreads(1);
	if (key == '2') mgr->setNumWorkerThreads(2);
	if (key == '3') mgr->setNumWorkerThreads(3);
	if (key == '4') mgr->setNumWorkerThreads(4);
	if (key >= 1 && key <= 4) playPause(key-1); // Function keys are used for play/pause

	if (key == 5 || key == 6 || key == 7)
	{
		TheoraOutputMode mode;
		if (key == 5) mode=TH_RGB;
		if (key == 6) mode=TH_YUV;
		if (key == 7) mode=TH_GREY3;

		for (int i=0;i<4;i++) clips[i]->setOutputMode(mode);
	}
}

void OnClick(float x,float y)
{

}


void init()
{
	printf("---\nUSAGE: press buttons 1,2,3 or 4 to change the number of worker threads\n---\n");

	std::string files[]={"short.ogg","konqi.ogg","room.ogg","titan.ogg"};
	mgr=new TheoraVideoManager(1);
	mgr->setDefaultNumPrecachedFrames(32);
	for (int i=0;i<4;i++)
	{
		clips[i]=mgr->createVideoClip(new TheoraMemoryFileDataSource("media/"+files[i]));
		clips[i]->setAutoRestart(1);
		textures[i]=createTexture(nextPow2(clips[i]->getWidth()),nextPow2(clips[i]->getHeight()));
	}
}

void destroy()
{
	delete mgr;
}
