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
#ifndef _TheoraDataSource_h
#define _TheoraDataSource_h

struct FILE;
#include <string>

/**
	This is a simple class that provides abstracted data feeding. You can use the
	TheoraFileDataSource for regular file playback or you can implement your own
	internet streaming solution, or a class that uses encrypted datafiles etc.
	The sky is the limit
*/
class TheoraDataSource
{
public:
	TheoraDataSource();
	
	/**
		Reads nBytes bytes from data source and returns number of read bytes.
		if function returns less bytes then nBytes, the system assumes EOF is reached.
	*/
	virtual int read(int nBytes);
	
};


/**
	provides standard file IO
*/
class TheoraFileDataSource : public TheoraDataSource
{
	FILE* mFilePtr;
public:
	TheoraFileDataSource(std::string filename);
	
	int read(int nBytes);
};



#endif