/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include <stdio.h>
#include <algorithm>
#include <math.h>
#include <map>
#include "TheoraUtil.h"
#include "TheoraException.h"

#ifdef _WIN32
#include <windows.h>
#pragma warning( disable: 4996 ) // MSVC++
#endif

std::string str(int i)
{
    char s[32];
    sprintf(s,"%d",i);
    return std::string(s);
}

std::string strf(float i)
{
    char s[32];
    sprintf(s,"%.3f",i);
    return std::string(s);
}

void _psleep(int milliseconds)
{
#ifndef _WIN32
    usleep(milliseconds*1000);
#else
	Sleep(milliseconds);
#endif
}


int _nextPow2(int x)
{
	int y;
	for (y=1;y<x;y*=2);
	return y;
}
