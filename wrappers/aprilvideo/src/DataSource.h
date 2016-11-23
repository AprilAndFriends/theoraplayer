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
/// Provides a data source for videos.

#ifndef APRILVIDEO_DATA_SOURCE_H
#define APRILVIDEO_DATA_SOURCE_H

#include <stdint.h>

#include <hltypes/hstring.h>
#include <hltypes/hresource.h>
#include <theoraplayer/DataSource.h>

namespace aprilvideo
{
	class DataSource : public theoraplayer::DataSource
	{
	public:
		DataSource(chstr formatName, chstr filename);
		~DataSource();

		inline hstr getFilename() const { return this->filename; }
		inline std::string getFormatName() const { return this->formatName.cStr(); }
		int64_t getSize();
		int64_t getPosition();

		int read(void* output, int bytesCount);
		void seek(int64_t byteIndex);

		std::string toString() const { return ("HRESOURCE:" + this->filename + " (format: " + this->formatName + ")").cStr(); }

	protected:
		hresource resource;
		hstr filename;
		hstr formatName;
		int64_t size;

		void _openFile();

	};

}
#endif
