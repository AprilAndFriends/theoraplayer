/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include "DataSource.h"

AprilVideoDataSource::AprilVideoDataSource(hstr filename)
{
	this->filename = filename;
	this->resource.open(filename);
	
	this->size = (unsigned long) this->resource.size();
}

AprilVideoDataSource::~AprilVideoDataSource()
{
	this->resource.close();
}

int AprilVideoDataSource::read(void* output, int nBytes)
{
	return this->resource.readRaw(output, nBytes);
}

void AprilVideoDataSource::seek(uint64_t byte_index)
{
	this->resource.seek((long) byte_index, hresource::START);
}

uint64_t AprilVideoDataSource::getSize()
{
	return this->size;
}

uint64_t AprilVideoDataSource::getPosition()
{
	return this->resource.position();
}
