#include "demo_seek.h"
#include "theoraplayer/MemoryDataSource.h"
#include "theoraplayer/theoraplayer.h"
#include "theoraplayer/VideoFrame.h"

using namespace theoraplayer;

unsigned int tex_id_seek;
Manager* mgr_seek;
VideoClip* clip_seek;
bool started_seek = 1, needsSeek = 1;
int cFrame_seek = 0, nWrongSeeks = 0;
float delay = 0;

void seek_draw()
{
	glBindTexture(GL_TEXTURE_2D, tex_id_seek);

	if (!needsSeek)
	{
		VideoFrame* f = clip_seek->getNextFrame();
		if (f)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip_seek->getWidth(), f->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, f->getBuffer());
			needsSeek = 1;
			if (f->getFrameNumber() != cFrame_seek)
				nWrongSeeks++;
			cFrame_seek++;
			if (cFrame_seek >= clip_seek->getNumFrames()) cFrame_seek = 0;
			printf("Displayed frame %d\n", (int)f->getFrameNumber());
			clip_seek->popFrame();
		}
	}


	float w = clip_seek->getWidth(), h = clip_seek->getHeight();
	float tw = nextPow2(w), th = nextPow2(h);

	glEnable(GL_TEXTURE_2D);
	if (shaderActive) enableShader();
	drawTexturedQuad(tex_id_seek, 0, 0, 800, 600, w / tw, h / th);
	if (shaderActive) disableShader();

	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(0, 570, 800, 30, 0, 0, 0, 1);
	drawWiredQuad(0, 570, 800, 30, 1, 1, 1, 1);

	float x = clip_seek->getTimePosition() / clip_seek->getDuration();
	drawColoredQuad(3, 573, 794 * x, 24, 1, 1, 1, 1);
}

void seek_update(float time_increase)
{
	mgr_seek->update(time_increase / 10);
	if (needsSeek)
	{
		delay += time_increase;
		if (delay >= 0.0f)
		{
			delay = 0;
			printf("Requesting seek to frame %d\n", cFrame_seek);
			clip_seek->seekToFrame(cFrame_seek);
			needsSeek = 0;
		}
	}
}

void seek_OnKeyPress(int key)
{

}

void seek_OnClick(float x, float y)
{

}

void seek_setDebugTitle(char* out)
{
	sprintf(out, " (%dx%d@%d) %d wrong seeks", clip_seek->getWidth(), clip_seek->getHeight(), (int)clip_seek->getFps(), nWrongSeeks);
}

void seek_init()
{
	mgr_seek = new Manager(1);
	clip_seek = mgr_seek->createVideoClip(new MemoryDataSource("media/bunny" + resourceExtension), TH_RGB, 4);
	clip_seek->setAutoRestart(1);

	tex_id_seek = createTexture(nextPow2(clip_seek->getWidth()), nextPow2(clip_seek->getHeight()));
}

void seek_destroy()
{
	delete mgr_seek;
}
