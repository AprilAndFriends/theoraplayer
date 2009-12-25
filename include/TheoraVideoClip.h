/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2009 Kresimir Spes (kreso@cateia.com)

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the 
Free Software Foundation; either version 2 of the License, or (at your option) 
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
*************************************************************************************/

#ifndef _TheoraVideoClip_h
#define _TheoraVideoClip_h

#include <string>
#include "TheoraExport.h"

// forward class declarations
class TheoraInfoStruct;
class TheoraMutex;
class TheoraFrameQueue;
class TheoraTimer;
class TheoraAudioInterface;
class TheoraWorkerThread;
class TheoraDataSource;

enum TheoraOutputMode
{
	// A= full alpha (255), order of letters represents the byte order for a pixel
	TH_RGB=1,
	TH_RGBA=2,
	TH_ARGB=3,
	TH_GREY=4,
	TH_GREY3=5, // RGB but all three components are luma
	TH_GREY3A=6,
	TH_AGREY3=7,
	TH_YUV=8,
	TH_YUVA=9,
	TH_AYUV=10
};

/**
	This object contains all data related to video playback, eg. the open source file,
	the frame queue etc.
*/
class TheoraPlayerExport TheoraVideoClip
{
	friend class TheoraWorkerThread;
	friend class TheoraVideoFrame;
	friend class TheoraVideoManager;

	TheoraFrameQueue* mFrameQueue;
	TheoraAudioInterface* mAudioInterface;
	TheoraDataSource* mStream;

	TheoraTimer *mTimer,*mDefaultTimer;

	TheoraWorkerThread* mAssignedWorkerThread;

	// benchmark vars
	int mNumDroppedFrames,mNumDisplayedFrames;
	
	int mTheoraStreams, mVorbisStreams;	// Keeps track of Theora and Vorbis Streams

	int mNumPrecachedFrames;
	int mAudioSkipSeekFlag;

	std::string mName;
	int mWidth,mHeight;
	float mDuration;

	float mAudioGain; //! multiplier for audio samples. between 0 and 1
	float mSeekPos; //! stores desired seek position. next worker thread will do the seeking and reset this var to -1
	TheoraOutputMode mOutputMode;
	bool mAutoRestart;
	bool mEndOfFile,mRestarted;
	int mIteration,mLastIteration; //! used to detect when the video restarted

	float mUserPriority;

	TheoraInfoStruct* mInfo; // a pointer is used to avoid having to include theora & vorbis headers

	TheoraMutex* mAudioMutex; //! syncs audio decoding and extraction

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
	void doSeek(); //! called by WorkerThread to seek to mSeekPos

	void load(TheoraDataSource* source);

	void _restart(); // resets the decoder and stream but leaves the frame queue intact
public:
	TheoraVideoClip(TheoraDataSource* data_source,TheoraOutputMode output_mode,int nPrecachedFrames);
	~TheoraVideoClip();

	std::string getName();

	// benchmark functions
	int getNumDisplayedFrames() { return mNumDisplayedFrames; }
	int getNumDroppedFrames() { return mNumDroppedFrames; }

	//! return width in pixels of the video clip
	int getWidth();
	//! return height in pixels of the video clip
	int getHeight();

	TheoraTimer* getTimer();
	void setTimer(TheoraTimer* timer);

	void decodeNextFrame();

	void update(float time_increase);
	void popFrame();
	TheoraVideoFrame* getNextFrame();
	//! check if there is enough audio data decoded to submit to the audio interface
	void decodedAudioCheck();

	void setAudioInterface(TheoraAudioInterface* iface);
	TheoraAudioInterface* getAudioInterface();

	void setNumPrecachedFrames(int n);
	int getNumPrecachedFrames();
	int getNumReadyFrames();

	void setAudioGain(float gain);
	float getAudioGain();
	void setAutoRestart(bool value);
	bool getAutoRestart();



	/**
	 * sets a user priority factor.
	 * @param priority must be in range [0,1]
	 */
	void setPriority(float priority);
	/**
	 * @return user set priority
	 */
	float getPriority();

	float getTimePosition();
	float getDuration();

	TheoraOutputMode getOutputMode();
	void setOutputMode(TheoraOutputMode mode);

    bool isDone();
	void play();
	void pause();
	void restart();
	bool isPaused();
	void stop();
	void seek(float time);
};

#endif

