/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _TheoraDataSource_h
#define _TheoraDataSource_h

#include <stdio.h>
#include <string>
#include "TheoraExport.h"

/**
	This is a simple class that provides abstracted data feeding. You can use the
	TheoraFileDataSource for regular file playback or you can implement your own
	internet streaming solution, or a class that uses encrypted datafiles etc.
	The sky is the limit
*/
class TheoraPlayerExport TheoraDataSource
{
public:

    virtual ~TheoraDataSource();
	/**
		Reads nBytes bytes from data source and returns number of read bytes.
		if function returns less bytes then nBytes, the system assumes EOF is reached.
	*/
	virtual int read(void* output,int nBytes)=0;
    //! returns a string representation of the DataSource, eg 'File: source.ogg'
	virtual std::string repr()=0;
	//! position the source pointer to byte_index from the start of the source
	virtual void seek(unsigned long byte_index)=0;
	//! return the size of the stream in bytes
	virtual unsigned long size()=0;
	//! return the current position of the source pointer
	virtual unsigned long tell()=0;
};


/**
	provides standard file IO
*/
class TheoraPlayerExport TheoraFileDataSource : public TheoraDataSource
{
	FILE* mFilePtr;
	std::string mFilename;
	unsigned long mSize;
public:
	TheoraFileDataSource(std::string filename);
	~TheoraFileDataSource();

	int read(void* output,int nBytes);
	void seek(unsigned long byte_index);
	std::string repr() { return mFilename; }
	unsigned long size();
	unsigned long tell();
};

/**
	Pre-loads the entire file and streams from memory.
	Very useful if you're continuously displaying a video and want to avoid disk reads.
	Not very practical for large files.
*/
class TheoraPlayerExport TheoraMemoryFileDataSource : public TheoraDataSource
{
	std::string mFilename;
	unsigned long mSize,mReadPointer;
	unsigned char* mData;
public:
	TheoraMemoryFileDataSource(std::string filename);
	~TheoraMemoryFileDataSource();

	int read(void* output,int nBytes);
	void seek(unsigned long byte_index);
	std::string repr() { return "MEM:"+mFilename; }
	unsigned long size();
	unsigned long tell();
};

#endif
