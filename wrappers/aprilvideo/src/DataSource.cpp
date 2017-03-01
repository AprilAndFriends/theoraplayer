/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "DataSource.h"

namespace aprilvideo
{
	DataSource::DataSource(chstr formatName, chstr filename) : theoraplayer::DataSource()
	{
		this->formatName = formatName;
		this->filename = filename;
		this->size = hresource::hinfo(this->filename).size;
	}

	DataSource::~DataSource()
	{
		if (this->resource.isOpen())
		{
			this->resource.close();
		}
	}

	int DataSource::read(void* output, int bytesCount)
	{
		if (!this->resource.isOpen())
		{
			this->_openFile();
		}
		return this->resource.readRaw(output, bytesCount);
	}

	void DataSource::seek(int64_t byteIndex)
	{
		if (!this->resource.isOpen())
		{
			this->_openFile();
		}
		this->resource.seek(byteIndex, hseek::Start);
	}

	int64_t DataSource::getSize()
	{
		return this->size;
	}

	int64_t DataSource::getPosition()
	{
		if (!this->resource.isOpen())
		{
			this->_openFile();
		}
		return this->resource.position();
	}

	void DataSource::_openFile()
	{
		this->resource.open(this->filename);
	}

}
