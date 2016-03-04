#include "demo_av.h"

#include "theoraplayer/theoraplayer.h"
#include "theoraplayer/VideoFrame.h"

using namespace theoraplayer;

unsigned int tex_id_av;
Manager* mgr_av;
VideoClip* clip_av;
bool started_av = 1;
AudioInterfaceFactory* iface_factory;

#ifdef MP4_VIDEO
TheoraOutputMode outputMode_av = TH_BGRX;
unsigned int textureFormat_av = GL_BGRA_EXT;
#else
TheoraOutputMode outputMode_av = TH_RGB;
unsigned int textureFormat_av = GL_RGB;
#endif

void av_draw()
{
	glBindTexture(GL_TEXTURE_2D, tex_id_av);

	VideoFrame* f = clip_av->getNextFrame();

	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip_av->getWidth(), f->getHeight(), textureFormat_av, GL_UNSIGNED_BYTE, f->getBuffer());
		clip_av->popFrame();
	}

	float w = clip_av->getWidth(), h = clip_av->getHeight();
	float tw = nextPow2(w), th = nextPow2(h);

	glEnable(GL_TEXTURE_2D);
	if (shader_on) enable_shader();
	drawTexturedQuad(tex_id_av, 0, 0, 800, 570, w / tw, h / th);
	if (shader_on) disable_shader();

	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(0, 570, 800, 30, 0, 0, 0, 1);
	drawWiredQuad(0, 570, 800, 29, 1, 1, 1, 1);

	float x = clip_av->getTimePosition() / clip_av->getDuration();
	drawColoredQuad(3, 573, 794 * x, 24, 1, 1, 1, 1);
}

void av_update(float time_increase)
{
	if (started_av)
	{
		// let's wait until the system caches up a few frames on startup
		if (clip_av->getNumReadyFrames() < 2)
			return;
		started_av = 0;
		clip_av->play();
	}
	mgr_av->update(time_increase);
}

void av_OnKeyPress(int key)
{
	if (key == ' ')
	{
		if (clip_av->isPaused()) clip_av->play(); else clip_av->pause();
	}

	if (key == 5) clip_av->setOutputMode(TH_RGB);
	if (key == 6) clip_av->setOutputMode(TH_YUV);
	if (key == 7) clip_av->setOutputMode(TH_GREY3);
}

void av_OnClick(float x, float y)
{
	if (y > 570)
	{
		clip_av->seek((x / window_w)*clip_av->getDuration());
		clip_av->waitForCache();
	}
}

void av_setDebugTitle(char* out)
{
	float buffer_size = 0;
	OpenAL_AudioInterface* audio_iface = (OpenAL_AudioInterface*)clip_av->getAudioInterface();
	if (audio_iface) buffer_size = audio_iface->getQueuedAudioSize();
	int nDropped = clip_av->getNumDroppedFrames();
	sprintf(out, "%d precached, %d dropped, buffered audio: %.2f s",
		clip_av->getNumReadyFrames(), nDropped,
		buffer_size);
}

void av_init()
{
	mgr_av = new Manager(1);
	iface_factory = new OpenAL_AudioInterfaceFactory();
	mgr_av->setAudioInterfaceFactory(iface_factory);

	/*/ Test Memory Leaks

	for (;;)
	{
	clip_av = mgr_av->createVideoClip("media/bunny.ogv", outputMode_av, 16);
	clip_av->seek((rand()%50)/10.0f);
	//		sleep(1);
	usleep(rand()%1000000);
	mgr_av->update(0);
	mgr_av->destroyVideoClip(clip_av);
	}

	//*/
	clip_av = mgr_av->createVideoClip("media/bunny" + resourceExtension, outputMode_av, 16);
	//  use this if you want to preload the file into ram and stream from there
	//	clip_av=mgr_av->createVideoClip(new TheoraMemoryFileDataSource("../media/short" + resourceExtension),TH_RGB);
	clip_av->setAutoRestart(1);
	clip_av->pause();
	tex_id_av = createTexture(nextPow2(clip_av->getWidth()), nextPow2(clip_av->getHeight()), textureFormat_av);
}

void av_destroy()
{
	delete mgr_av;
	delete iface_factory;
}
