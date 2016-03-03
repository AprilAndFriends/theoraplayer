/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "TheoraVideoManager.h"

#include "FrameQueue.h"
#include "Mutex.h"
#include "Utility.h"
#include "VideoFrame.h"

namespace theoraplayer
{
	FrameQueue::FrameQueue(TheoraVideoClip* parent)
	{
		this->parent = parent;
	}

	FrameQueue::~FrameQueue()
	{
		foreach_l (VideoFrame*, it, this->queue)
		{
			delete (*it);
		}
		this->queue.clear();
	}

	VideoFrame* FrameQueue::createFrameInstance(TheoraVideoClip* clip)
	{
		VideoFrame* frame = new VideoFrame(clip);
		if (frame->getBuffer() == NULL) // This can happen if you run out of memory
		{
			delete frame;
			return NULL;
		}
		return frame;
	}

	void FrameQueue::setSize(int n)
	{
		Mutex::ScopeLock lock(&this->mutex);
		if (this->queue.size() > 0)
		{
			foreach_l (VideoFrame*, it, this->queue)
			{
				delete (*it);
			}
			this->queue.clear();
		}
		VideoFrame* frame = NULL;
		for (int i = 0; i < n; ++i)
		{
			frame = this->createFrameInstance(this->parent);
			if (frame != NULL)
			{
				this->queue.push_back(frame);
			}
			else
			{
				TheoraVideoManager::getSingleton().logMessage("FrameQueue: unable to create " + str(n) + " frames, out of memory. Created " + str((int) this->queue.size()) + " frames.");
				break;
			}
		}
		lock.release();
	}

	int FrameQueue::getSize()
	{
		return (int) this->queue.size();
	}

	VideoFrame* FrameQueue::_getFirstAvailableFrame()
	{
		VideoFrame* frame = this->queue.front();
		return (frame->ready ? frame : NULL);
	}

	VideoFrame* FrameQueue::getFirstAvailableFrame()
	{
		Mutex::ScopeLock lock(&this->mutex);
		VideoFrame* frame = this->_getFirstAvailableFrame();
		lock.release();
		return frame;
	}

	void FrameQueue::clear()
	{
		Mutex::ScopeLock lock(&this->mutex);
		foreach_l (VideoFrame*, it, this->queue)
		{
			(*it)->clear();
		}
		lock.release();
	}

	void FrameQueue::_pop(int n)
	{
		VideoFrame* first = NULL;
		for (int i = 0; i < n; ++i)
		{
			first = this->queue.front();
			first->clear();
			this->queue.pop_front();
			this->queue.push_back(first);
		}
	}

	void FrameQueue::pop(int n)
	{
		Mutex::ScopeLock lock(&this->mutex);
		this->_pop(n);
		lock.release();
	}

	VideoFrame* FrameQueue::requestEmptyFrame()
	{
		VideoFrame* frame = NULL;
		Mutex::ScopeLock lock(&this->mutex);
		foreach_l (VideoFrame*, it, this->queue)
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

	int FrameQueue::getUsedCount()
	{
		Mutex::ScopeLock lock(&this->mutex);
		int n = 0;
		foreach_l (VideoFrame*, it, this->queue)
		{
			if ((*it)->inUse)
			{
				++n;
			}
		}
		lock.release();
		return n;
	}

	int FrameQueue::_getReadyCount()
	{
		int n = 0;
		foreach_l (VideoFrame*, it, this->queue)
		{
			if ((*it)->ready)
			{
				++n;
			}
		}
		return n;
	}

	int FrameQueue::getReadyCount()
	{
		Mutex::ScopeLock lock(&this->mutex);
		int n = this->_getReadyCount();
		lock.release();
		return n;
	}

	bool FrameQueue::isFull()
	{
		return (size_t)getReadyCount() == this->queue.size();
	}

	std::list<VideoFrame*>& FrameQueue::_getFrameQueue()
	{
		return this->queue;
	}

}
