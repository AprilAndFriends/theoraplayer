/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Provides an interface for a video data source.

#ifndef THEORAPLAYER_MEMORY_DATA_SOURCE_H
#define THEORAPLAYER_MEMORY_DATA_SOURCE_H

#include "DataSource.h"
#include "theoraplayerExport.h"

// TODOth - split these into separate files

namespace theoraplayer
{
	/**
		Pre-loads the entire file and streams from memory.
		Very useful if you're continuously displaying a video and want to avoid disk reads.
		Not very practical for large files.
	*/
	class theoraplayerExport MemoryDataSource : public DataSource
	{
	public:
		MemoryDataSource(unsigned char* data, long size, const std::string& filename = "memory");
		MemoryDataSource(std::string filename);
		~MemoryDataSource();

		uint64_t getSize();
		uint64_t getPosition();
		std::string getFilename() { return this->filename; }

		int read(void* output, int bytes);
		void seek(uint64_t byte_index);

		inline std::string toString() { return "MEM:" + this->filename; }

	private:
		std::string filename;
		uint64_t length;
		uint64_t readPointer;
		unsigned char* data;

	};
}
#endif
