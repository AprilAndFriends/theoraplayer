#include "demo_multiple.h"

TheoraVideoClip* clips_multiple[4];
unsigned int textures_multiple[4];
TheoraVideoManager* mgr_multiple;

#ifdef MP4_VIDEO
TheoraOutputMode outputMode_multiple = TH_BGRX;
unsigned int textureFormat_multiple = GL_BGRA_EXT;
unsigned int uploadFormat_multiple = GL_BGRA_EXT;
#else
TheoraOutputMode outputMode_multiple = TH_RGB;
unsigned int textureFormat_multiple = GL_RGB;
unsigned int uploadFormat_multiple = GL_RGB;
#endif

void drawVideo(int x, int y, unsigned int tex_id, TheoraVideoClip* clip)
{
	//glLoadIdentity();
	//glTranslatef(x,y,0);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	TheoraVideoFrame* f = clip->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), f->getHeight(), uploadFormat_multiple, GL_UNSIGNED_BYTE, f->getBuffer());

		clip->popFrame();
	}

	float w = clip->getWidth(), h = clip->getHeight();
	float tw = nextPow2(w), th = nextPow2(h);

	glEnable(GL_TEXTURE_2D);
	if (shader_on) enable_shader();
	drawTexturedQuad(tex_id, x, y, 395, 285, w / tw, h / th);
	if (shader_on) disable_shader();

	glDisable(GL_TEXTURE_2D);
	drawColoredQuad(x, y + 285, 395, 15, 0, 0, 0, 1);
	drawWiredQuad(x, y + 285, 395, 14, 1, 1, 1, 1);

	float p = clip->getTimePosition() / clip->getDuration();
	drawColoredQuad(x + 1.5f, y + 574 / 2, 784 * p / 2, 11, 1, 1, 1, 1);
}

void multiple_draw()
{
	drawVideo(0, 0, textures_multiple[0], clips_multiple[0]);
	drawVideo(400, 0, textures_multiple[1], clips_multiple[1]);
	drawVideo(0, 300, textures_multiple[2], clips_multiple[2]);
	drawVideo(400, 300, textures_multiple[3], clips_multiple[3]);
}

void multiple_update(float time_increase)
{
	mgr_multiple->update(time_increase);
}

void multiple_setDebugTitle(char* out)
{
	char temp[32];
	for (int i = 0;i<4;i++)
	{
		sprintf(temp, "%d/%d  ", clips_multiple[i]->getNumReadyFrames(), clips_multiple[i]->getNumPrecachedFrames());
		strcat(out, temp);
	}
	sprintf(temp, "(%d worker threads)", mgr_multiple->getNumWorkerThreads());
	strcat(out, temp);
}

void playPause(int index)
{
	if (clips_multiple[index]->isPaused()) clips_multiple[index]->play();
	else                          clips_multiple[index]->pause();
}

void multiple_OnKeyPress(int key)
{
	if (key == '1') mgr_multiple->setNumWorkerThreads(1);
	if (key == '2') mgr_multiple->setNumWorkerThreads(2);
	if (key == '3') mgr_multiple->setNumWorkerThreads(3);
	if (key == '4') mgr_multiple->setNumWorkerThreads(4);
	if (key >= 1 && key <= 4) playPause(key - 1); // Function keys are used for play/pause

	if (key == 5 || key == 6 || key == 7)
	{
		TheoraOutputMode mode;
		if (key == 5) mode = TH_RGB;
		if (key == 6) mode = TH_YUV;
		if (key == 7) mode = TH_GREY3;

		for (int i = 0;i<4;i++) clips_multiple[i]->setOutputMode(mode);
	}
}

void multiple_OnClick(float x, float y)
{

}


void multiple_init()
{
	printf("---\nUSAGE: press buttons 1,2,3 or 4 to change the number of worker threads\n---\n");

	std::string files[] = { "media/bunny" + resourceExtension,
		"media/konqi" + resourceExtension,
		"media/room" + resourceExtension,
		"media/titan" + resourceExtension };
	mgr_multiple = new TheoraVideoManager(4);
	mgr_multiple->setDefaultNumPrecachedFrames(16);
	for (int i = 0;i<4;i++)
	{
		clips_multiple[i] = mgr_multiple->createVideoClip(new TheoraMemoryFileDataSource(files[i]), outputMode_multiple);

		clips_multiple[i]->setAutoRestart(1);
		textures_multiple[i] = createTexture(nextPow2(clips_multiple[i]->getWidth()), nextPow2(clips_multiple[i]->getHeight()), textureFormat_multiple);
	}
}

void multiple_destroy()
{
	delete mgr_multiple;
}
