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
/// Defines a video clip.

#ifndef THEORAPLAYER_VIDEO_CLIP_H
#define THEORAPLAYER_VIDEO_CLIP_H

#include <string>

#include "theoraplayerExport.h"

namespace theoraplayer
{
	// forward class declarations
	class DataSource;
	class FrameQueue;
	class Manager;
	class Mutex;
	class WorkerThread;
	class Timer;
	class VideoFrame;
	class AudioInterface;

	/**
		format of the VideoFrame pixels. Affects decoding time
		*/
	enum TheoraOutputMode
	{
		// A = full alpha (255), order of letters represents the byte order for a pixel
		// A means the image is treated as if it contains an alpha channel, while X formats
		// just mean that RGB frame is transformed to a 4 byte format
		TH_UNDEFINED = 0,
		TH_RGB = 1,
		TH_RGBA = 2,
		TH_RGBX = 3,
		TH_ARGB = 4,
		TH_XRGB = 5,
		TH_BGR = 6,
		TH_BGRA = 7,
		TH_BGRX = 8,
		TH_ABGR = 9,
		TH_XBGR = 10,
		TH_GREY = 11,
		TH_GREY3 = 12,
		TH_GREY3A = 13,
		TH_GREY3X = 14,
		TH_AGREY3 = 15,
		TH_XGREY3 = 16,
		TH_YUV = 17,
		TH_YUVA = 18,
		TH_YUVX = 19,
		TH_AYUV = 20,
		TH_XYUV = 21
	};

	/**
		This object contains all data related to video playback, eg. the open source file,
		the frame queue etc.
		*/
	class theoraplayerExport VideoClip
	{
	public:
		friend class Manager;
		friend class VideoFrame;
		friend class WorkerThread;

		std::string getName();
		//! Returns the string name of the decoder backend (eg. Theora, AVFoundation)
		virtual std::string getDecoderName() = 0;

		//! benchmark function
		int getDisplayedFramesCount() { return this->displayedFramesCount; }
		//! benchmark function
		int getDroppedFramesCount() { return this->droppedFramesCount; }

		//! return width in pixels of the video clip
		int getWidth();
		//! return height in pixels of the video clip
		int getHeight();
		//! returns whether the clip has an alpha channel
		bool hasAlphaChannel() { return this->useAlpha; }

		//! Width of the actual picture inside a video frame (depending on implementation, this may be equal to mWidth or differ within a codec block size (usually 16))
		int getSubFrameWidth();
		//! Height of the actual picture inside a video frame (depending on implementation, this may be equal to mHeight or differ within a codec block size (usually 16))
		int getSubFrameHeight();
		//! X Offset of the actual picture inside a video frame (depending on implementation, this may be 0 or within a codec block size (usually 16))
		int getSubFrameOffsetX();
		//! Y Offset of the actual picture inside a video frame (depending on implementation, this may be 0 or differ within a codec block size (usually 16))
		int getSubFrameOffsetY();
		/**
			\brief return stride in pixels

			If you've specified usePower2Stride when creating the VideoClip object
			then this value will be the next power of two size compared to width,
			eg: w=376, stride=512.

			Otherwise, stride will be equal to width
			*/
		int getStride() { return this->stride; }
		//! retur the timer objet associated with this object
		Timer* getTimer();

		FrameQueue* getFrameQueue();
		/**
		\brief Returns the first available frame in the queue or NULL if no frames are available.

		see FrameQueue::getFirstAvailableFrame() for more details
		*/
		VideoFrame* getNextFrame();

		AudioInterface* getAudioInterface();

		// TODOth - rename these
		//! returns the size of the frame queue
		int getNumPrecachedFrames();
		//! returns the number of ready frames in the frame queue
		int getNumReadyFrames();

		float getAudioGain();

		bool getAutoRestart() { return this->autoRestart; }

		float getPriority();
		//! Used by TheoraVideoManager to schedule work
		float getPriorityIndex();

		//! get the current time index from the timer object
		float getTimePosition();
		//! get the duration of the movie in seconds
		float getDuration();
		//! return the clips' frame rate, warning, fps can be a non integer number!
		float getFps();
		//! get the number of frames in this movie
		int getNumFrames() { return this->numFrames; }

		//! return the current output mode for this video object
		TheoraOutputMode getOutputMode();

		float getPlaybackSpeed();

