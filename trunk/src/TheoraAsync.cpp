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
#include "TheoraAsync.h"

// TODO: platform independence
#include <windows.h>

unsigned long WINAPI theoraAsync_Call(void* param)
{
	TheoraThread* t=(TheoraThread*) param;
	t->executeThread();
	return 0;
}

TheoraMutex::TheoraMutex()
{
	mHandle=0;
}

TheoraMutex::~TheoraMutex()
{
	if (mHandle) CloseHandle(mHandle);
}

void TheoraMutex::lock()
{
	if (!mHandle) mHandle=CreateMutex(0,0,0);
	WaitForSingleObject(mHandle,INFINITE);
}

void TheoraMutex::unlock()
{
	ReleaseMutex(mHandle);
}


TheoraThread::TheoraThread()
{
	mThreadRunning=false;
	mHandle=0;
}

TheoraThread::~TheoraThread()
{
	if (mHandle) CloseHandle(mHandle);
}

void TheoraThread::startThread()
{
	mThreadRunning=true;
	mHandle=CreateThread(0,0,&theoraAsync_Call,this,0,0);
}

void TheoraThread::waitforThread()
{
	mThreadRunning=false;
	WaitForSingleObject(mHandle,INFINITE);
	if (mHandle) { CloseHandle(mHandle); mHandle=0; }
}