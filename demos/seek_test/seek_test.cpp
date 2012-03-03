/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "../demo_basecode.h"
#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/TheoraDataSource.h>

unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="seek_test";
bool started=1, needsSeek = 1;
int cFrame = 0;
int window_w=800,window_h=600;

void draw()
{
	glBindTexture(GL_TEXTURE_2D,tex_id);

	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
		needsSeek = 1;
		printf("Displayed frame %d\n", f->getFrameNumber());
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
	mgr->update(time_increase/3);
	if (needsSeek)
	{
		clip->seek((float) cFrame / clip->getFPS());
		needsSeek = 0;
		cFrame++;
	}
}

void OnKeyPress(int key)
{

}

void OnClick(float x,float y)
{

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
	clip=mgr->createVideoClip("media/bunny.ogg", TH_RGB, 4);
	clip->setAutoRestart(1);

	tex_id=createTexture(nextPow2(clip->getWidth()), nextPow2(clip->getHeight()));
}

void destroy()
{
	delete mgr;
}
