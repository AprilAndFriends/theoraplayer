/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _YUV_C_h
#define _YUV_C_h

#include "TheoraPixelTransform.h"

extern int YTable [256];
extern int BUTable[256];
extern int GUTable[256];
extern int GVTable[256];
extern int RVTable[256];

struct TheoraPixelTransform* incOut(struct TheoraPixelTransform* t, int n);
void _decodeAlpha(struct TheoraPixelTransform* t, int stride);

#endif
