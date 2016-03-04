#pragma once
#include "demo_basecode.h"

extern Demo demoMenu;

extern void menu_init();
extern void menu_destroy();
extern void menu_update(float);
extern void menu_draw();
extern void menu_setDebugTitle(char* out);
extern void menu_OnKeyPress(int key);
extern void menu_OnClick(float x, float y);