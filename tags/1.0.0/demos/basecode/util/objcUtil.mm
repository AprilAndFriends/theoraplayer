/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#import <Foundation/Foundation.h>

#include <sys/time.h>
unsigned long GetTickCount()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) return 0;
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void ObjCUtil_setCWD()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    const char* dir = [[[NSBundle mainBundle] resourcePath] UTF8String];
	chdir(dir);
	[pool release];	
}
