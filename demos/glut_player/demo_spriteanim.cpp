#include "demo_spriteanim.h"

unsigned int tex_id_spriteanim;
TheoraVideoManager* mgr_spriteanim;
TheoraVideoClip* clips[8];
bool started_spriteanim = 1;
int cClip = 0;
unsigned char buffer[203 * 300 * 4];

void spriteanim_draw()
{
	glBindTexture(GL_TEXTURE_2D, tex_id_spriteanim);

	TheoraVideoFrame* f = clips[cClip]->getNextFrame();
	if (f)
	{
		unsigned char* src = f->getBuffer();
		int i, j, k, x, y, w = f->getWidth();
		for (y = 0;y<300;y++)
		{
			for (x = 0;x<203;x++)
			{
				i = (y * 203 + x) * 4;
				j = ((y + 2)*w + x + 4) * 3;
				k = ((y + 2)*w + x + 205 + 4) * 3;
				buffer[i] = src[j];
				buffer[i + 1] = src[j + 1];
				buffer[i + 2] = src[j + 2];
				buffer[i + 3] = src[k];
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 203, 300, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
		clips[cClip]->popFrame();
	}

	glEnable(GL_TEXTURE_2D);
	drawTexturedQuad(tex_id_spriteanim, 298, 150, 203, 300, 203.0f / 256.0f, 300.0f / 512.0f);
}

void spriteanim_update(float time_increase)
{
	int newindex = -1;
	float x, y;
	getCursorPos(&x, &y);
	if (x >= 400 && y <= 300)
	{
		if (x - 400 < (300 - y) / 2) newindex = 0;
		else if (x - 400 > (300 - y) * 2) newindex = 2;
		else                        newindex = 1;
	}
	else if (x < 400 && y <= 300)
	{
		if (400 - x < (300 - y) / 2) newindex = 0;
		else if (400 - x >(300 - y) * 2) newindex = 6;
		else                        newindex = 7;
	}
	else if (x >= 400 && y > 300)
	{
		if (x - 400 < (y - 300) / 2) newindex = 4;
		else if (x - 400 > (y - 300) * 2) newindex = 2;
		else                        newindex = 3;
	}
	else if (x < 400 && y > 300)
	{
		if (400 - x < (y - 300) / 2) newindex = 4;
		else if (400 - x >(y - 300) * 2) newindex = 6;
		else                        newindex = 5;
	}
	if (newindex >= 0)
	{
		clips[cClip]->pause();
		cClip = newindex;
		clips[cClip]->play();
	}
	mgr_spriteanim->update(time_increase);
}

void spriteanim_OnKeyPress(int key)
{
}

void spriteanim_OnClick(float x, float y)
{
}

void spriteanim_setDebugTitle(char* out)
{
	char temp[32];
	for (int i = 0;i<8;i++)
	{
		sprintf(temp, "%d/%d  ", clips[i]->getNumReadyFrames(), clips[i]->getNumPrecachedFrames());
		strcat(out, temp);
	}
	strcat(out, temp);
	strcat(out, " (Kaptain Brawe)");
}

void spriteanim_init()
{
	mgr_spriteanim = new TheoraVideoManager();
	std::string orientations[] = { "N","NE","E","SE","S","SW","W","NW" };
	for (int i = 0;i<8;i++)
	{
		// Note - this demo isn't using TH_RGBA for now since the frames in this video are not mod 16 aligned.
		clips[i] = mgr_spriteanim->createVideoClip(new TheoraMemoryFileDataSource("media/brawe/brawe_" + orientations[i] + "" + resourceExtension), TH_RGB, 8);
		clips[i]->setAutoRestart(1);
		if (i != 0) clips[i]->pause();
	}

	tex_id_spriteanim = createTexture(256, 512, GL_RGBA);

	glClearColor(0, 0.5f, 0, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void spriteanim_destroy()
{
	delete mgr_spriteanim;
}