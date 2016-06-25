#ifdef _USE_WEBM
#include <clipwebm/clipwebm.h>
#endif
#ifdef _USE_FFMPEG
#include <clipffmpeg/clipffmpeg.h>
#endif
#ifdef _USE_AVFOUNDATION
#include <clipavfoundation/clipavfoundation.h>
#endif
#include <theoraplayer/Manager.h>
#include <theoraplayer/theoraplayer.h>

#include "demo_basecode.h"
#include "opengl_demos/demo_menu.h"
#include "util.h"

std::string window_name = "OpenGL Demos";
int windowWidth = 800;
int windowHeight = 600;
float mx = 0.0f;
float my = 0.0f;

Demo* currentDemo = &menu::demo;

void reshape(int w, int h);

void changeDemo(Demo* demo)
{
	destroy();
	currentDemo = demo;
    reshape(windowWidth, windowHeight);
	init();
}

#ifndef _LINUX // temp, compilation issues
#define USE_SHADERS
#endif

#ifdef USE_SHADERS

#if defined(WIN32)
#define pglGetProcAddress(func) wglGetProcAddress(func)
#elif defined(__APPLE__) || defined(_LINUX)
#define pglGetProcAddress(func) glXGetProcAddress((GLubyte*) func)
#endif

#ifndef __APPLE__
    #include <GL/glext.h>
    PFNGLCREATEPROGRAMPROC glCreateProgram = 0;
    PFNGLCREATESHADERPROC glCreateShader = 0;
    PFNGLLINKPROGRAMPROC glLinkProgram = 0;
    PFNGLSHADERSOURCEPROC glShaderSource = 0;
    PFNGLUSEPROGRAMPROC glUseProgram = 0;
    PFNGLCOMPILESHADERPROC glCompileShader = 0;
    PFNGLATTACHSHADERPROC glAttachShader = 0;
    PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = 0;
    PFNGLACTIVETEXTUREARBPROC   glActiveTextureARB = 0;
#else
    #ifdef _MAC
        #include <OpenGL/glext.h>
    #else

    #endif
#endif

unsigned int program, shader;
#endif

void init()
{
	if (currentDemo->init != NULL)
	{
		(*currentDemo->init)();
	}
}

void destroy()
{
	if (currentDemo->destroy != NULL)
	{
		(*currentDemo->destroy)();
	}
}

void update(float timeDelta)
{
	theoraplayer::manager->update(timeDelta);
	if (currentDemo->update != NULL)
	{
		(*currentDemo->update)(timeDelta);
	}
}

void draw()
{
	if (currentDemo->draw != NULL)
	{
		(*currentDemo->draw)();
	}
}

void setDebugTitle(char* newTitle)
{
	if (currentDemo->setDebugTitle != NULL)
	{
		(*currentDemo->setDebugTitle)(newTitle);
	}
}

void onKeyPress(int key)
{
	if (currentDemo->onKeyPress != NULL)
	{
		(*currentDemo->onKeyPress)(key);
	}
}

void onClick(float x, float y)
{
	if (currentDemo->onClick != NULL)
	{
		(*currentDemo->onClick)(x, y);
	}
}

void getCursorPosition(float& outX, float& outY)
{
	outX = mx;
	outY = my;
}

void display()
{
#ifdef __ZBUFFER
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
	glClear(GL_COLOR_BUFFER_BIT);
#endif
	draw();
	glutSwapBuffers();
	static unsigned long time = GetTickCount();
	unsigned long t = GetTickCount();
	float diff = (t - time) / 1000.0f;
	if (diff > 0.25f)
	{
		diff = 0.05f; // prevent spikes (usually happen on app load)
	}
	update(diff);
	static unsigned long fpsTimer = time;
	static unsigned long fpsCounter = 0;
	if (t - fpsTimer >= 250)
	{
		char title[512] = { 0 };
		char debug[256] = "";
		setDebugTitle(debug);
		sprintf(title, "%s: %ld FPS; %s", window_name.c_str(), fpsCounter * 4, debug);
		glutSetWindowTitle(title);
		fpsCounter = 0;
		fpsTimer = t;
	}
	else
	{
		fpsCounter++;
	}
	time = t;
}

void reshape(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, w, h);
#ifdef __3D_PROJECTION
	gluPerspective(FOVY, (float)windowWidth / windowHeight, 10.0f, 10000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#else
	gluOrtho2D(0.0, (double)windowWidth, (double)windowHeight, 0.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
}

void keyboard(unsigned char key, int x, int y)
{
	if (key == 27) // esc
	{
		if (currentDemo == &menu::demo)
		{
			destroy();
			exit(0);
		}
		else
		{
			changeDemo(&menu::demo);
		}
	}
	else
	{
		onKeyPress(key);
	}
}

void keyboard_special(int key, int x, int y)
{
	if (key == 10)
	{
		toggle_YUV2RGB_shader(); // F10
	}
	onKeyPress(key);
}

void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON)
	{
		mx = ((float)x / glutGet(GLUT_WINDOW_WIDTH)) * windowWidth;
		my = ((float)y / glutGet(GLUT_WINDOW_HEIGHT)) * windowHeight;
		onClick(mx, my);
	}
}

void motion(int x, int y)
{
	mx = ((float)x / glutGet(GLUT_WINDOW_WIDTH)) * windowWidth;
	my = ((float)y / glutGet(GLUT_WINDOW_HEIGHT)) * windowHeight;
}

