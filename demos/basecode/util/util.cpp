#include "demo_basecode.h"

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

bool shader_on = 0;
float FOVY = 45;
std::string resourceExtension = ".ogv";
//#ifdef MP4_VIDEO
//".mp4";
//#else
//".ogg";
//#endif

#ifndef WIN32
unsigned long GetTickCount()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0) return 0;
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
#endif

void psleep(int milliseconds)
{
#ifdef _WIN32
	Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

int nextPow2(int x)
{
	int y;
	for (y=1;y<x;y*=2);
	return y;
}

unsigned int createTexture(int w,int h,unsigned int format)
{
	unsigned int tex_id;
	glGenTextures(1,&tex_id);
	glBindTexture(GL_TEXTURE_2D,tex_id);
	unsigned char* b=new unsigned char[w*h*4];
	memset(b,0,w*h*4);
	
	glTexImage2D(GL_TEXTURE_2D,0,format == GL_RGB ? GL_RGB : GL_RGBA,w,h,0,format,GL_UNSIGNED_BYTE,b);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	delete b;
	return tex_id;
}
