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
#include "OpenAL_AudioInterface.h"

unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="av_player";
bool started=1;
int window_w=800,window_h=600;
OpenAL_AudioInterfaceFactory* iface_factory;

#ifdef MP4_VIDEO
TheoraOutputMode outputMode = TH_BGRX;
unsigned int textureFormat = GL_BGRA_EXT;
#else
TheoraOutputMode outputMode = TH_RGB;
unsigned int textureFormat = GL_RGB;
#endif

void draw()
{
	glBindTexture(GL_TEXTURE_2D,tex_id);

	TheoraVideoFrame* f=clip->getNextFrame();

	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),textureFormat,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
	}


	float w=clip->getWidth(),h=clip->getHeight();
	float tw=nextPow2(w),th=nextPow2(h);

	glEnable(GL_TEXTURE_2D);
	if (shader_on) enable_shader();
	drawTexturedQuad(tex_id,0,0,800,570,w/tw,h/th);
	if (shader_on) disable_shader();

	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(0,570,800,30,0,0,0,1);
	drawWiredQuad(0,570,800,29,1,1,1,1);

	float x=clip->getTimePosition()/clip->getDuration();
	drawColoredQuad(3,573,794*x,24,1,1,1,1);
}

void update(float time_increase)
{
	if (started)
	{
		// let's wait until the system caches up a few frames on startup
		if (clip->getNumReadyFrames() < 2)
			return;
		started=0;
		clip->play();
	}
	mgr->update(time_increase);
}

void OnKeyPress(int key)
{
	if (key == ' ')
    { if (clip->isPaused()) clip->play(); else clip->pause(); }

	if (key == 5) clip->setOutputMode(TH_RGB);
	if (key == 6) clip->setOutputMode(TH_YUV);
	if (key == 7) clip->setOutputMode(TH_GREY3);
}

void OnClick(float x,float y)
{
	if (y > 570)
	{
		clip->seek((x/window_w)*clip->getDuration());
		clip->waitForCache();
	}
}

void setDebugTitle(char* out)
{
	float buffer_size = 0;
	OpenAL_AudioInterface* audio_iface = (OpenAL_AudioInterface*) clip->getAudioInterface();
	if (audio_iface) buffer_size = audio_iface->getQueuedAudioSize();
	int nDropped=clip->getNumDroppedFrames();
	sprintf(out,"%d precached, %d dropped, buffered audio: %.2f s",
		clip->getNumReadyFrames(),	nDropped, 
		buffer_size);
}

void init()
{
	mgr = new TheoraVideoManager();
	iface_factory = new OpenAL_AudioInterfaceFactory();
	mgr->setAudioInterfaceFactory(iface_factory);
	
	/*/ Test Memory Leaks
	
	for (;;)
	{
		clip = mgr->createVideoClip("media/bunny.ogv", outputMode, 16);
		clip->seek((rand()%50)/10.0f);
//		sleep(1);
		usleep(rand()%1000000);
		mgr->update(0);
		mgr->destroyVideoClip(clip);
	}
	
	//*/
	clip=mgr->createVideoClip("media/bunny" + resourceExtension, outputMode, 16);
//  use this if you want to preload the file into ram and stream from there
//	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("../media/short" + resourceExtension),TH_RGB);
	clip->setAutoRestart(1);
	clip->pause();
	tex_id=createTexture(nextPow2(clip->getWidth()),nextPow2(clip->getHeight()), textureFormat);
}

void destroy()
{
	delete mgr;
	delete iface_factory;
}
