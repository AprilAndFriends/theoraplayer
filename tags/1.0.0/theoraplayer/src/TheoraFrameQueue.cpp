/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
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
	foreach_l(TheoraVideoFrame*,mQueue)
		delete (*it);
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
	mMutex.lock();
	if (mQueue.size() > 0)
	{
		foreach_l (TheoraVideoFrame*,mQueue)
			delete (*it);
		mQueue.clear();
	}
	TheoraVideoFrame* frame;
	for (int i = 0;i < n; i++)
	{
		frame = createFrameInstance(mParent);
		if (frame != NULL) mQueue.push_back(frame);
		else
		{
			TheoraVideoManager::getSingleton().logMessage("TheoraFrameQueue: unable to create " + str(n) + " frames, out of memory. Created " + str(mQueue.size()) + " frames.");
			break;
		}
	}
	mMutex.unlock();
}

int TheoraFrameQueue::getSize()
{
	return mQueue.size();
}

TheoraVideoFrame* TheoraFrameQueue::getFirstAvailableFrame()
{
	TheoraVideoFrame* frame = NULL;
	mMutex.lock();
	if (mQueue.front()->mReady) frame = mQueue.front();
	mMutex.unlock();
	return frame;
}

void TheoraFrameQueue::clear()
{
	mMutex.lock();
	foreach_l(TheoraVideoFrame*,mQueue)
		(*it)->clear();
	mMutex.unlock();
}

void TheoraFrameQueue::pop()
{
	mMutex.lock();
	TheoraVideoFrame* first=mQueue.front();
	first->clear();
	mQueue.pop_front();
	mQueue.push_back(first);
	mMutex.unlock();
}

TheoraVideoFrame* TheoraFrameQueue::requestEmptyFrame()
{
	TheoraVideoFrame* frame = NULL;
	mMutex.lock();
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
	mMutex.unlock();
	return frame;
}

int TheoraFrameQueue::getUsedCount()
{
	mMutex.lock();
	int n=0;
	foreach_l(TheoraVideoFrame*,mQueue)
		if ((*it)->mInUse) n++;
	mMutex.unlock();
	return n;
}

int TheoraFrameQueue::getReadyCount()
{
	mMutex.lock();
	int n=0;
	foreach_l(TheoraVideoFrame*,mQueue)
		if ((*it)->mReady) n++;
	mMutex.unlock();
	return n;
}

bool TheoraFrameQueue::isFull()
{
	return getReadyCount() == mQueue.size();
}

void TheoraFrameQueue::lock()
{
	mMutex.lock();
}

void TheoraFrameQueue::unlock()
{
	mMutex.unlock();
}