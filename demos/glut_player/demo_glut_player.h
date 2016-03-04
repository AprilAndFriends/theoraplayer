#pragma once
#include "demo_basecode.h"

#include <theoraplayer/TheoraPlayer.h>
#include <theoraplayer/DataSource.h>

//#define BENCHMARK // uncomment this to benchmark decoding times
#ifdef BENCHMARK
#include <time.h>
#include <theoraplayer/TheoraFrameQueue.h>
#endif

extern void glutplayer_init();
extern void glutplayer_destroy();
extern void glutplayer_update(float);
extern void glutplayer_draw();
extern void glutplayer_setDebugTitle(char* out);
extern void glutplayer_OnKeyPress(int key);
extern void glutplayer_OnClick(float x, float y);
#ifdef BENCHMARK
	extern void glutplayer_benchmark(const char* filename);
#endif