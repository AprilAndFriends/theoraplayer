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
		foreach (VideoClip::Format, it, videoClipFormats)
		{
			if (stringEndsWith(filename, (*it).extension))
			{
				this->formatName = (*it).name;
				break;
			}
		}
		if (this->formatName == "")
		{
			log("WARNING: Could not determine format for: '" + filename + "'! Loading the file could fail!");
		}
	}

	MemoryDataSource::MemoryDataSource(const std::string& filename)
	{
		this->filename = filename;
		this->data = NULL;
		this->size = 0;
		this->position = 0;
	}

	MemoryDataSource::~MemoryDataSource()
	{
		if (this->data != NULL)
		{
			delete[] this->data;
		}
	}

	// must not be called in the ctor, can throw exceptions
	void MemoryDataSource::_loadFile()
	{
		if (this->data == NULL)
		{
			VideoClip::Format format;
			FILE* file = openSupportedFormatFile(filename, format, this->fullFilename);
			if (file == NULL)
			{
				std::string message = "Can't open or find video file: " + filename;
				log(message);
				throw TheoraplayerException(message);
			}
			this->formatName = format.name;
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
				fclose(file);
				throw TheoraplayerException("MemoryDataSource doesn't support files larger than 4GB!");
			}
			this->data = new unsigned char[(unsigned int)this->size];
			if (this->size < UINT_MAX)
			{
				fread(this->data, 1, (size_t)this->size, file);
			}
			else
			{
				fclose(file);
				throw TheoraplayerException("Unable to preload file to memory, file is too large.");
			}
			fclose(file);
		}
	}

	int MemoryDataSource::read(void* output, int count)
	{
		if (this->size == 0)
		{
			this->_loadFile();
		}
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
