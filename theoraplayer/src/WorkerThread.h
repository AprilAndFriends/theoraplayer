/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines a worker thread.

#ifndef THEORAPLAYER_WORKER_THREAD_H
#define THEORAPLAYER_WORKER_THREAD_H

#include "Mutex.h"
#include "Thread.h"

class TheoraVideoClip;

namespace theoraplayer
{
	/**
		This is the worker thread, requests work from TheoraVideoManager
		and decodes assigned TheoraVideoClip objects
	*/
	class WorkerThread : public Thread
	{
		TheoraVideoClip* clip;
	public:
		WorkerThread();
		~WorkerThread();

		TheoraVideoClip* getAssignedClip() { return this->clip; }

	protected:
		static void _work(Thread* thread);

	};

}
#endif
