/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "demo_av.h"
#include "demo_composite.h"
#include "demo_environment.h"
#include "demo_glutPlayer.h"
#include "demo_lightMap.h"
#include "demo_menu.h"
#include "demo_multiple.h"
#include "demo_seek.h"
#include "demo_spriteAnimation.h"
#include "demo_tv.h"
#include "tga.h"
#include "util.h"

#define MAX_DEMOS 9 // change this if another demo is added

namespace menu
{
	unsigned int ids[MAX_DEMOS];

	void init()
	{
		for (int i = 0; i < MAX_DEMOS; i++)
		{
			ids[i] = loadTexture(("media/button" + str(i + 1) + ".tga").c_str());
		}
	}

	void destroy()
	{
		glDeleteTextures(MAX_DEMOS, ids);
	}

	void draw()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glLoadIdentity();
		glColor3f(1.0f, 1.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_COLOR_MATERIAL);
		for (int i = 0; i < MAX_DEMOS; i++)
		{
			drawTexturedQuad(ids[i], 0.0f, i * 32.0f, 128.0f, 32.0f, 1.0f, 1.0f);
		}
	}

	void onClick(float x, float y)
	{
		float buttonPanelY = 0.0f;
		float buttonHeight = 32.0f;
		int selected = MAX_DEMOS + 1;
		for (int i = 0; i < MAX_DEMOS; i++)
		{
			if (y > buttonPanelY + i * buttonHeight && y < buttonPanelY + (i + 1) * buttonHeight)
			{
				selected = i + 1;
				break;
			}
		}
		switch (selected)
		{
		case 1:		changeDemo(&glutPlayer::demo);		break;
		case 2:		changeDemo(&spriteAnimation::demo);	break;
		case 3:		changeDemo(&multiple::demo);		break;
		case 4:		changeDemo(&seek::demo);			break;
		case 5:		changeDemo(&tv::demo);				break;
		case 6:		changeDemo(&lightMap::demo);		break;
		case 7:		changeDemo(&av::demo);				break;
		case 8:		changeDemo(&composite::demo);		break;
		case 9:		changeDemo(&environment::demo);		break;
		default:	changeDemo(&menu::demo);			break;
		}
	}

	Demo demo = { init, destroy, NULL, draw, NULL, NULL, onClick };

}