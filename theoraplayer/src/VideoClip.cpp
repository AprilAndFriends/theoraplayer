/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <math.h>

#include "Manager.h"
#include "AudioInterface.h"

#include "DataSource.h"
#include "Exception.h"
#include "FrameQueue.h"
#include "Mutex.h"
#include "Thread.h"
#include "Timer.h"
#include "Utility.h"
#include "VideoClip.h"
#include "VideoFrame.h"

// TODOth - needs to be refactored

namespace theoraplayer
{
	VideoClip::VideoClip(DataSource* data_source,
		TheoraOutputMode output_mode,
		int nPrecachedFrames,
		bool usePower2Stride) :
		frameQueue(NULL),
		audioInterface(NULL),
		stream(NULL),
		assignedWorkerThread(NULL),
		useAlpha(false),
		waitingForCache(false),
		droppedFramesCount(0),
		displayedFramesCount(0),
		precachedFramesCount(nPrecachedFrames),
		threadAccessCount(0),
		seekFrame(-1),
		duration(-1),
		frameDuration(0),
		priority(1),
		name(data_source->toString()),
		width(0),
		height(0),
		stride(usePower2Stride),
		numFrames(-1),
		fps(1),
		subFrameWidth(0),
		subFrameHeight(0),
		subFrameOffsetX(0),
		subFrameOffsetY(0),
		audioGain(1.0f),
		outputMode(TH_UNDEFINED),
		requestedOutputMode(output_mode),
		firstFrameDisplayed(false),
		autoRestart(false),
		endOfFile(false),
		restarted(false),
		iteration(0),
		playbackIteration(0),
		audioMutex(NULL)
	{
		this->threadAccessMutex = new Mutex();
		this->timer = this->defaultTimer = new Timer();

		setOutputMode(output_mode);
	}

	VideoClip::~VideoClip()
	{
		// wait untill a worker thread is done decoding the frame
		Mutex::ScopeLock lock(this->threadAccessMutex);

		delete this->defaultTimer;

		if (this->stream)
		{
			delete this->stream;
		}

		if (this->frameQueue)
		{
			delete this->frameQueue;
		}

		if (this->audioInterface)
		{
			Mutex::ScopeLock audioMutexLock(this->audioMutex); // ensure a thread isn't using this mutex
			delete this->audioInterface; // notify audio interface it's time to call it a day
			audioMutexLock.release();
			delete this->audioMutex;
		}

		lock.release();
		delete this->threadAccessMutex;
	}

	Timer* VideoClip::getTimer()
	{
		return this->timer;
	}

	void VideoClip::setTimer(Timer* timer)
	{
		if (!timer)
		{
			this->timer = this->defaultTimer;
		}
		else
		{
			this->timer = timer;
		}
	}

	void VideoClip::resetFrameQueue()
	{
		this->frameQueue->clear();
		this->playbackIteration = this->iteration = 0;
	}

	void VideoClip::restart()
	{
		this->endOfFile = true; //temp, to prevent threads to decode while restarting
		Mutex::ScopeLock lock(this->threadAccessMutex);
		_restart();
		this->timer->seek(0);
		this->firstFrameDisplayed = false;
		resetFrameQueue();
		this->endOfFile = false;
		this->restarted = false;
		this->seekFrame = -1;
		lock.release();
	}

	void VideoClip::update(float timeDelta)
	{
		if (this->timer->isPaused())
		{
			this->timer->update(0); // update timer in case there is some code that needs to execute each frame
			return;
		}
		float time = this->timer->getTime(), speed = this->timer->getSpeed();
		if (time + timeDelta * speed >= this->duration)
		{
			if (this->autoRestart && this->restarted)
			{
				float seekTime = time + timeDelta * speed;
				for (; seekTime >= this->duration;)
				{
					seekTime -= this->duration;
					++this->playbackIteration;
				}

				this->timer->seek(seekTime);
			}
			else
			{
				if (time != this->duration)
				{
					this->timer->update((this->duration - time) / speed);
				}
			}
		}
		else
		{
			this->timer->update(timeDelta);
		}
	}

	float VideoClip::updateToNextFrame()
	{
		VideoFrame* f = this->frameQueue->getFirstAvailableFrame();
		if (f == NULL)
		{
			return 0;
		}

		float time = f->timeToDisplay - this->timer->getTime();
		update(time);
		return time;
	}

	FrameQueue* VideoClip::getFrameQueue()
	{
		return this->frameQueue;
	}

