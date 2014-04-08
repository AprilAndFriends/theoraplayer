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

//#define BENCHMARK // uncomment this to benchmark decoding times
#ifdef BENCHMARK
    #include <time.h>
#include <theoraplayer/TheoraFrameQueue.h>
#endif
unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="glut_player";
bool started=1;
int window_w=800,window_h=600;

#ifdef MP4_VIDEO
	TheoraOutputMode outputMode = TH_BGRX;
	unsigned int textureFormat = GL_BGRA_EXT;

// TEST
//	TheoraOutputMode outputMode = TH_RGB;
//	unsigned int textureFormat = GL_RGB;
//	TheoraOutputMode outputMode = TH_GREY;
//	unsigned int textureFormat = GL_LUMINANCE;
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
		//printf("Displaying frame %d\n", f->getFrameNumber());
		clip->popFrame();
	}

	float w = clip->getSubFrameWidth(),h = clip->getSubFrameHeight();
    float sx = clip->getSubFrameOffsetX(), sy = clip->getSubFrameOffsetY();
	float tw = nextPow2(w),th = nextPow2(h);

	glEnable(GL_TEXTURE_2D);
	if (shader_on) enable_shader();
	drawTexturedQuad(tex_id, 0, 0, 800, 570, w / tw,h / th, sx / tw, sy / th);
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
	{
		clip->seek((x/window_w)*clip->getDuration());
	}
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

#ifdef BENCHMARK
void benchmark(const char* filename)
{
	int nPrecached = 256;
	int n = nPrecached;
	TheoraVideoClip* clip = mgr->createVideoClip(filename, outputMode, 32);
    clock_t t = clock();
	while (n > 0)
	{
		clip->waitForCache(1.0f, 1000000);
		n -= 32;
		clip->getFrameQueue()->clear();
	}
    float diff = ((float) (clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
    printf("%s: Decoding %d frames took %.1fms (%.2fms average per frame)\n",filename, nPrecached, diff, diff / nPrecached);
	mgr->destroyVideoClip(clip);
}
#endif

void init()
{
	mgr = new TheoraVideoManager();

#ifdef BENCHMARK
	benchmark("media/witch_intro.ogv");
	benchmark("media/hotel_intro.ogv");
	benchmark("media/angels_intro.ogv");
	benchmark("media/witch_intro.mp4");
	benchmark("media/hotel_intro.mp4");
	benchmark("media/angels_intro.mp4");
#endif
    clip=mgr->createVideoClip("media/bunny" + resourceExtension, outputMode, 16);

//  use this if you want to preload the file into ram and stream from there
//	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("../media/short" + resourceExtension),TH_RGB);
	clip->setAutoRestart(1);
	
	tex_id=createTexture(nextPow2(clip->getWidth()),nextPow2(clip->getHeight()), textureFormat);
}

void destroy()
{
	delete mgr;
}
