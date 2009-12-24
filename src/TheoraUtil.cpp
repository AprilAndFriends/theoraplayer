#include <algorithm>
#include <math.h>
#include <map>
#include "TheoraUtil.h"
#include "TheoraException.h"
#include <windows.h>

#pragma warning( disable: 4996 ) // MSVC++

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
	Sleep(milliseconds);
}