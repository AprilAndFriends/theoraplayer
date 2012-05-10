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
std::string window_name="composite_player";
bool started = 1;
int window_w = 1024, window_h = 768;

void draw()
{
	glBindTexture(GL_TEXTURE_2D,tex_id);
    
	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		//printf("Displaying frame %d\n", f->getFrameNumber());
		clip->popFrame();
	}
    
	float w=clip->getWidth(),h=clip->getHeight();
	float tw=nextPow2(w),th=nextPow2(h);
    
	glEnable(GL_TEXTURE_2D);
	drawTexturedQuad(0, 768 - 176, 1024, 176, w/tw, h/th);
}

void update(float time_increase)
{
	if (started)
	{
		if (clip->getNumReadyFrames() < clip->getNumPrecachedFrames()*0.5f)
			return;
		started=0;
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
	clip=mgr->createVideoClip("media/locv/locv_water.ogg", TH_RGB, 4);
    clip->setPlaybackSpeed(0.5f);
	clip->setAutoRestart(1);
    
	tex_id=createTexture(nextPow2(clip->getWidth()), nextPow2(clip->getHeight()));
}

void destroy()
{
	delete mgr;
}
