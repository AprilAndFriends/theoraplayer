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
	if (!mHandle) mHandle=CreateMutex(0, 0, 0);
	WaitForSingleObject(mHandle, INFINITE);
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
	mHandle=CreateThread(0, 0, &theoraAsync_Call, this, 0, 0);
#else
    int ret=pthread_create(&mHandle, NULL, &theoraAsync_Call, this);
    if (ret) printf("ERROR: Unable to create thread!\n"); // <-- TODO: log this, rather then using printf, and remove stdio include.
#endif
}

void TheoraThread::waitforThread()
{
	mThreadRunning = false;
#ifdef _WIN32
	WaitForSingleObject(mHandle, INFINITE);
	if (mHandle) { CloseHandle(mHandle); mHandle = 0; }
#else
    pthread_join(mHandle, 0);
#endif
}
