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
/// Provides an interface for a generic video data source.

#ifndef THEORAPLAYER_DATA_SOURCE_H
#define THEORAPLAYER_DATA_SOURCE_H

#include <stdint.h>
#include <string>

#include "theoraplayerExport.h"

namespace theoraplayer
{
	/**
		This is a simple class that provides abstracted data feeding. You can use the
		TheoraFileDataSource for regular file playback or you can implement your own
		internet streaming solution, or a class that uses encrypted datafiles etc.
		The sky is the limit
	*/
	class theoraplayerExport DataSource
	{
	public:
		DataSource();
		virtual ~DataSource();
		/**
			Reads nBytes bytes from data source and returns number of read bytes.
			if function returns less bytes then nBytes, the system assumes EOF is reached.
		*/
		virtual int read(void* output, int nBytes) = 0;
		//! returns a string representation of the DataSource, eg 'File: source.ogg'
		virtual std::string toString() = 0;
		//! position the source pointer to byte_index from the start of the source
		virtual void seek(uint64_t byte_index) = 0;
		//! return the size of the stream in bytes
		virtual uint64_t getSize() = 0;
		//! return the current position of the source pointer
		virtual uint64_t getPosition() = 0;

	};

}
#endif