	void VideoClip::popFrame()
	{
		++this->displayedFramesCount;

		// after transfering frame data to the texture, free the frame
		// so it can be used again
		if (!this->firstFrameDisplayed)
		{
			Mutex::ScopeLock lock(this->frameQueue->getMutex());
			this->frameQueue->_pop(1);
			this->firstFrameDisplayed = true;
			lock.release();
		}
		else
		{
			this->frameQueue->pop();
		}
	}

	int VideoClip::getWidth()
	{
		return this->useAlpha ? this->width / 2 : this->width;
	}

	int VideoClip::getHeight()
	{
		return this->height;
	}

	int VideoClip::getSubFrameWidth()
	{
		return this->useAlpha ? this->width / 2 : this->subFrameWidth;
	}

	int VideoClip::getSubFrameHeight()
	{
		return this->useAlpha ? this->height : this->subFrameHeight;
	}

	int VideoClip::getSubFrameOffsetX()
	{
		return this->useAlpha ? 0 : this->subFrameOffsetX;
	}

	int VideoClip::getSubFrameOffsetY()
	{
		return this->useAlpha ? 0 : this->subFrameOffsetY;
	}

	float VideoClip::getAbsPlaybackTime()
	{
		return this->timer->getTime() + this->playbackIteration * this->duration;
	}

	int VideoClip::discardOutdatedFrames(float absTime)
	{
		int nReady = this->frameQueue->_getReadyCount();
		// only drop frames if you have more frames to show. otherwise even the late frame will do..
		if (nReady == 1)
		{
			return 0;
		}
		float time = absTime;

		int nPop = 0;
		VideoFrame* frame = NULL;
		float timeToDisplay = 0.0f;

		std::list<VideoFrame*>& queue = this->frameQueue->_getFrameQueue();
		foreach_l(VideoFrame*, it, queue)
		{
			frame = *it;
			if (!frame->ready)
			{
				break;
			}
			timeToDisplay = frame->timeToDisplay + frame->iteration * this->duration;
			if (time <= timeToDisplay + this->frameDuration)
			{
				break;
			}
			++nPop;
			if (nReady - nPop == 1)
			{
				break; // always leave at least one in the queue
			}
		}

		if (nPop > 0)
		{
#ifdef _DEBUG_FRAMEDROP
			std::string log = getName() + ": dropped frame ";

			int i = nPop;
			foreach_l(VideoFrame*, queue)
			{
				log += str((int)(*it)->getFrameNumber());
				if (i-- > 1)
				{
					log += ", ";
				}
				else break;
			}
			th_writelog(log);
#endif
			this->droppedFramesCount += nPop;
			this->frameQueue->_pop(nPop);
		}

		return nPop;
	}

	VideoFrame* VideoClip::getNextFrame()
	{
		VideoFrame* frame = NULL;
		// if we are about to seek, then the current frame queue is invalidated
		// (will be cleared when a worker thread does the actual seek)
		if (this->seekFrame != -1)
		{
			return NULL;
		}

		Mutex::ScopeLock lock(this->frameQueue->getMutex());
		float time = getAbsPlaybackTime();
		discardOutdatedFrames(time);

		frame = this->frameQueue->_getFirstAvailableFrame();
		if (frame != NULL)
		{
			if (frame->timeToDisplay + frame->iteration * this->duration > time && this->firstFrameDisplayed)
			{
				frame = NULL; // frame is ready but it's not yet time to display it, except when we haven't displayed any frames yet
			}
		}

		lock.release();
		return frame;
	}

	std::string VideoClip::getName()
	{
		return this->name;
	}

	bool VideoClip::isBusy()
	{
		return this->assignedWorkerThread || this->outputMode != this->requestedOutputMode;
	}

	TheoraOutputMode VideoClip::getOutputMode()
	{
		return this->outputMode;
	}

	void VideoClip::setOutputMode(TheoraOutputMode mode)
	{
		if (mode == TH_UNDEFINED)
		{
			throw TheoraplayerException("Invalid output mode: TH_UNDEFINED for video: " + this->name);
		}
		if (this->outputMode == mode)
		{
			return;
		}
		this->requestedOutputMode = mode;
		this->useAlpha = (mode == TH_RGBA ||
			mode == TH_ARGB ||
			mode == TH_BGRA ||
			mode == TH_ABGR ||
			mode == TH_GREY3A ||
			mode == TH_AGREY3 ||
			mode == TH_YUVA ||
			mode == TH_AYUV);
		if (this->assignedWorkerThread)
		{
			Mutex::ScopeLock lock(this->threadAccessMutex);
			// discard current frames and recreate them
			this->frameQueue->setSize(this->frameQueue->getSize());
			lock.release();

		}
		this->outputMode = this->requestedOutputMode;
	}

	float VideoClip::getTimePosition()
	{
		return this->timer->getTime();
	}

