/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#ifndef THEORA_ASYNC_H
#define THEORA_ASYNC_H

#include <stdlib.h>
#ifndef _WIN32
#include <pthread.h>
#endif

/// @note Based on hltypes::Thread
class TheoraMutex
{
public:
	class ScopeLock
	{
	public:
		ScopeLock(TheoraMutex* mutex = NULL, bool logUnhandledUnlocks = true);
		~ScopeLock();
		bool acquire(TheoraMutex* mutex);
		bool release();

	protected:
		TheoraMutex* mutex;
		bool logUnhandledUnlocks;
	};

	TheoraMutex();
	~TheoraMutex();
	void lock();
	void unlock();
		
protected:
	void* handle;
		
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
	bool isRunning();
	virtual void execute() = 0;
	void join();
		
protected:
	void* id;
	volatile bool running;

private:
	TheoraMutex runningMutex;		
};

#endif
