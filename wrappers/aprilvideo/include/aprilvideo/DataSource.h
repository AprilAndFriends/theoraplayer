/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef APRILVIDEO_DATA_SOURCE_H
#define APRILVIDEO_DATA_SOURCE_H

#include <hltypes/hstring.h>
#include <hltypes/hresource.h>
#include <theoraplayer/TheoraDataSource.h>

class AprilVideoDataSource : public TheoraDataSource
{
	hresource mResource;
	hstr mFilename;
	unsigned long mSize;
public:
	AprilVideoDataSource(hstr filename);
	~AprilVideoDataSource();
	
	int read(void* output, int nBytes);
	void seek(unsigned long byte_index);
	std::string repr() { return "HRESOURCE:" + mFilename; }
	unsigned long size();
	unsigned long tell();
	hstr getFilename() { return mFilename; }
};

#endif
