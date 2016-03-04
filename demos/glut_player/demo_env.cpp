#include "demo_env.h"
#include "theoraplayer/MemoryDataSource.h"
#include "theoraplayer/theoraplayer.h"
#include "theoraplayer/VideoFrame.h"

using namespace theoraplayer;

unsigned int tex_id_env;
Manager* mgr_env;
VideoClip* clip_env;
bool started_env = 1;
float angle_env = 0;
ObjModel teapot_env;

void env_draw()
{
	glBindTexture(GL_TEXTURE_2D, tex_id_env);

	glLoadIdentity();
	gluLookAt(0, 2000, 1000, 0, 0, 0, 0, -1, 0);

	glRotatef(angle_env, 0, 0, 1);
	VideoFrame* f = clip_env->getNextFrame();
	if (f)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip_env->getWidth(), f->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, f->getBuffer());
		clip_env->popFrame();
	}

	teapot_env.draw();
}

void env_update(float time_increase)
{
	angle_env += time_increase * 20;
	mgr_env->update(time_increase);
}

void env_OnKeyPress(int key)
{

}

void env_OnClick(float x, float y)
{

}

void env_setDebugTitle(char* out)
{
	sprintf(out, "");
}

void env_init()
{
	mgr_env = new Manager(1);
	clip_env = mgr_env->createVideoClip(new MemoryDataSource("media/environment_mapping/room256" + resourceExtension), TH_RGB);
	clip_env->setAutoRestart(1);

	tex_id_env = createTexture(nextPow2(clip_env->getWidth()), nextPow2(clip_env->getHeight()));

	teapot_env.load("media/environment_mapping/teapot_env.obj", tex_id_env, true);

	glClearColor(1, 1, 1, 1);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_COLOR_MATERIAL);
}

void env_destroy()
{
	delete mgr_env;
}