void toggle_YUV2RGB_shader()
{
#ifdef USE_SHADERS
	if (!glCreateProgram)
	{
		if (!strstr((char*)glGetString(GL_EXTENSIONS), "GL_ARB_fragment_shader"))
		{
			printf("Unable to turn on yuv2rgb shader, your OpenGL driver doesn't support GLSL shaders!\n");
			return;
		}
#ifndef __APPLE__
		glCreateProgram = (PFNGLCREATEPROGRAMPROC)pglGetProcAddress("glCreateProgram");
		glCreateShader = (PFNGLCREATESHADERPROC)pglGetProcAddress("glCreateShader");
		glLinkProgram = (PFNGLLINKPROGRAMPROC)pglGetProcAddress("glLinkProgram");
		glShaderSource = (PFNGLSHADERSOURCEPROC)pglGetProcAddress("glShaderSource");
		glUseProgram = (PFNGLUSEPROGRAMPROC)pglGetProcAddress("glUseProgram");
		glCompileShader = (PFNGLCOMPILESHADERPROC)pglGetProcAddress("glCompileShader");
#endif
		const char* shader_code = "\
		uniform sampler2D diffuseMap; \
		void main(void) \
		{ \
			vec3 yuv = texture2D(diffuseMap, gl_TexCoord[0].st).xyz; \
			float y, u, v, r, g, b; \
			y = 1.1643 * (yuv.x - 0.0625); \
			u = yuv.y - 0.5; \
			v = yuv.z - 0.5; \
			r = y + 1.5958 * v; \
			g = y - 0.39173 * u - 0.81290 * v; \
			b = y + 2.017 * u; \
			gl_FragColor = vec4(r, g, b, 1.0); \
		}";
		program = glCreateProgram();
		shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(shader, 1, &shader_code, NULL);
		glCompileShader(shader);
		glAttachShader(program, shader);
		glLinkProgram(program);
	}
	shaderActive = !shaderActive;
#endif
}

void getMultiTextureExtensionFuncPointers()
{
#if defined(USE_SHADERS) && defined(WIN32)
	glAttachShader = (PFNGLATTACHSHADERPROC)pglGetProcAddress("glAttachShader");
	glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)pglGetProcAddress("glMultiTexCoord2fARB");
	glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)pglGetProcAddress("glActiveTextureARB");
#endif
}

void enableShader()
{
#ifdef USE_SHADERS
	glUseProgram(program);
#endif
}
void disableShader()
{
#ifdef USE_SHADERS
	glUseProgram(0);
#endif
}

void drawColoredQuad(float x, float y, float w, float h, float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
	glBegin(GL_QUADS);
	glVertex3f(x, y, 0.0f);
	glVertex3f(x + w, y, 0.0f);
	glVertex3f(x + w, y + h, 0.0f);
	glVertex3f(x, y + h, 0.0f);
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void drawWiredQuad(float x, float y, float w, float h, float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
	glBegin(GL_LINE_LOOP);
	glVertex3f(x, y, 0.0f);
	glVertex3f(x + w, y, 0.0f);
	glVertex3f(x + w, y + h, 0.0f);
	glVertex3f(x, y + h, 0.0f);
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void drawTexturedQuad(unsigned int texID, float x, float y, float w, float h, float sw, float sh, float sx, float sy)
{
	glBindTexture(GL_TEXTURE_2D, texID);
	glBegin(GL_QUADS);
	glTexCoord2f(sx, sy);    glVertex3f(x, y, 0.0f);
	glTexCoord2f(sx + sw, sy);    glVertex3f(x + w, y, 0.0f);
	glTexCoord2f(sx + sw, sy + sh); glVertex3f(x + w, y + h, 0.0f);
	glTexCoord2f(sx, sy + sh); glVertex3f(x, y + h, 0.0f);
	glEnd();
}

int main(int argc, char** argv)
{

#ifdef _MSC_VER // detect a msvc++ build and adjust the working directory
	char cwd[513] = { 0 };
	GetCurrentDirectoryA(512, cwd);
	if (strstr(cwd, "msvc"))
	{
		*(strstr(cwd, "msvc")) = 0;
		strcat(cwd, "demos");
		SetCurrentDirectoryA(cwd);
	}
#endif
#ifdef __APPLE__
	ObjCUtil_setCWD();
#endif
	glutInit(&argc, argv);
#ifdef __ZBUFFER
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
#endif
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - windowWidth) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - windowHeight) / 2);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow(window_name.c_str());
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// general init
	theoraplayer::init(1);
#ifdef _USE_WEBM
	clipwebm::init();
#endif
#ifdef _USE_FFMPEG
	clipffmpeg::init();
#endif
#ifdef _USE_AVFOUNDATION
	clipavfoundation::init();
#endif
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutPassiveMotionFunc(motion);
	glutSpecialFunc(keyboard_special);
	glutIdleFunc(display);
	// main loop
	try
	{
		glutMainLoop();
	}
	catch (void*)
	{
	}
	// destroy
	destroy();
#ifdef _USE_AVFOUNDATION
	clipavfoundation::destroy();
#endif
#ifdef _USE_FFMPEG
	clipffmpeg::destroy();
#endif
#ifdef _USE_WEBM
	clipwebm::destroy();
#endif
	theoraplayer::destroy();
	glutDestroyWindow(0);
	return 0;
}
