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

#include "MemoryDataSource.h"
#include "Exception.h"
#include "Manager.h"
#include "theoraplayer.h"
#include "Utility.h"

namespace theoraplayer
{
	MemoryDataSource::MemoryDataSource(unsigned char* data, long size, const std::string& filename)
	{
		this->filename = filename;
		this->data = data;
		this->size = size;
		this->position = 0;
	}

	MemoryDataSource::MemoryDataSource(const std::string& filename)
	{
		this->filename = filename;
		this->data = NULL;
		this->size = 0;
		this->position = 0;
		FILE* file = fopen(filename.c_str(), "rb");
		// TODOth - change this, constructors must not throw exceptions
		if (file == NULL)
		{
			throw TheoraplayerException("Can't open video file: " + filename);
		}
#ifdef _WIN32
		struct _stat64 s;
		_fstati64(_fileno(file), &s);
#else
		struct stat s;
		fstat(fileno(file), &s);
#endif
		this->size = (uint64_t)s.st_size;
		if (this->size > 0xFFFFFFFF)
		{
			throw TheoraplayerException("TheoraMemoryFileDataSource doesn't support files larger than 4GB!");
		}
		this->data = new unsigned char[(unsigned int)this->size];
		if (this->size < UINT_MAX)
		{
			fread(this->data, 1, (size_t)this->size, file);
		}
		else
		{
			// TODOth - change this, constructors must not throw exceptions
			throw TheoraplayerException("Unable to preload file to memory, file is too large.");
		}
		fclose(file);
	}

	MemoryDataSource::~MemoryDataSource()
	{
		if (this->data != NULL)
		{
			delete[] this->data;
		}
	}

	int MemoryDataSource::read(void* output, int count)
	{
		int result = (int)((this->position + count <= this->size) ? count : this->size - this->position);
		if (result > 0)
		{
			memcpy(output, this->data + this->position, result);
			this->position += result;
		}
		return result;
	}

	void MemoryDataSource::seek(uint64_t byteIndex)
	{
		this->position = byteIndex;
	}

}