	int VideoClip::getNumPrecachedFrames()
	{
		return this->frameQueue->getSize();
	}

	void VideoClip::setNumPrecachedFrames(int n)
	{
		if (this->frameQueue->getSize() != n)
		{
			this->frameQueue->setSize(n);
		}
	}

	int VideoClip::_getNumReadyFrames()
	{
		if (this->seekFrame != -1)
		{
			return 0;
		}
		return this->frameQueue->_getReadyCount();
	}

	int VideoClip::getNumReadyFrames()
	{
		if (this->seekFrame != -1)
		{
			return 0; // we are about to seek, consider frame queue empty even though it will be emptied upon seek
		}
		return this->frameQueue->getReadyCount();
	}

	float VideoClip::getDuration()
	{
		return this->duration;
	}

	float VideoClip::getFps()
	{
		return this->fps;
	}

	void VideoClip::play()
	{
		this->timer->play();
	}

	void VideoClip::pause()
	{
		this->timer->pause();
	}

	bool VideoClip::isPaused()
	{
		return this->timer->isPaused();
	}

	bool VideoClip::isDone()
	{
		if (this->endOfFile)
		{
			VideoFrame* frame = this->frameQueue->getFirstAvailableFrame();
			if (frame == NULL || frame->timeToDisplay >= this->duration) // in some cases, there could be a diference between the reported video duration and timestamp on the last frame(s)
			{
				return true;
			}
		}
		return false;
	}

	void VideoClip::stop()
	{
		pause();
		resetFrameQueue();
		this->firstFrameDisplayed = false;
		seek(0);
	}

	void VideoClip::setPlaybackSpeed(float speed)
	{
		this->timer->setSpeed(speed);
	}

	float VideoClip::getPlaybackSpeed()
	{
		return this->timer->getSpeed();
	}

	void VideoClip::seek(float time)
	{
		seekToFrame((int)(time * getFps()));
	}

	void VideoClip::seekToFrame(int frame)
	{
		if (frame < 0)
		{
			this->seekFrame = 0;
		}
		else if (frame > this->numFrames)
		{
			this->seekFrame = this->numFrames;
		}
		else
		{
			this->seekFrame = frame;
		}

		this->firstFrameDisplayed = false;
		this->endOfFile = false;
	}

	float VideoClip::waitForCache(float desired_cache_factor, float max_wait_time)
	{
		this->waitingForCache = true;
		bool paused = this->timer->isPaused();
		if (!paused)
		{
			this->timer->pause();
		}
		int elapsed = 0, nReady = 0, frameQueueSize = getNumPrecachedFrames();
		if (this->numFrames < frameQueueSize)
		{
			frameQueueSize = this->numFrames;
		}
		int desired_num_precached_frames = (int)ceil(desired_cache_factor * frameQueueSize);
		do
		{
			nReady = getNumReadyFrames();
			if (nReady >= desired_num_precached_frames)
			{
				break;
			}
			Thread::sleep(10.0f);
			elapsed += 10;
		} while (elapsed >= max_wait_time * 1000);
		if (!paused)
		{
			this->timer->play();
		}
		this->waitingForCache = false;

		return (float)nReady / (float)frameQueueSize;
	}

	float VideoClip::getPriority()
	{
		return this->priority;
	}

	void VideoClip::setPriority(float priority)
	{
		this->priority = priority;
	}

	float VideoClip::getPriorityIndex()
	{
		float priority = (float)getNumReadyFrames();
		if (this->timer->isPaused())
		{
			priority += getNumPrecachedFrames() / 2;
		}

		return priority;
	}

	void VideoClip::setAudioInterface(AudioInterface* iface)
	{
		this->audioInterface = iface;
		if (iface && !this->audioMutex)
		{
			this->audioMutex = new Mutex();
		}
		if (!iface && this->audioMutex)
		{
			delete this->audioMutex;
			this->audioMutex = NULL;
		}
	}

	AudioInterface* VideoClip::getAudioInterface()
	{
		return this->audioInterface;
	}

	void VideoClip::setAudioGain(float gain)
	{
		if (gain > 1)
		{
			this->audioGain = 1;
		}
		else if (gain < 0)
		{
			this->audioGain = 0;
		}
		else
		{
			this->audioGain = gain;
		}
	}

	float VideoClip::getAudioGain()
	{
		return this->audioGain;
	}

	void VideoClip::setAutoRestart(bool value)
	{
		this->autoRestart = value;
		if (value)
		{
			this->endOfFile = false;
		}
	}

	void VideoClip::_lockAudioMutex()
	{
		this->audioMutex->lock();
	}

	void VideoClip::_unlockAudioMutex()
	{
		this->audioMutex->unlock();
	}

}
