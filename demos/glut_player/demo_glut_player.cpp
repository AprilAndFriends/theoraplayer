#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_glut_player.h"

unsigned int textureId = 0;
theoraplayer::VideoClip* clip = NULL;
bool started = true;

#ifdef MP4_VIDEO
theoraplayer::TheoraOutputMode outputMode = theoraplayer::TH_BGRX;
unsigned int textureFormat = GL_BGRA_EXT;
#else
theoraplayer::TheoraOutputMode outputMode = theoraplayer::TH_RGB;
unsigned int textureFormat = GL_RGB;
#endif

void glutplayer_draw()
{
	glBindTexture(GL_TEXTURE_2D, textureId);

	theoraplayer::VideoFrame* frame = clip->getNextFrame();
	if (frame != NULL)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), clip->getHeight(), textureFormat, GL_UNSIGNED_BYTE, frame->getBuffer());
		//printf("Displaying frame %d\n", f->getFrameNumber());
		clip->popFrame();
	}
	float w = clip->getSubFrameWidth();
	float h = clip->getSubFrameHeight();
	float sx = clip->getSubFrameOffsetX();
	float sy = clip->getSubFrameOffsetY();
	float tw = nextPow2(w);
	float th = nextPow2(h);
	glEnable(GL_TEXTURE_2D);
	if (shaderActive)
	{
		enableShader();
	}
	drawTexturedQuad(textureId, 0.0f, 0.0f, 800.0f, 570.0f, w / tw, h / th, sx / tw, sy / th);
	if (shaderActive)
	{
		disableShader();
	}
	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(0.0f, 570.0f, 800.0f, 30.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	drawWiredQuad(0.0f, 570.0f, 800.0f, 29.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	float x = clip->getTimePosition() / clip->getDuration();
	drawColoredQuad(3.0f, 573.0f, 794.0f * x, 24.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

void glutplayer_update(float time_increase)
{
	if (started)
	{
		// let's wait until the system caches up a few frames on startup
		if (clip->getNumReadyFrames() < clip->getNumPrecachedFrames() * 0.5f)
		{
			return;
		}
		started = false;
	}
	theoraplayer::manager->update(time_increase);
}

void glutplayer_OnKeyPress(int key)
{
	if (key == ' ')
	{
		clip->isPaused() ? clip->play() : clip->pause();
	}
	if (key == 114)
	{
		clip->restart();
		clip->pause();
		clip->play();
	}
	if (key == 5)
	{
		clip->setOutputMode(theoraplayer::TH_RGB);
	}
	else if (key == 6)
	{
		clip->setOutputMode(theoraplayer::TH_YUV);
	}
	else if (key == 7)
	{
		clip->setOutputMode(theoraplayer::TH_GREY3);
	}
}

void glutplayer_OnClick(float x, float y)
{
	if (y > 570)
	{
		clip->seek((x / windowWidth) * clip->getDuration());
	}
}

void glutplayer_setDebugTitle(char* out)
{
	int dropped = clip->getDroppedFramesCount();
	int displayed = clip->getDisplayedFramesCount();
	float percent = 100 * ((float)dropped / displayed);
	sprintf(out, " (%dx%d) %d precached, %d displayed, %d dropped (%.1f %%)", clip->getWidth(), clip->getHeight(), clip->getNumReadyFrames(), displayed, dropped, percent);
}

#ifdef BENCHMARK
void glutplayer_benchmark(const char* filename)
{
	int nPrecached = 256;
	int n = nPrecached;
	VideoClip* clip = mgr->createVideoClip(filename, outputMode, 32);
	clock_t t = clock();
	while (n > 0)
	{
		clip->waitForCache(1.0f, 1000000);
		n -= 32;
		clip->getFrameQueue()->clear();
	}
	float diff = ((float)(clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
	printf("%s: Decoding %d frames took %.1fms (%.2fms average per frame)\n", filename, nPrecached, diff, diff / nPrecached);
	mgr->destroyVideoClip(clip);
}
#endif

void glutplayer_init()
{
	theoraplayer::init(1);

#ifdef BENCHMARK
	benchmark("media/witch_intro.ogv");
	benchmark("media/hotel_intro.ogv");
	benchmark("media/angels_intro.ogv");
	benchmark("media/witch_intro.mp4");
	benchmark("media/hotel_intro.mp4");
	benchmark("media/angels_intro.mp4");
#endif
#ifndef __WEBM
	clip = theoraplayer::manager->createVideoClip("media/bunny" + resourceExtension, outputMode, 16);
#else
	clip = theoraplayer::manager->createVideoClip("media/bunny.webm", outputMode, 16);
#endif

	//  use this if you want to preload the file into ram and stream from there
	//	clip = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource("../media/short" + resourceExtension), theoraplayer::TH_RGB);
	clip->setAutoRestart(true);
	textureId = createTexture(nextPow2(clip->getWidth()), nextPow2(clip->getHeight()), textureFormat);
}

void glutplayer_destroy()
{
	theoraplayer::destroy();
}