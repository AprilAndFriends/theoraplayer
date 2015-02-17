/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#ifndef APRILVIDEO_DATA_SOURCE_H
#define APRILVIDEO_DATA_SOURCE_H

#include <stdint.h>
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
	void seek(uint64_t byte_index);
	std::string repr() { return ("HRESOURCE:" + mFilename).cStr(); }
	uint64_t size();
	uint64_t tell();
	hstr getFilename() { return mFilename; }
};

#endif
