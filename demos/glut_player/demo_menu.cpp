#include "demo_menu.h"
#include "demo_glut_player.h"
#include "demo_spriteanim.h"
#include "demo_multiple.h"
#include "demo_seek.h"
#include "demo_tv.h"
#include "demo_light.h"
#include "demo_av.h"
#include "demo_comp.h"
#include "demo_env.h"

#include "tga.h"

Demo demoMenu = { menu_init, menu_destroy, menu_update, menu_draw, menu_setDebugTitle, menu_OnKeyPress, menu_OnClick };
Demo demoGlut = { glutplayer_init, glutplayer_destroy, glutplayer_update, glutplayer_draw, glutplayer_setDebugTitle, glutplayer_OnKeyPress, glutplayer_OnClick };
Demo demoSpriteanim = { spriteanim_init, spriteanim_destroy, spriteanim_update, spriteanim_draw, spriteanim_setDebugTitle, spriteanim_OnKeyPress, spriteanim_OnClick };
Demo demoMultiple = { multiple_init, multiple_destroy, multiple_update, multiple_draw, multiple_setDebugTitle, multiple_OnKeyPress, multiple_OnClick };
Demo demoSeek = { seek_init, seek_destroy, seek_update, seek_draw, seek_setDebugTitle, seek_OnKeyPress, seek_OnClick };
Demo demoTv = { tv_init, tv_destroy, tv_update, tv_draw, tv_setDebugTitle, tv_OnKeyPress, tv_OnClick };
Demo demoLight = { light_init, light_destroy, light_update, light_draw, light_setDebugTitle, light_OnKeyPress, light_OnClick };
Demo demoAv = { av_init, av_destroy, av_update, av_draw, av_setDebugTitle, av_OnKeyPress, av_OnClick };
Demo demoComp = { comp_init, comp_destroy, comp_update, comp_draw, comp_setDebugTitle, comp_OnKeyPress, comp_OnClick };
Demo demoEnv = { env_init, env_destroy, env_update, env_draw, env_setDebugTitle, env_OnKeyPress, env_OnClick };

unsigned int ids[9];

void menu_draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glLoadIdentity();

	glColor3f(1.0f, 1.0f, 1.0f);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_COLOR_MATERIAL);

	for (int i = 0; i < 9; i++)
	{
		drawTexturedQuad(ids[i], 0, i*32, 128, 32, 1, 1);
	}
}

void menu_update(float time_increase)
{
	
}

void menu_OnKeyPress(int key)
{
	
}

void menu_OnClick(float x, float y)
{
	float buttonPanelY = 0;
	float buttonHeight = 32;
	int num = 10;
	for (int i = 0; i < 9; i++)
	{
		if (y > buttonPanelY + i*buttonHeight && y < buttonPanelY + (i + 1)*buttonHeight)
			num = i + 1;
	}
	switch (num)
	{
	case 1:
		ChangeDemo(&demoGlut); break;
	case 2:
		ChangeDemo(&demoSpriteanim); break;
	case 3:
		ChangeDemo(&demoMultiple); break;
	case 4:
		ChangeDemo(&demoSeek); break;
	case 5:
		ChangeDemo(&demoTv); break;
	case 6:
		ChangeDemo(&demoLight); break;
	case 7:
		ChangeDemo(&demoAv); break;
	case 8:
		ChangeDemo(&demoComp); break;
	case 9:
		ChangeDemo(&demoEnv); break;
	default:
		ChangeDemo(&demoMenu); break;
	}
}

void menu_setDebugTitle(char* out)
{
	
}

void menu_init()
{
	int w, h;
	ids[0] = loadTexture("media/button1.tga", &w, &h);
	ids[1] = loadTexture("media/button2.tga", &w, &h);
	ids[2] = loadTexture("media/button3.tga", &w, &h);
	ids[3] = loadTexture("media/button4.tga", &w, &h);
	ids[4] = loadTexture("media/button5.tga", &w, &h);
	ids[5] = loadTexture("media/button6.tga", &w, &h);
	ids[6] = loadTexture("media/button7.tga", &w, &h);
	ids[7] = loadTexture("media/button8.tga", &w, &h);
	ids[8] = loadTexture("media/button9.tga", &w, &h);
}

void menu_destroy()
{
	for (int i = 0; i < 9; i++)
		glDeleteTextures(1, &ids[i]);
}