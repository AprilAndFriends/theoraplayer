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

unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="seek_test";
bool started=1, needsSeek = 1;
int cFrame = 0, nWrongSeeks = 0;
int window_w=800,window_h=600;
float delay = 0;

void draw()
{
	glBindTexture(GL_TEXTURE_2D,tex_id);

	if (!needsSeek)
	{
		TheoraVideoFrame* f=clip->getNextFrame();
		if (f)
		{
			glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
			needsSeek = 1;
			if (f->getFrameNumber() != cFrame)
				nWrongSeeks++;
			cFrame++;
			if (cFrame >= clip->getNumFrames()) cFrame = 0;
			printf("Displayed frame %d\n", (int)f->getFrameNumber());
			clip->popFrame();
		}
	}


	float w=clip->getWidth(),h=clip->getHeight();
	float tw=nextPow2(w),th=nextPow2(h);

	glEnable(GL_TEXTURE_2D);
	if (shader_on) enable_shader();
	drawTexturedQuad(tex_id,0,0,800,600,w/tw,h/th);
	if (shader_on) disable_shader();

	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(0,570,800,30,0,0,0,1);
	drawWiredQuad(0,570,800,30,1,1,1,1);

	float x=clip->getTimePosition()/clip->getDuration();
	drawColoredQuad(3,573,794*x,24,1,1,1,1);
}

void update(float time_increase)
{
	mgr->update(time_increase / 10);
	if (needsSeek)
	{
		delay += time_increase;
		if (delay >= 0.0f)
		{
			delay = 0;
			printf("Requesting seek to frame %d\n", cFrame);
			clip->seekToFrame(cFrame);
			needsSeek = 0;
		}
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
	sprintf(out, " (%dx%d@%d) %d wrong seeks", clip->getWidth(), clip->getHeight(), (int) clip->getFPS(), nWrongSeeks);
}

void init()
{
	mgr=new TheoraVideoManager();
	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("media/bunny" + resourceExtension), TH_RGB, 4);
	clip->setAutoRestart(1);

	tex_id=createTexture(nextPow2(clip->getWidth()), nextPow2(clip->getHeight()));
}

void destroy()
{
	delete mgr;
}
