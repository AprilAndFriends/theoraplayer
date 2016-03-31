/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#import <Foundation/Foundation.h>

void ObjCUtil_setCWD()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    const char* dir = [[[NSBundle mainBundle] resourcePath] UTF8String];
	chdir(dir);
	[pool release];	
}
