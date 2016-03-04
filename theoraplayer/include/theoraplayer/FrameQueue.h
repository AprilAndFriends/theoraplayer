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
/// Defines a frame queue.

#ifndef THEORAPLAYER_FRAME_QUEUE_H
#define THEORAPLAYER_FRAME_QUEUE_H

#include <list>

#include "Mutex.h"
#include "theoraplayerExport.h"
#include "Thread.h"

namespace theoraplayer
{
	class VideoClip;
	class VideoFrame;

	/**
		This class handles the frame queue. contains frames and handles their alloctation/deallocation
		it is designed to be thread-safe
	*/
	class theoraplayerExport FrameQueue
	{
	public:
		FrameQueue(VideoClip* parent);
		~FrameQueue();

		/**
			\brief Returns the first available frame in the queue or NULL if no frames are available.

			This function DOES NOT remove the frame from the queue, you have to do it manually
			when you want to mark the frame as used by calling the pop() function.
		*/
		VideoFrame* getFirstAvailableFrame();
		//! non-mutex version
		VideoFrame* _getFirstAvailableFrame();

		//! return the number of used (not ready) frames
		int getUsedCount();

		//! return the number of ready frames
		int getReadyCount();
		//! non-mutex version
		int _getReadyCount();

		/**
		\brief set's the size of the frame queue.

		Beware, currently stored ready frames will be lost upon this call
		*/
		void setSize(int n);
		//! return the size of the queue
		int getSize();

		//! return whether all frames in the queue are ready for display
		bool isFull();

		Mutex* getMutex() { return &this->mutex; }

		//! returns the internal frame queue. Warning: Always lock / unlock queue's mutex before accessing frames directly!
		std::list<VideoFrame*>& _getFrameQueue();

		//! Called by WorkerThreads when they need to unload frame data, do not call directly!
		VideoFrame* requestEmptyFrame();

		/**
			\brief remove the first N available frame from the queue.

			Use this every time you display a frame	so you can get the next one when the time comes.
			This function marks the frame on the front of the queue as unused and it's memory then
			get's used again in the decoding process.
			If you don't call this, the frame queue will fill up with precached frames up to the
			specified amount in the TheoraVideoManager class and you won't be able to advance the video.
		*/
		void pop(int n = 1);

		//! This is an internal _pop function. use externally only in combination with lock() / unlock() calls
		void _pop(int n);

		//! frees all decoded frames for reuse (does not destroy memory, just marks them as free)
		void clear();

	protected:
		std::list<VideoFrame*> queue;
		VideoClip* parent;
		Mutex mutex;

		//! implementation function that returns a VideoFrame instance
		VideoFrame* createFrameInstance(VideoClip* clip);

	};

}
#endif
