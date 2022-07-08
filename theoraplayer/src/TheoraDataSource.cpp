/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include <stdio.h>
#include <sys/stat.h>
#include <memory.h>
#include "TheoraDataSource.h"
#include "TheoraException.h"
#include "TheoraVideoManager.h"
#include "TheoraUtil.h"

TheoraDataSource::~TheoraDataSource()
{

}

TheoraFileDataSource::TheoraFileDataSource(std::string filename)
{
	this->filename = filename;
	this->filePtr = NULL;
}

TheoraFileDataSource::~TheoraFileDataSource()
{
	if (this->filePtr)
	{
		fclose(this->filePtr);
		this->filePtr = NULL;
	}
}

void TheoraFileDataSource::openFile()
{
	if (this->filePtr == NULL)
	{
		this->filePtr = fopen(filename.c_str(), "rb");
		if (!this->filePtr)
		{
			std::string msg = "Can't open video file: " + this->filename;
			th_writelog(msg);
			throw TheoraGenericException(msg);
		}

		
#ifdef _WIN32
		struct _stat64 s;
		_fstati64(_fileno(this->filePtr), &s);
#else
		struct stat s;
		fstat(fileno(this->filePtr), &s);
#endif
		this->length = (uint64_t)s.st_size;
	}
}

int TheoraFileDataSource::read(void* output, int nBytes)
{
	if (this->filePtr == NULL)
	{
		openFile();
	}
	uint64_t n = fread(output, 1, nBytes, this->filePtr);
	return (int) n;
}

void TheoraFileDataSource::seek(uint64_t byte_index)
{
	if (this->filePtr == NULL) 
	{
		openFile();
	}
#ifdef _LINUX //fpos_t is not a scalar in Linux, for more info refer here: https://code.google.com/p/libtheoraplayer/issues/detail?id=6
	fpos_t fpos = { 0 };
	fpos.__pos = byte_index;
#else
	fpos_t fpos = byte_index;
#endif
	fsetpos(this->filePtr, &fpos);
}

uint64_t TheoraFileDataSource::getSize()
{
	if (this->filePtr == NULL)
	{
		openFile();
	}
	return this->length;
}

uint64_t TheoraFileDataSource::getPosition()
{
	if (this->filePtr == NULL) 
	{
		return 0;
	}
#ifdef _LINUX
	fpos_t pos;
	fgetpos(this->filePtr, &pos);
	return (uint64_t) pos.__pos;
#else
	fpos_t pos;
	fgetpos(this->filePtr, &pos);
	return (uint64_t) pos;
#endif
}

TheoraMemoryFileDataSource::TheoraMemoryFileDataSource(std::string filename) :
	readPointer(0),
	data(0)
{
	this->filename = filename;
	FILE* f = fopen(this->filename.c_str(),"rb");
	if (!f) 
	{
		throw TheoraGenericException("Can't open video file: " + this->filename);
	}

#ifdef _WIN32
	struct _stat64 s;
	_fstati64(_fileno(f), &s);
#else
	struct stat s;
	fstat(fileno(f), &s);
#endif
	this->length = (uint64_t)s.st_size;
	if (this->length > 0xFFFFFFFF)
	{
		throw TheoraGenericException("TheoraMemoryFileDataSource doesn't support files larger than 4GB!");
	}
	this->data = new unsigned char[(unsigned int) this->length];
	if (this->length < UINT_MAX)
	{
		fread(this->data, 1, (size_t) this->length, f);
	}
	else
	{
		throw TheoraGenericException("Unable to preload file to memory, file is too large.");
	}

	fclose(f);
}

TheoraMemoryFileDataSource::TheoraMemoryFileDataSource(unsigned char* data, long size, const std::string& filename)
{
	this->filename = filename;
	this->data = data;
	this->length = size;
	this->readPointer = 0;
}

TheoraMemoryFileDataSource::~TheoraMemoryFileDataSource()
{
	if (this->data) 
	{
		delete[] this->data;
	}
}

int TheoraMemoryFileDataSource::read(void* output, int nBytes)
{
	int n = (int)((this->readPointer + nBytes <= this->length) ? nBytes : this->length - this->readPointer);
	if (!n)
	{
		return 0;
	}
	memcpy(output, this->data + this->readPointer, n);
	this->readPointer += n;
	return n;
}

void TheoraMemoryFileDataSource::seek(uint64_t byte_index)
{
	this->readPointer = byte_index;
}

uint64_t TheoraMemoryFileDataSource::getSize()
{
	return this->length;
}

uint64_t TheoraMemoryFileDataSource::getPosition()
{
	return this->readPointer;
}
