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

#include "DataSource.h"
#include "Exception.h"
#include "Manager.h"
#include "theoraplayer.h"
#include "Utility.h"

namespace theoraplayer
{
	DataSource::DataSource()
	{
	}

	DataSource::~DataSource()
	{
	}

	FileDataSource::FileDataSource(std::string filename)
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

	uint64_t FileDataSource::getSize()
	{
		if (this->filePtr == NULL)
		{
			openFile();
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

	MemoryDataSource::MemoryDataSource(std::string filename) : readPointer(0), data(0)
	{
		this->filename = filename;
		FILE* f = fopen(this->filename.c_str(), "rb");
		if (f == NULL)
		{
			throw TheoraplayerException("Can't open video file: " + this->filename);
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
			throw TheoraplayerException("MemoryDataSource doesn't support files larger than 4GB!");
		}
		this->data = new unsigned char[(unsigned int) this->length];
		if (this->length < UINT_MAX)
		{
			fread(this->data, 1, (size_t) this->length, f);
		}
		else
		{
			throw TheoraplayerException("Unable to preload file to memory, file is too large.");
		}
		fclose(f);
	}

	MemoryDataSource::MemoryDataSource(unsigned char* data, long size, const std::string& filename)
	{
		this->filename = filename;
		this->data = data;
		this->length = size;
		this->readPointer = 0;
	}

	MemoryDataSource::~MemoryDataSource()
	{
		if (this->data)
		{
			delete[] this->data;
		}
	}

	int MemoryDataSource::read(void* output, int nBytes)
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

	void MemoryDataSource::seek(uint64_t byte_index)
	{
		this->readPointer = byte_index;
	}

	uint64_t MemoryDataSource::getSize()
	{
		return this->length;
	}

	uint64_t MemoryDataSource::getPosition()
	{
		return this->readPointer;
	}

}
