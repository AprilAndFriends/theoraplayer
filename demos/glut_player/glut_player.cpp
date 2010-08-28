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

unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="glut_player";
bool started=1;
int window_w=800,window_h=600;

void draw()
{
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
	drawTexturedQuad(0,0,800,600,w/tw,h/th);
	if (shader_on) disable_shader();

	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(0,570,800,30,0,0,0,1);
	drawWiredQuad(0,570,800,30,1,1,1,1);

	float x=clip->getTimePosition()/clip->getDuration();
	drawColoredQuad(3,573,794*x,24,1,1,1,1);
}

void update(float time_increase)
{
	if (started)
	{
		// let's wait until the system caches up a few frames on startup
		if (clip->getNumReadyFrames() < clip->getNumPrecachedFrames()*0.5f)
			return;
		started=0;
	}
	mgr->update(time_increase);
}

void OnKeyPress(int key)
{
	if (key == ' ')
    { if (clip->isPaused()) clip->play(); else clip->pause(); }
	if (key == 114)
	{
		clip->restart();
		clip->pause();
		clip->play();
	}

	if (key == 5) clip->setOutputMode(TH_RGB);
	if (key == 6) clip->setOutputMode(TH_YUV);
	if (key == 7) clip->setOutputMode(TH_GREY3);
}

void OnClick(float x,float y)
{
	if (y > 570)
		clip->seek((x/window_w)*clip->getDuration());
}

void setDebugTitle(char* out)
{
	int nDropped=clip->getNumDroppedFrames(),nDisplayed=clip->getNumDisplayedFrames();
	float percent=100*((float) nDropped/nDisplayed);
	sprintf(out," (%dx%d) %d precached, %d displayed, %d dropped (%.1f %%)",clip->getWidth(),
		                                                                    clip->getHeight(),
		                                                                    clip->getNumReadyFrames(),
		                                                                    nDisplayed,
		                                                                    nDropped,
			                                                                percent);
}

void init()
{
	mgr=new TheoraVideoManager();
	clip=mgr->createVideoClip("media/short.ogg",TH_RGB);
//  use this if you want to preload the file into ram and stream from there
//	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("../media/short.ogg"),TH_RGB);
	clip->setAutoRestart(1);

	tex_id=createTexture(nextPow2(clip->getWidth()),nextPow2(clip->getHeight()));
}

void destroy()
{
	delete mgr;
}
