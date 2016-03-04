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
