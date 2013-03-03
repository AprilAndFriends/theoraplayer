/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "TheoraVideoClip.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoFrame.h"
#include "TheoraFrameQueue.h"
#include "TheoraAudioInterface.h"
#include "TheoraTimer.h"
#include "TheoraDataSource.h"
#include "TheoraUtil.h"
#include "TheoraException.h"

TheoraVideoClip::TheoraVideoClip(TheoraDataSource* data_source,
								 TheoraOutputMode output_mode,
								 int nPrecachedFrames,
								 bool usePower2Stride):
	mAudioInterface(NULL),
	mNumDroppedFrames(0),
	mNumDisplayedFrames(0),
	mSeekFrame(-1),
	mDuration(-1),
	mNumFrames(-1),
	mFPS(1),
	mUseAlpha(0),
	mFrameDuration(0),
    mName(data_source->repr()),
    mStride(usePower2Stride),
    mAudioGain(1),
    mRequestedOutputMode(output_mode),
    mAutoRestart(0),
    mEndOfFile(0),
    mRestarted(0),
	mIteration(0),
	mLastIteration(0),
	mStream(0),
	mThreadAccessCount(0),
	mPriority(1),
	mWaitingForCache(false),
	mOutputMode(TH_UNDEFINED)
{
	mAudioMutex = NULL;
	mThreadAccessMutex = new TheoraMutex();
	mTimer = mDefaultTimer = new TheoraTimer();

	mFrameQueue = NULL;
	mAssignedWorkerThread = NULL;
	mNumPrecachedFrames = nPrecachedFrames;
	setOutputMode(output_mode);
}

TheoraVideoClip::~TheoraVideoClip()
{
	// wait untill a worker thread is done decoding the frame
	mThreadAccessMutex->lock();

	delete mDefaultTimer;

	if (mStream) delete mStream;

	if (mFrameQueue) delete mFrameQueue;

	if (mAudioInterface)
	{
		mAudioMutex->lock(); // ensure a thread isn't using this mutex
		delete mAudioInterface; // notify audio interface it's time to call it a day
        mAudioMutex ->unlock();
		delete mAudioMutex;
	}
	
	mThreadAccessMutex->unlock();

	delete mThreadAccessMutex;
}

TheoraTimer* TheoraVideoClip::getTimer()
{
	return mTimer;
}

void TheoraVideoClip::setTimer(TheoraTimer* timer)
{
	if (!timer) mTimer = mDefaultTimer;
	else mTimer = timer;
}

void TheoraVideoClip::restart()
{
	mEndOfFile = 1; //temp, to prevent threads to decode while restarting
	mThreadAccessMutex->lock();
	_restart();
	mTimer->seek(0);
	mFrameQueue->clear();
	mEndOfFile = 0;
	mIteration = 0;
	mRestarted = 0;
	mSeekFrame = -1;
	mThreadAccessMutex->unlock();

}

void TheoraVideoClip::update(float time_increase)
{
	if (mTimer->isPaused()) return;
	mTimer->update(time_increase);
	float time = mTimer->getTime();
	if (time >= mDuration)
	{
		if (mAutoRestart && mRestarted)
		{
			mIteration = !mIteration;
			float seekTime = time-mDuration;
			if (seekTime > 1.0f) seekTime = 1.0f; // to maintain loop smoothnes but prevent error accumulation
			mTimer->seek(seekTime);
			mRestarted = 0;
			int n = 0;
			for (;;)
			{
				TheoraVideoFrame* f = mFrameQueue->getFirstAvailableFrame();
				if (!f) break;
				if (f->mTimeToDisplay > 0.5f)
				{
					if (n == 0)
					{
						f->mTimeToDisplay = time-mDuration;
						f->mIteration = !f->mIteration;
					}
					else
						popFrame();
					n++;
				}
				else break;
			}
			if (n > 0) th_writelog("dropped " + str(n) + " end frames");
		}
		else return;
	}
}

float TheoraVideoClip::updateToNextFrame()
{
	TheoraVideoFrame* f = mFrameQueue->getFirstAvailableFrame();
	if (!f) return 0;

	float time = f->mTimeToDisplay-mTimer->getTime();
	update(time);
	return time;
}

TheoraFrameQueue* TheoraVideoClip::getFrameQueue()
{
	return mFrameQueue;
}

void TheoraVideoClip::popFrame()
{
	mNumDisplayedFrames++;
	mFrameQueue->pop(); // after transfering frame data to the texture, free the frame
						// so it can be used again
}

int TheoraVideoClip::getWidth()
{
	return mUseAlpha ? mWidth / 2 : mWidth;
}

int TheoraVideoClip::getHeight()
{
	return mHeight;
}

TheoraVideoFrame* TheoraVideoClip::getNextFrame()
{
	if (mSeekFrame != -1) return 0; // if we are about to seek, then the current frame queue is invalidated
	                                // (will be cleared when a worker thread does the actual seek)
	TheoraVideoFrame* frame;
	float time=mTimer->getTime();
	for (;;)
	{
		frame=mFrameQueue->getFirstAvailableFrame();
		if (!frame) return 0;
		if (frame->mTimeToDisplay > time) return 0;
		// only drop frames if you have more frames to show. otherwise even the late frame will do..
		if (time > frame->mTimeToDisplay + mFrameDuration && mFrameQueue->getReadyCount() > 1) 
		{
			if (mRestarted && frame->mTimeToDisplay < 2) return 0;
#ifdef _DEBUG
			th_writelog(mName+": dropped frame "+str(frame->getFrameNumber()));
#endif
			mNumDroppedFrames++;
			mNumDisplayedFrames++;
			mFrameQueue->pop();
		}
		else break;
	}

	mLastIteration=frame->mIteration;
	return frame;

}

