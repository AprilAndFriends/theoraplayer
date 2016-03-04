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
/// Implements a reader for MKV files.

#ifndef THEORAPLAYER_MKV_READER_H
#define THEORAPLAYER_MKV_READER_H

#include <mkvparser.hpp>

namespace theoraplayer
{
	class DataSource;

	class MkvReader : public mkvparser::IMkvReader
	{
	public:
		explicit MkvReader(DataSource* dataSource);
		virtual ~MkvReader();

		// dictated by the interface
		virtual int Read(long long position, long length, unsigned char* buffer);
		virtual int Length(long long* total, long long* available);

	protected:
		DataSource* dataSource;
		int length;

		MkvReader(const MkvReader&);
		MkvReader& operator=(const MkvReader&);

	};

}
#endif
