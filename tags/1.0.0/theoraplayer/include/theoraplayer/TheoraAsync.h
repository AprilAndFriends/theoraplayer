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

/**
    This is a Mutex object, used in thread syncronization.
 */
class TheoraMutex
{
protected:
#ifdef _WIN32
	void* mHandle;
#else
    pthread_mutex_t mHandle;
#endif
public:
	TheoraMutex();
	~TheoraMutex();
	//! Lock the mutex. If another thread has lock, the caller thread will wait until the previous thread unlocks it
	void lock();
	//! Unlock the mutex. Use this when you're done with thread-safe sections of your code
	void unlock();
};

/**
    This is a Mutex object, used in thread syncronization.
 */
class TheoraThread
{
protected:
#ifdef _WIN32
	void* mHandle;
#else
    pthread_t mHandle;
#endif
	//! Indicates whether the thread is running. As long as this is true, the thread runs in a loop
	volatile bool mThreadRunning;
public:
	TheoraThread();
	virtual ~TheoraThread();

	//! Creates the thread object and runs it
	void startThread();
	//! The main thread loop function
	virtual void executeThread()=0;
	//! sets mThreadRunning to false and waits for the thread to complete the last cycle
	void waitforThread();

};

#endif
