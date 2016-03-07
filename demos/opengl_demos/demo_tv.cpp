#if 0
#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_tv.h"

unsigned int textureIdTv = 0;
theoraplayer::VideoClip* clipTv = NULL;
bool started_tv = true;

ObjModel chair1, chair2, tv, room, table;
float anglex = 0, angley = 0;
unsigned int r = 0, g = 0, b = 0;

void tv_draw()
{
	glBindTexture(GL_TEXTURE_2D, textureIdTv);
	glLoadIdentity();
	gluLookAt(sin(anglex) * 400 - 200, angley, cos(anglex) * 400, -200, 150, 0, 0, 1, 0);
	theoraplayer::VideoFrame* frame = clipTv->getNextFrame();
	if (frame != NULL)
	{
		unsigned char* data = frame->getBuffer();
		unsigned int n = clipTv->getWidth() * frame->getHeight();
		r = g = b = 0;
		for (unsigned int i = 0; i < n; ++i)
		{
			r += data[i * 3];
			g += data[i * 3 + 1];
			b += data[i * 3 + 2];
		}
		r /= n; g /= n; b /= n;
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clipTv->getWidth(), frame->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, frame->getBuffer());
		clipTv->popFrame();
	}
	float w = clipTv->getWidth();
	float h = clipTv->getHeight();
	float tw = nextPow2(w);
	float th = nextPow2(h);
	glEnable(GL_TEXTURE_2D);
	if (shaderActive)
	{
		enableShader();
	}
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glRotatef(90, 0, 1, 0);
	glTranslatef(0, 0, -415);
	drawTexturedQuad(textureIdTv, -2 * 30, 190, 4 * 30, -3 * 25, w / tw, h / th);
	glPopMatrix();
	if (shaderActive)
	{
		disableShader();
	}
	glColor3f(0.2f + 0.8f * (r / 255.0f), 0.2f + 0.8f * (g / 255.0f), 0.2f + 0.8f * (b / 255.0f));
	chair1.draw();
	chair2.draw();
	table.draw();
	tv.draw();
	glEnable(GL_CULL_FACE);
	room.draw();
	glDisable(GL_CULL_FACE);
}

void tv_update(float timeDelta)
{
	float x = 0.0f;
	float y = 0.0f;
	getCursorPos(&x, &y);
	anglex = -4 * 3.14f*x / windowWidth;
	angley = 1500 * (y - 300) / windowHeight;
	if (started_tv)
	{
		// let's wait until the system caches up a few frames on startup
		if (clipTv->getNumReadyFrames() < clipTv->getNumPrecachedFrames() * 0.5f)
		{
			return;
		}
		started_tv = false;
	}
	theoraplayer::manager->update(timeDelta);
}

void tv_onKeyPress(int key)
{
	if (key == ' ')
	{
		clipTv->isPaused() ? clipTv->play() : clipTv->pause();
	}
	if (key == 5)
	{
		clipTv->setOutputMode(theoraplayer::TH_RGB);
	}
	if (key == 6)
	{
		clipTv->setOutputMode(theoraplayer::TH_YUV);
	}
	if (key == 7)
	{
		clipTv->setOutputMode(theoraplayer::TH_GREY3);
	}
}

void tv_onClick(float x, float y)
{
	if (y > 570)
	{
		clipTv->seek((x / windowWidth) * clipTv->getDuration());
	}
}

void tv_setDebugTitle(char* out)
{
	int dropped = clipTv->getDroppedFramesCount();
	int displayed = clipTv->getDisplayedFramesCount();
	float percent = 100 * ((float)dropped / displayed);
	sprintf(out, " (%dx%d) %d precached, %d displayed, %d dropped (%.1f %%)", clipTv->getWidth(), clipTv->getHeight(), clipTv->getNumReadyFrames(), displayed, dropped, percent);
}

void tv_init()
{
	theoraplayer::init(1);
	clipTv = theoraplayer::manager->createVideoClip("media/bunny" + resourceExtension, theoraplayer::TH_RGB);
	//  use this if you want to preload the file into ram and stream from there
	//	clipTv=mgr_tv->createVideoClip(new TheoraMemoryFileDataSource("../media/short" + resourceExtension),TH_RGB);
	clipTv->setAutoRestart(true);
	textureIdTv = createTexture(nextPow2(clipTv->getWidth()), nextPow2(clipTv->getHeight()));
	chair1.load("media/tv_room/chair1.obj", loadTexture("media/tv_room/chair1.tga"));
	chair2.load("media/tv_room/chair2.obj", loadTexture("media/tv_room/chair2.tga"));
	room.load("media/tv_room/room.obj", loadTexture("media/tv_room/room.tga"));
	table.load("media/tv_room/table.obj", loadTexture("media/tv_room/table.tga"));
	tv.load("media/tv_room/tv.obj", loadTexture("media/tv_room/tv.tga"));

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_COLOR_MATERIAL);
}

void tv_destroy()
{
	theoraplayer::destroy();
}
#endif
