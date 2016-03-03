/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include "TheoraFrameQueue.h"
#include "TheoraVideoFrame.h"
#include "TheoraVideoManager.h"
#include "TheoraUtil.h"

#include "Mutex.h"

using namespace theoraplayer; // TODOth - remove

TheoraFrameQueue::TheoraFrameQueue(TheoraVideoClip* parent)
{
	this->parent = parent;
}

TheoraFrameQueue::~TheoraFrameQueue()
{
	foreach_l (TheoraVideoFrame*, this->queue)
	{
		delete (*it);
	}
	this->queue.clear();
}

TheoraVideoFrame* TheoraFrameQueue::createFrameInstance(TheoraVideoClip* clip)
{
	TheoraVideoFrame* frame = new TheoraVideoFrame(clip);
	if (frame->getBuffer() == NULL) // This can happen if you run out of memory
	{
		delete frame;
		return NULL;
	}
	return frame;
}

void TheoraFrameQueue::setSize(int n)
{
	Mutex::ScopeLock lock(&this->mutex);
	if (this->queue.size() > 0)
	{
		foreach_l (TheoraVideoFrame*, this->queue)
		{
			delete (*it);
		}
		this->queue.clear();
	}
	TheoraVideoFrame* frame;
	for (int i = 0;i < n; ++i)
	{
		frame = createFrameInstance(this->parent);
		if (frame != NULL)
		{
			this->queue.push_back(frame);
		}
		else
		{
			TheoraVideoManager::getSingleton().logMessage("TheoraFrameQueue: unable to create " + str(n) + " frames, out of memory. Created " + str((int) this->queue.size()) + " frames.");
			break;
		}
	}
	lock.release();
}

int TheoraFrameQueue::getSize()
{
	return (int) this->queue.size();
}

TheoraVideoFrame* TheoraFrameQueue::_getFirstAvailableFrame()
{
	TheoraVideoFrame* frame = this->queue.front();
	if (frame->ready)
	{
		return frame;
	}
	else
	{
		return NULL;
	}
}

TheoraVideoFrame* TheoraFrameQueue::getFirstAvailableFrame()
{
	Mutex::ScopeLock lock(&this->mutex);
	TheoraVideoFrame* frame = _getFirstAvailableFrame();
	lock.release();
	return frame;
}

void TheoraFrameQueue::clear()
{
	Mutex::ScopeLock lock(&this->mutex);
	foreach_l (TheoraVideoFrame*, this->queue)
	{
		(*it)->clear();
	}
	lock.release();
}

void TheoraFrameQueue::_pop(int n)
{
	for (int i = 0; i < n; ++i)
	{
		TheoraVideoFrame* first = this->queue.front();
		first->clear();
		this->queue.pop_front();
		this->queue.push_back(first);
	}
}

void TheoraFrameQueue::pop(int n)
{
	Mutex::ScopeLock lock(&this->mutex);
	_pop(n);
	lock.release();
}

TheoraVideoFrame* TheoraFrameQueue::requestEmptyFrame()
{
	TheoraVideoFrame* frame = NULL;
	Mutex::ScopeLock lock(&this->mutex);
	foreach_l (TheoraVideoFrame*, this->queue)
	{
		if (!(*it)->inUse)
		{
			(*it)->inUse = 1;
			(*it)->ready = 0;
			frame = (*it);
			break;
		}
	}
	lock.release();
	return frame;
}

int TheoraFrameQueue::getUsedCount()
{
	Mutex::ScopeLock lock(&this->mutex);
	int n = 0;
	foreach_l (TheoraVideoFrame*, this->queue)
	{
		if ((*it)->inUse)
		{
			++n;
		}
	}
	lock.release();
	return n;
}

int TheoraFrameQueue::_getReadyCount()
{
	int n = 0;
	foreach_l (TheoraVideoFrame*, this->queue)
	{
		if ((*it)->ready)
		{
			++n;
		}
	}
	return n;
}


int TheoraFrameQueue::getReadyCount()
{
	Mutex::ScopeLock lock(&this->mutex);
	int n = _getReadyCount();
	lock.release();
	return n;
}

bool TheoraFrameQueue::isFull()
{
	return (size_t)getReadyCount() == this->queue.size();
}

std::list<TheoraVideoFrame*>& TheoraFrameQueue::_getFrameQueue()
{
	return this->queue;
}
