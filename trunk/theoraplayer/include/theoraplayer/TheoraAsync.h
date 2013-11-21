/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _TheoraAsync_h
#define _TheoraAsync_h

#ifndef _WIN32
#include <pthread.h>
#endif

/// @note Based on hltypes::Thread
class TheoraMutex
{
public:
	TheoraMutex();
	~TheoraMutex();
	void lock();
	void unlock();
		
protected:
	void* mHandle;
		
};

/// @note Based on hltypes::Thread
class TheoraThread
{
public:
	TheoraThread();
	virtual ~TheoraThread();
	void start();
	void stop();
	void resume();
	void pause();
	virtual void execute() = 0;
	void join();
		
protected:
	void* mId;
	volatile bool mRunning;
		
};

#endif
