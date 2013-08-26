/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include <stdio.h>
#include "TheoraAsync.h"
#include "TheoraUtil.h"

#ifdef _WIN32
#include <windows.h>

unsigned long WINAPI theoraAsync_Call(void* param)
{
#else
void *theoraAsync_Call(void* param)
{
#endif

	TheoraThread* t = (TheoraThread*) param;
	t->executeThread();
#ifndef _WIN32
    pthread_exit(NULL);
#endif
	return 0;
}


TheoraMutex::TheoraMutex()
{
#ifdef _WIN32
	mHandle = 0;
#else
    pthread_mutex_init(&mHandle, 0);
#endif
}

TheoraMutex::~TheoraMutex()
{
#ifdef _WIN32
	if (mHandle) CloseHandle(mHandle);
#else
    pthread_mutex_destroy(&mHandle);
#endif
}

void TheoraMutex::lock()
{
#ifdef _WIN32
	if (!mHandle)
	{
#ifndef _WINRT // WinXP does not have CreateMutexEx()
		mHandle = CreateMutex(0, 0, 0);
#else
		mHandle = CreateMutexEx(NULL, NULL, 0, SYNCHRONIZE);
#endif
	}
	WaitForSingleObjectEx(mHandle, INFINITE, FALSE);
#else
	pthread_mutex_lock(&mHandle);
#endif
}

void TheoraMutex::unlock()
{
#ifdef _WIN32
	ReleaseMutex(mHandle);
#else
    pthread_mutex_unlock(&mHandle);
#endif
}


#ifdef _WINRT
using namespace Windows::Foundation;
using namespace Windows::System::Threading;

struct TheoraAsyncActionWrapper
{
public:
	IAsyncAction^ async_action;
	TheoraAsyncActionWrapper(IAsyncAction^ async_action)
	{
		this->async_action = async_action;
	}
};

#endif

TheoraThread::TheoraThread()
{
	mThreadRunning = false;
	mHandle = 0;
}

TheoraThread::~TheoraThread()
{
#ifdef _WIN32
	if (mHandle) CloseHandle(mHandle);
#endif
}

void TheoraThread::startThread()
{
	mThreadRunning = true;
#ifdef _WIN32
#ifndef _WINRT
	mHandle = CreateThread(0, 0, &theoraAsync_Call, this, 0, 0);
#else
	mHandle = new TheoraAsyncActionWrapper(ThreadPool::RunAsync(
	ref new WorkItemHandler([&](IAsyncAction^ work_item)
	{
		this->executeThread();
	}),
	WorkItemPriority::Normal, WorkItemOptions::TimeSliced));
#endif
#else
    int ret=pthread_create(&mHandle, NULL, &theoraAsync_Call, this);
    if (ret) printf("ERROR: Unable to create thread!\n"); // <-- TODO: log this, rather then using printf, and remove stdio include.
#endif
}

void TheoraThread::waitforThread()
{
	mThreadRunning = false;
#ifdef _WIN32
#ifndef _WINRT
	WaitForSingleObject(mHandle, INFINITE);
	if (mHandle != 0)
	{
		CloseHandle(mHandle);
		mHandle = 0;
	}
#else
	IAsyncAction^ action = ((TheoraAsyncActionWrapper*)mHandle)->async_action;
	int i = 0;
	while (action->Status != AsyncStatus::Completed &&
		action->Status != AsyncStatus::Canceled &&
		action->Status != AsyncStatus::Error &&
		i < 100)
	{
		_psleep(50);
		i++;
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
			i++;
		}
	}
	if (mHandle != NULL)
	{
		delete mHandle;
		mHandle = NULL;
	}
#endif
#else
    pthread_join(mHandle, 0);
#endif
}
