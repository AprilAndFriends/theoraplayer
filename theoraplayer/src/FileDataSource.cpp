/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <memory.h>
#include <stdio.h>
#include <sys/stat.h>

#include "FileDataSource.h"
#include "Exception.h"
#include "Manager.h"
#include "theoraplayer.h"
#include "Utility.h"

namespace theoraplayer
{	
	FileDataSource::FileDataSource(const std::string& filename)
	{
		this->filename = filename;
		this->filePtr = NULL;
	}

	FileDataSource::~FileDataSource()
	{
		if (this->filePtr)
		{
			fclose(this->filePtr);
			this->filePtr = NULL;
		}
	}

	void FileDataSource::openFile()
	{
		if (this->filePtr == NULL)
		{
			this->filePtr = fopen(filename.c_str(), "rb");
			if (!this->filePtr)
			{
				std::string message = "Can't open video file: " + this->filename;
				log(message);
				throw TheoraplayerException(message);
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

	int FileDataSource::read(void* output, int nBytes)
	{
		if (this->filePtr == NULL)
		{
			openFile();
		}
		uint64_t n = fread(output, 1, nBytes, this->filePtr);
		return (int) n;
	}

	void FileDataSource::seek(uint64_t byte_index)
	{
		if (this->filePtr == NULL) 
		{
			this->openFile();
		}
#ifdef _LINUX //fpos_t is not a scalar in Linux, for more info refer here: https://code.google.com/p/libtheoraplayer/issues/detail?id=6
		fpos_t fpos = { 0 };
		fpos.__pos = byte_index;
#else
		fpos_t fpos = byte_index;
#endif
		fsetpos(this->filePtr, &fpos);
	}

	uint64_t FileDataSource::getSize()
	{
		if (this->filePtr == NULL)
		{
			this->openFile();
		}
		return this->length;
	}

	uint64_t FileDataSource::getPosition()
	{
		if (this->filePtr == NULL)
		{
			return 0;
		}
#ifdef _LINUX
		fpos_t pos;
		fgetpos(mFilePtr, &pos);
		return (uint64_t)pos.__pos;
#else
		fpos_t pos;
		fgetpos(this->filePtr, &pos);
		return (uint64_t)pos;
#endif
	}
}
