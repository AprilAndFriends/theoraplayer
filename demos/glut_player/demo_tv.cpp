#include <theoraplayer/MemoryDataSource.h>
#include <theoraplayer/theoraplayer.h>
#include <theoraplayer/VideoFrame.h>

#include "demo_tv.h"

unsigned int tex_id_tv = 0;
theoraplayer::VideoClip* clip_tv = NULL;
bool started_tv = true;

ObjModel chair1, chair2, tv, room, table;
float anglex = 0, angley = 0;
unsigned int r = 0, g = 0, b = 0;

void tv_draw()
{
	glBindTexture(GL_TEXTURE_2D, tex_id_tv);

	glLoadIdentity();
	gluLookAt(sin(anglex) * 400 - 200, angley, cos(anglex) * 400, -200, 150, 0, 0, 1, 0);

	theoraplayer::VideoFrame* frame = clip_tv->getNextFrame();
	if (frame != NULL)
	{
		unsigned char* data = frame->getBuffer();
		unsigned int n = clip_tv->getWidth() * frame->getHeight();

		r = g = b = 0;
		for (unsigned int i = 0;i<n;i++)
		{
			r += data[i * 3];
			g += data[i * 3 + 1];
			b += data[i * 3 + 2];
		}
		r /= n; g /= n; b /= n;
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip_tv->getWidth(), frame->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, frame->getBuffer());
		clip_tv->popFrame();
	}


	float w = clip_tv->getWidth();
	float h = clip_tv->getHeight();
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
	drawTexturedQuad(tex_id_tv, -2 * 30, 190, 4 * 30, -3 * 25, w / tw, h / th);
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

void tv_update(float time_increase)
{
	float x, y;
	getCursorPos(&x, &y);
	anglex = -4 * 3.14f*x / windowWidth;
	angley = 1500 * (y - 300) / windowHeight;

	if (started_tv)
	{
		// let's wait until the system caches up a few frames on startup
		if (clip_tv->getNumReadyFrames() < clip_tv->getNumPrecachedFrames()*0.5f)
			return;
		started_tv = 0;
	}
	theoraplayer::manager->update(time_increase);
}

void tv_OnKeyPress(int key)
{
	if (key == ' ')
	{
		if (clip_tv->isPaused()) clip_tv->play(); else clip_tv->pause();
	}

	if (key == 5) clip_tv->setOutputMode(theoraplayer::TH_RGB);
	if (key == 6) clip_tv->setOutputMode(theoraplayer::TH_YUV);
	if (key == 7) clip_tv->setOutputMode(theoraplayer::TH_GREY3);
}

void tv_OnClick(float x, float y)
{
	if (y > 570)
		clip_tv->seek((x / windowWidth)*clip_tv->getDuration());
}

void tv_setDebugTitle(char* out)
{
	int dropped = clip_tv->getDroppedFramesCount();
	int displayed = clip_tv->getDisplayedFramesCount();
	float percent = 100 * ((float)dropped / displayed);
	sprintf(out, " (%dx%d) %d precached, %d displayed, %d dropped (%.1f %%)", clip_tv->getWidth(), clip_tv->getHeight(), clip_tv->getNumReadyFrames(), displayed, dropped, percent);
}

void tv_init()
{
	theoraplayer::init(1);
	clip_tv = theoraplayer::manager->createVideoClip("media/bunny" + resourceExtension, theoraplayer::TH_RGB);
	//  use this if you want to preload the file into ram and stream from there
	//	clip_tv=mgr_tv->createVideoClip(new TheoraMemoryFileDataSource("../media/short" + resourceExtension),TH_RGB);
	clip_tv->setAutoRestart(1);

	tex_id_tv = createTexture(nextPow2(clip_tv->getWidth()), nextPow2(clip_tv->getHeight()));

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
