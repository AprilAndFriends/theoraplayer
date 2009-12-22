/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2009 Kresimir Spes (kreso@cateia.com)

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the 
Free Software Foundation; either version 2 of the License, or (at your option) 
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
*************************************************************************************/
#include "TheoraDataSource.h"
#include "TheoraException.h"

TheoraFileDataSource::TheoraFileDataSource(std::string filename)
{
	mFilename=filename;
	mFilePtr=fopen(filename.c_str(),"rb");
	if (!mFilePtr) throw TheoraGenericException("Can't open video file: "+filename);
	fseek(mFilePtr,0,SEEK_END);
	mSize=ftell(mFilePtr);	
	fseek(mFilePtr,0,SEEK_SET);
}

TheoraFileDataSource::~TheoraFileDataSource()
{
	if (mFilePtr) fclose(mFilePtr);
}

int TheoraFileDataSource::read(void* output,int nBytes)
{
	int n=fread(output,1,nBytes,mFilePtr);
	return nBytes;
}

void TheoraFileDataSource::seek(unsigned long byte_index)
{
	fseek(mFilePtr,byte_index,SEEK_SET);
}

unsigned long TheoraFileDataSource::size()
{
	return mSize;
}

unsigned long TheoraFileDataSource::tell()
{
	return ftell(mFilePtr);
}
