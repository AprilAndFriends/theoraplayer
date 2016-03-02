/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#ifndef THEORA_WORKER_THREAD_H
#define THEORA_WORKER_THREAD_H

#include "TheoraAsync.h"

class TheoraVideoClip;

/**
	This is the worker thread, requests work from TheoraVideoManager
	and decodes assigned TheoraVideoClip objects
*/
class TheoraWorkerThread : public TheoraThread
{
	TheoraVideoClip* clip;
public:
	TheoraWorkerThread();
	~TheoraWorkerThread();

	TheoraVideoClip* getAssignedClip() { return this->clip; }

	//! Main Thread Body - do not call directly!
	void execute();
};
#endif
