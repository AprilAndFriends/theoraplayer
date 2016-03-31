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
/// Provides an interface for a video data source from a file.

#ifndef THEORAPLAYER_FILE_DATA_SOURCE_H
#define THEORAPLAYER_FILE_DATA_SOURCE_H

#include "DataSource.h"
#include "theoraplayerExport.h"

namespace theoraplayer
{	
	/// @brief Provides file IO through standard C-API.
	class theoraplayerExport FileDataSource : public DataSource
	{
	public:
		FileDataSource(const std::string& filename);
		~FileDataSource();

		inline std::string getFormatName() { return this->formatName; }
		inline std::string getFilename() { return this->filename; }
		uint64_t getSize();
		uint64_t getPosition();

		int read(void* output, int nBytes);
		void seek(uint64_t byte_index);

		inline std::string toString() { return this->filename + " (format: " + this->formatName + ")"; }

	private:
		FILE* filePtr;
		std::string filename;
		std::string fullFilename;
		std::string formatName;
		uint64_t length;

		void _openFile();

	};

}
#endif
