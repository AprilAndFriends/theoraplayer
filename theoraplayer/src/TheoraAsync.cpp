/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include "TheoraAsync.h"
#include "TheoraUtil.h"
#include "TheoraVideoManager.h"

#ifdef _WINRT
#include <wrl.h>
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex
///////////////////////////////////////////////////////////////////////////////////////////////////

TheoraMutex::TheoraMutex()
{
#ifdef _WIN32
#ifndef _WINRT // WinXP does not have CreateTheoraMutexEx()
	this->handle = CreateMutex(0, 0, 0);
#else
	this->handle = CreateMutexEx(NULL, NULL, 0, SYNCHRONIZE);
#endif
#else
	this->handle = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init((pthread_mutex_t*)this->handle, 0);
#endif
}

TheoraMutex::~TheoraMutex()
{
#ifdef _WIN32
	CloseHandle(this->handle);
#else
	pthread_mutex_destroy((pthread_mutex_t*)this->handle);
	free((pthread_mutex_t*)this->handle);
	this->handle = NULL;
#endif
}

void TheoraMutex::lock()
{
#ifdef _WIN32
	WaitForSingleObjectEx(this->handle, INFINITE, FALSE);
#else
	pthread_mutex_lock((pthread_mutex_t*)this->handle);
#endif
}

void TheoraMutex::unlock()
{
#ifdef _WIN32
	ReleaseMutex(this->handle);
#else
	pthread_mutex_unlock((pthread_mutex_t*)this->handle);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ScopeLock
///////////////////////////////////////////////////////////////////////////////////////////////////

TheoraMutex::ScopeLock::ScopeLock(TheoraMutex* mutex, bool logUnhandledUnlocks) : mutex(NULL)
{
	this->logUnhandledUnlocks = logUnhandledUnlocks;
	acquire(mutex);
}

TheoraMutex::ScopeLock::~ScopeLock()
{
	if (release() && this->logUnhandledUnlocks)
	{
		th_writelog("A mutex has been scope-unlocked automatically!");
	}
}

bool TheoraMutex::ScopeLock::acquire(TheoraMutex* mutex)
{
	if (this->mutex == NULL && mutex != NULL)
	{
		this->mutex = mutex;
		this->mutex->lock();
		return true;
	}
	return false;
}

bool TheoraMutex::ScopeLock::release()
{
	if (this->mutex != NULL)
	{
		this->mutex->unlock();
		this->mutex = NULL;
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Thread
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WINRT
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
#endif

#ifdef _WIN32
unsigned long WINAPI theoraAsyncCall(void* param)
#else
void* theoraAsyncCall(void* param)
#endif
{
	TheoraThread* t = (TheoraThread*)param;
	t->execute();
#ifdef _WIN32
	return 0;
#else
	pthread_exit(NULL);
	return NULL;
#endif
}

#ifdef _WINRT
struct TheoraAsyncActionWrapper
{
public:
	IAsyncAction^ asyncAction;
	TheoraAsyncActionWrapper(IAsyncAction^ asyncAction)
	{
		this->asyncAction = asyncAction;
	}
};
#endif
	
TheoraThread::TheoraThread() : id(0), running(false)
{
#ifndef _WIN32
	this->id = (pthread_t*)malloc(sizeof(pthread_t));
#endif
}

TheoraThread::~TheoraThread()
{
	TheoraMutex::ScopeLock lock(&this->runningMutex);
	bool running = this->running;
	lock.release();
	if (running)
	{
		stop();
	}
	if (this->id != NULL)
	{
#ifdef _WIN32
#ifndef _WINRT
		CloseHandle(this->id);
#else
		delete this->id;
#endif
#else
		free((pthread_t*)this->id);
#endif
		this->id = NULL;
	}
}

void TheoraThread::start()
{
	TheoraMutex::ScopeLock lock(&this->runningMutex);
	this->running = true;
	lock.release();
#ifdef _WIN32
#ifndef _WINRT
	this->id = CreateThread(0, 0, &theoraAsyncCall, this, 0, 0);
#else
	this->id = new TheoraAsyncActionWrapper(ThreadPool::RunAsync(
		ref new WorkItemHandler([&](IAsyncAction^ work_item)
		{
			execute();
		}),
		WorkItemPriority::Normal, WorkItemOptions::TimeSliced));
#endif
#else
	pthread_create((pthread_t*)this->id, NULL, &theoraAsyncCall, this);
#endif
}

bool TheoraThread::isRunning()
{
	TheoraMutex::ScopeLock lock(&this->runningMutex);
	bool result = this->running;
	lock.release();
	return result;
}

void TheoraThread::join()
{
	TheoraMutex::ScopeLock lock(&this->runningMutex);
	this->running = false;
	lock.release();
#ifdef _WIN32
#ifndef _WINRT
	WaitForSingleObject(this->id, INFINITE);
	if (this->id != NULL)
	{
		CloseHandle(this->id);
		this->id = NULL;
	}
#else
	IAsyncAction^ action = ((TheoraAsyncActionWrapper*)this->id)->asyncAction;
	int i = 0;
	while (action->Status != AsyncStatus::Completed &&
		action->Status != AsyncStatus::Canceled &&
		action->Status != AsyncStatus::Error &&
		i < 100)
	{
		_psleep(50);
		++i;
	}
	if (i >= 100)
	{
		i = 0;
		action->Cancel();
		while (action->Status != AsyncStatus::Completed &&
			action->Status != AsyncStatus::Canceled &&
			action->Status != AsyncStatus::Error &&
			i < 100)
		{
			_psleep(50);
			++i;
		}
	}
#endif
#else
	pthread_join(*((pthread_t*)this->id), 0);
#endif
}
	
void TheoraThread::resume()
{
#ifdef _WIN32
#ifndef _WINRT
	ResumeThread(this->id);
#else
	// not available in WinRT
#endif
#endif
}
	
void TheoraThread::pause()
{
#ifdef _WIN32
#ifndef _WINRT
	SuspendThread(this->id);
#else
	// not available in WinRT
#endif
#endif
}
	
void TheoraThread::stop()
{
	bool stop = false;
	TheoraMutex::ScopeLock lock(&this->runningMutex);
	if (this->running)
	{
		this->running = false;
		stop = true;
	}
	lock.release();
	if (stop)
	{
#ifdef _WIN32
#ifndef _WINRT
		TerminateThread(this->id, 0);
#else
		((TheoraAsyncActionWrapper*)this->id)->asyncAction->Cancel();
#endif
#elif defined(_ANDROID)
		pthread_kill(*((pthread_t*)this->id), 0);
#else
		pthread_cancel(*((pthread_t*)this->id));
#endif
	}
}
	