std::string TheoraVideoClip::getName()
{
	return mName;
}

bool TheoraVideoClip::isBusy()
{
	return mAssignedWorkerThread || mOutputMode != mRequestedOutputMode;
}

TheoraOutputMode TheoraVideoClip::getOutputMode()
{
	return mOutputMode;
}

void TheoraVideoClip::setOutputMode(TheoraOutputMode mode)
{
	if (mode == TH_UNDEFINED) throw TheoraGenericException("Invalid output mode: TH_UNDEFINED for video: " + mName);
	if (mOutputMode == mode) return;
	mRequestedOutputMode = mode;
	mUseAlpha = (mode == TH_RGBA   ||
				 mode == TH_ARGB   ||
				 mode == TH_BGRA   ||
				 mode == TH_ABGR   ||
				 mode == TH_GREY3A ||
				 mode == TH_AGREY3 ||
				 mode == TH_YUVA   ||
				 mode == TH_AYUV);
	if (mAssignedWorkerThread)
	{
		mThreadAccessMutex->lock();
		// discard current frames and recreate them
		mFrameQueue->setSize(mFrameQueue->getSize());
		mThreadAccessMutex->unlock();

	}
	mOutputMode=mRequestedOutputMode;
}

float TheoraVideoClip::getTimePosition()
{
	return mTimer->getTime();
}

int TheoraVideoClip::getNumPrecachedFrames()
{
	return mFrameQueue->getSize();
}

void TheoraVideoClip::setNumPrecachedFrames(int n)
{
	if (mFrameQueue->getSize() != n)
		mFrameQueue->setSize(n);
}

int TheoraVideoClip::getNumReadyFrames()
{
	if (mSeekFrame != -1) return 0; // we are about to seek, consider frame queue empty even though it will be emptied upon seek
	return mFrameQueue->getReadyCount();
}

float TheoraVideoClip::getDuration()
{
	return mDuration;
}

float TheoraVideoClip::getFPS()
{
	return mFPS;
}

void TheoraVideoClip::play()
{
	mTimer->play();
}

void TheoraVideoClip::pause()
{
	mTimer->pause();
}

bool TheoraVideoClip::isPaused()
{
	return mTimer->isPaused();
}

bool TheoraVideoClip::isDone()
{
    return mEndOfFile && !mFrameQueue->getFirstAvailableFrame();
}

void TheoraVideoClip::stop()
{
	pause();
	mFrameQueue->clear();
	seek(0);
}

void TheoraVideoClip::setPlaybackSpeed(float speed)
{
    mTimer->setSpeed(speed);
}

float TheoraVideoClip::getPlaybackSpeed()
{
    return mTimer->getSpeed();
}

void TheoraVideoClip::seek(float time)
{
	seekToFrame((int) (time * getFPS()));
}

void TheoraVideoClip::seekToFrame(int frame)
{
	mSeekFrame = frame;
	mEndOfFile = false;
}

void TheoraVideoClip::waitForCache(float desired_cache_factor, float max_wait_time)
{
	mWaitingForCache = true;
	bool paused = mTimer->isPaused();
	if (!paused) mTimer->pause();
	int elapsed = 0;
	int desired_num_precached_frames = (int) (desired_cache_factor * getNumPrecachedFrames());
	while (getNumReadyFrames() < desired_num_precached_frames)
	{
		_psleep(10);
		elapsed += 10;
		if (elapsed >= max_wait_time * 1000) break;
	}
	if (!paused) mTimer->play();
	mWaitingForCache = false;
}

float TheoraVideoClip::getPriority()
{
	return mPriority;
}

void TheoraVideoClip::setPriority(float priority)
{
	mPriority = priority;
}

float TheoraVideoClip::getPriorityIndex()
{
	float priority = (float) getNumReadyFrames();
	if (mTimer->isPaused()) priority += getNumPrecachedFrames() / 2;
	
	return priority;
}

void TheoraVideoClip::setAudioInterface(TheoraAudioInterface* iface)
{
	mAudioInterface = iface;
	if (iface && !mAudioMutex) mAudioMutex = new TheoraMutex;
	if (!iface && mAudioMutex)
	{
		delete mAudioMutex;
		mAudioMutex = NULL;
	}
}

TheoraAudioInterface* TheoraVideoClip::getAudioInterface()
{
	return mAudioInterface;
}

void TheoraVideoClip::setAudioGain(float gain)
{
	if (gain > 1) mAudioGain=1;
	if (gain < 0) mAudioGain=0;
	else          mAudioGain=gain;
}

float TheoraVideoClip::getAudioGain()
{
	return mAudioGain;
}

void TheoraVideoClip::setAutoRestart(bool value)
{
	mAutoRestart=value;
	if (value) mEndOfFile=false;
}
