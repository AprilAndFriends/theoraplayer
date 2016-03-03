/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "Mutex.h"
#include "WorkerThread.h"

#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"

#ifdef _THREAD_NAMING
static int threadCounter = 1;
static Mutex counterMutex;
#endif

namespace theoraplayer
{
	WorkerThread::WorkerThread() : Thread(&WorkerThread::_work)
	{
		this->clip = NULL;
	}

	WorkerThread::~WorkerThread()
	{
	}

	void WorkerThread::_work(Thread* thread)
	{
		WorkerThread* self = (WorkerThread*)thread;
		Mutex::ScopeLock lock;
#ifdef _THREAD_NAMING
		{
			lock.acquire(&counterMutex);
			char name[64] = { 0 };
			sprintf(name, "WorkerThread %d", threadCounter++);
			pthread_setname_np(name);
			lock.release();
		}
#endif
		while (self->executing)
		{
			self->clip = TheoraVideoManager::getSingleton().requestWork(self);
			if (!self->clip)
			{
				Thread::sleep(100.0f);
				continue;
			}
			lock.acquire(self->clip->threadAccessMutex);
			// if user requested seeking, do that then.
			if (self->clip->seekFrame >= 0)
			{
				self->clip->doSeek();
			}
			if (!self->clip->decodeNextFrame())
			{
				Thread::sleep(1.0f); // this happens when the video frame queue is full.
			}
			self->clip->assignedWorkerThread = NULL;
			lock.release();
			self->clip = NULL;
		}
	}

}