		//! replace the timer object with a new one
		void setTimer(Timer* timer);

		void setAudioInterface(AudioInterface* iface);

		/**
		\brief resize the frame queues

		Warning: this call discards ready frames in the frame queue
		*/
		void setNumPrecachedFrames(int n);
		//! if you want to adjust the audio gain. range [0,1]
		void setAudioGain(float gain);
		//! if you want the video to automatically and smoothly restart when the last frame is reached
		void setAutoRestart(bool value);
		void setPriority(float priority);

		/**
		set a new output mode

		Warning: this discards the frame queue. ready frames will be lost.
		*/
		void setOutputMode(TheoraOutputMode mode);

		void setPlaybackSpeed(float speed);

		//! used by TheoraWorkerThread, do not call directly
		virtual bool decodeNextFrame() = 0;

		//! advance time. TheoraVideoManager calls this
		void update(float timeDelta);
		/**
			\brief update timer to the display time of the next frame

			useful if you want to grab frames instead of regular display
			\return time advanced. 0 if no frames are ready
			*/
		float updateToNextFrame();

		/**
			\brief pop the frame from the front of the FrameQueue

			see FrameQueue::pop() for more details
			*/
		void popFrame();


		/**
			check if there is enough audio data decoded to submit to the audio interface

			TheoraWorkerThread calls this
			*/
		virtual void decodedAudioCheck() = 0;

		bool isDone();
		void play();
		void pause();
		void restart();
		bool isPaused();
		void stop();

		//! seek to a given time position
		void seek(float time);
		//! seek to a given frame number
		void seekToFrame(int frame);
		//! wait max_time for the clip to cache a given percentage of frames, factor in range [0,1]. Returns actual precache factor
		float waitForCache(float desired_cache_factor = 0.5f, float max_wait_time = 1.0f);

	protected:
		FrameQueue* frameQueue;
		AudioInterface* audioInterface;
		DataSource* stream;

		Timer *timer, *defaultTimer;

		WorkerThread* assignedWorkerThread;

		bool useAlpha;

		bool waitingForCache;

		// benchmark vars
		int droppedFramesCount;
		int displayedFramesCount;
		int precachedFramesCount;

		int threadAccessCount; //! counter used by TheoraVideoManager to schedule workload

		int seekFrame; //! stores desired seek position as a frame number. next worker thread will do the seeking and reset this var to -1
		float duration, frameDuration;
		float priority; //! User assigned priority. Default value is 1
		std::string name;
		int width, height, stride;
		int numFrames;
		float fps;

		int subFrameWidth, subFrameHeight, subFrameOffsetX, subFrameOffsetY;
		float audioGain; //! multiplier for audio samples. between 0 and 1

		TheoraOutputMode outputMode, requestedOutputMode;
		bool firstFrameDisplayed;
		bool autoRestart;
		bool endOfFile, restarted;
		int iteration, playbackIteration; //! used to ensure smooth playback of looping videos

		Mutex* audioMutex; //! syncs audio decoding and extraction
		Mutex* threadAccessMutex;

		VideoClip(DataSource* data_source, TheoraOutputMode output_mode, int nPrecachedFrames, bool usePower2Stride);
		virtual ~VideoClip();

		/**
		* Get the priority of a video clip. based on a forumula that includes user
		* priority factor, whether the video is paused or not, how many precached
		* frames it has etc.
		* This function is used in TheoraVideoManager to efficiently distribute job
		* assignments among worker threads
		* @return priority number of this video clip
		*/
		int calculatePriority();
		void readTheoraVorbisHeaders();
		virtual void doSeek() = 0; //! called by WorkerThread to seek to mSeekFrame
		virtual bool _readData() = 0;
		bool isBusy();

		/**
		* decodes audio from the vorbis stream and stores it in audio packets
		* This is an internal function of VideoClip, called regularly if playing an
		* audio enabled video clip.
		* @return last decoded timestamp (if found in decoded packet's granule position)
		*/
		virtual float decodeAudio() = 0;

		int _getNumReadyFrames();
		void resetFrameQueue();
		int discardOutdatedFrames(float absTime);
		float getAbsPlaybackTime();
		virtual void _load(DataSource* source) = 0;

		virtual void _restart() = 0; // resets the decoder and stream but leaves the frame queue intact

	};

}
#endif
