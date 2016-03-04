#include "demo_light.h"

#include "theoraplayer/MemoryDataSource.h"
#include "theoraplayer/theoraplayer.h"
#include "theoraplayer/VideoFrame.h"

using namespace theoraplayer;

unsigned int tex_id_light, diffuse_map;
Manager* mgr_light;
VideoClip* clip_light;
bool started_light = 1, diffuse_enabled = 1, lighting_enabled = 1;

struct xyz
{
	float x, y, z;
};
std::vector<xyz> camera_pos;

ObjModel room_light;
float angle_x = 0, angle_y = 0;

void light_draw()
{
	glClearColor(1, 1, 1, 1);

	glLoadIdentity();
	float x1, y1, z1, x2 = -65.147f, y2 = 80.219f, z2 = 12.301f;
	static int index = 0;

	VideoFrame* f = clip_light->getNextFrame();
	if (f)
	{
		index = (int)f->getFrameNumber();
		glBindTexture(GL_TEXTURE_2D, tex_id_light);
		unsigned char* buffer = f->getBuffer();
		int x, len = f->getWidth() * f->getHeight() * 3;
		for (int i = 0; i < len; i++)
		{
			x = (*buffer) * 0.8f + 255 * 0.2f;
			if (x > 255) *buffer = 255;
			else *buffer = x;
			buffer++;
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, f->getWidth(), f->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, f->getBuffer());
		clip_light->popFrame();
	}
	x1 = camera_pos[index].x; y1 = camera_pos[index].y; z1 = camera_pos[index].z;
	gluLookAt(x1, z1, -y1, x2, z2, -y2, 0, 1, 0);
	//gluLookAt(x1, y1, z1, x2, z2, -y2,  0,1,0);

	glBlendFunc(GL_ONE, GL_ZERO);
	glEnable(GL_CULL_FACE);
	if (diffuse_enabled) glEnable(GL_TEXTURE_2D);
	else glDisable(GL_TEXTURE_2D);
	room_light.draw();
	glDisable(GL_CULL_FACE);

	if (lighting_enabled)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_id_light);

		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glDisable(GL_DEPTH_TEST);

		glBlendFunc(GL_DST_COLOR, GL_ZERO);

		glBegin(GL_QUADS);

		glTexCoord2f(0, 4 / 1024.0f); glVertex3f(-1, 1, 0);
		glTexCoord2f(800 / 1024.0f, 4 / 1024.0f); glVertex3f(1, 1, 0);
		glTexCoord2f(800 / 1024.0f, 604 / 1024.0f); glVertex3f(1, -1, 0);
		glTexCoord2f(0, 604 / 1024.0f); glVertex3f(-1, -1, 0);

		glEnd();

		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glEnable(GL_DEPTH_TEST);
	}
}

void light_update(float time_increase)
{
	float x, y;
	getCursorPos(&x, &y);
	angle_x = -4 * 3.14f*x / windowWidth;
	angle_y = 1500 * (y - 300) / windowHeight;

	mgr_light->update(time_increase);
}

void light_OnKeyPress(int key)
{
	if (key == ' ') diffuse_enabled = !diffuse_enabled;
	if (key == 13) lighting_enabled = !lighting_enabled; // 13 = ENTER key
}

void light_OnClick(float x, float y)
{

}

void light_setDebugTitle(char* out)
{
	sprintf(out, "press SPACE to toggle diffuse map, ENTER to toggle lighting");
}

void light_init()
{
	FILE* f = fopen("media/lighting/camera.txt", "r");

	xyz pos;
	while (!feof(f))
	{
		fscanf(f, "%f %f %f", &pos.x, &pos.y, &pos.z);
		camera_pos.push_back(pos);
	}

	fclose(f);

	FOVY = 54.495f;
	mgr_light = new Manager(1);
	clip_light = mgr_light->createVideoClip(new MemoryDataSource("media/lighting/lighting" + resourceExtension), TH_RGB);
	clip_light->setAutoRestart(1);
	//clip_light->setPlaybackSpeed(0.5f);

	tex_id_light = createTexture(nextPow2(clip_light->getWidth()), nextPow2(clip_light->getHeight()));
	diffuse_map = loadTexture("media/lighting/diffuse_map.tga");

	room_light.load("media/lighting/room_light.obj", diffuse_map);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LESS);
	glEnable(GL_COLOR_MATERIAL);
}

void light_destroy()
{
	delete mgr_light;
}