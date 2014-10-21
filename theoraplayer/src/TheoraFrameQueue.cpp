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


TheoraFrameQueue::TheoraFrameQueue(TheoraVideoClip* parent)
{
	mParent = parent;
}

TheoraFrameQueue::~TheoraFrameQueue()
{
	foreach_l (TheoraVideoFrame*, mQueue)
	{
		delete (*it);
	}
	mQueue.clear();
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
	TheoraMutex::ScopeLock mutex(&mMutex);
	if (mQueue.size() > 0)
	{
		foreach_l (TheoraVideoFrame*, mQueue)
		{
			delete (*it);
		}
		mQueue.clear();
	}
	TheoraVideoFrame* frame;
	for (int i = 0;i < n; ++i)
	{
		frame = createFrameInstance(mParent);
		if (frame != NULL) mQueue.push_back(frame);
		else
		{
			TheoraVideoManager::getSingleton().logMessage("TheoraFrameQueue: unable to create " + str(n) + " frames, out of memory. Created " + str((int) mQueue.size()) + " frames.");
			break;
		}
	}
	mutex.release();
}

int TheoraFrameQueue::getSize()
{
	return (int) mQueue.size();
}

TheoraVideoFrame* TheoraFrameQueue::_getFirstAvailableFrame()
{
	TheoraVideoFrame* frame = mQueue.front();
	if (frame->mReady) return frame;
	else               return NULL;
}

TheoraVideoFrame* TheoraFrameQueue::getFirstAvailableFrame()
{
	TheoraMutex::ScopeLock mutex(&mMutex);
	TheoraVideoFrame* frame = _getFirstAvailableFrame();
	mutex.release();
	return frame;
}

void TheoraFrameQueue::clear()
{
	TheoraMutex::ScopeLock mutex(&mMutex);
	foreach_l (TheoraVideoFrame*, mQueue)
	{
		(*it)->clear();
	}
	mutex.release();
}

void TheoraFrameQueue::_pop(int n)
{
	for (int i = 0; i < n; ++i)
	{
		TheoraVideoFrame* first = mQueue.front();
		first->clear();
		mQueue.pop_front();
		mQueue.push_back(first);
	}
}

void TheoraFrameQueue::pop(int n)
{
	TheoraMutex::ScopeLock mutex(&mMutex);
	_pop(n);
	mutex.release();
}

TheoraVideoFrame* TheoraFrameQueue::requestEmptyFrame()
{
	TheoraVideoFrame* frame = NULL;
	TheoraMutex::ScopeLock mutex(&mMutex);
	foreach_l (TheoraVideoFrame*, mQueue)
	{
		if (!(*it)->mInUse)
		{
			(*it)->mInUse = 1;
			(*it)->mReady = 0;
			frame = (*it);
			break;
		}
	}
	mutex.release();
	return frame;
}

int TheoraFrameQueue::getUsedCount()
{
	TheoraMutex::ScopeLock mutex(&mMutex);
	int n = 0;
	foreach_l (TheoraVideoFrame*, mQueue)
	{
		if ((*it)->mInUse)
		{
			++n;
		}
	}
	mutex.release();
	return n;
}

int TheoraFrameQueue::_getReadyCount()
{
	int n = 0;
	foreach_l (TheoraVideoFrame*, mQueue)
	{
		if ((*it)->mReady)
		{
			++n;
		}
	}
	return n;
}


int TheoraFrameQueue::getReadyCount()
{
	TheoraMutex::ScopeLock mutex(&mMutex);
	int n = _getReadyCount();
	mutex.release();
	return n;
}

bool TheoraFrameQueue::isFull()
{
	return getReadyCount() == mQueue.size();
}

std::list<TheoraVideoFrame*>& TheoraFrameQueue::_getFrameQueue()
{
    return mQueue;
}
