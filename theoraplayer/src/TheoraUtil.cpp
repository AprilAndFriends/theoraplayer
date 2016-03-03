/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

// TODOth - maybe move this to Utility.cpp

#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#include <pthread.h>
#endif

#ifdef _WIN32
#include <windows.h>
#pragma warning( disable: 4996 ) // MSVC++
#endif

#include "TheoraUtil.h"
#include "TheoraException.h"

std::string str(int i)
{
	char s[32];
	sprintf(s, "%d", i);
	return std::string(s);
}

std::string strf(float i)
{
	char s[32];
	sprintf(s, "%.3f", i);
	return std::string(s);
}

void _psleep(int miliseconds)
{
#ifdef _WIN32
#ifndef _WINRT
	Sleep(miliseconds);
#else
	WaitForSingleObjectEx(GetCurrentThread(), miliseconds, 0);
#endif
#else
	usleep(miliseconds * 1000);
#endif
}


int _nextPow2(int x)
{
	int y;
	for (y = 1; y < x; y *= 2);
	return y;
}
