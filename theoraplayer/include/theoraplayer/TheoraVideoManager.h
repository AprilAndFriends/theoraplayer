/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/

#ifndef THEORA_VIDEOMANAGER_H
#define THEORA_VIDEOMANAGER_H

#include <vector>
#include <list>
#include <string>

#ifdef _WIN32
#pragma warning( disable: 4251 ) // MSVC++
#endif

#include "TheoraExport.h"
#include "TheoraVideoClip.h"

// forward class declarations
class TheoraWorkerThread;
class TheoraMutex;
class TheoraDataSource;
class TheoraAudioInterfaceFactory;
/**
	This is the main singleton class that handles all playback/sync operations
*/
class TheoraPlayerExport TheoraVideoManager
{
public:
	TheoraVideoManager(int num_worker_threads = 1);
	virtual ~TheoraVideoManager();

	//! get the global reference to the manager instance
	static TheoraVideoManager& getSingleton();
	//! get the global pointer to the manager instance
	static TheoraVideoManager* getSingletonPtr();

	//! search registered clips by name
	TheoraVideoClip* getVideoClipByName(std::string name);
	TheoraAudioInterfaceFactory* getAudioInterfaceFactory();
	int getNumWorkerThreads();
	int getDefaultNumPrecachedFrames() { return mDefaultNumPrecachedFrames; }

	//! get nicely formated version string
	std::string getVersionString();

	/**
	\brief get version numbers

	if c is negative, it means it's a release candidate -c
	*/
	void getVersion(int* a, int* b, int* c);

	//! returns the supported decoders (eg. Theora, AVFoundation...)
	std::vector<std::string> getSupportedDecoders();

	/**
	\brief you can set your own log function to recieve theora's log calls

	This way you can integrate libtheoraplayer's log messages in your own
	logging system, prefix them, mute them or whatever you want
	*/
	static void setLogFunction(void(*fn)(std::string));

	void setAudioInterfaceFactory(TheoraAudioInterfaceFactory* factory);

	void setNumWorkerThreads(int n);

	void setDefaultNumPrecachedFrames(int n) { mDefaultNumPrecachedFrames = n; }

	TheoraVideoClip* createVideoClip(std::string filename,TheoraOutputMode output_mode=TH_RGB,int numPrecachedOverride=0,bool usePower2Stride=0);
	TheoraVideoClip* createVideoClip(TheoraDataSource* data_source,TheoraOutputMode output_mode=TH_RGB,int numPrecachedOverride=0,bool usePower2Stride=0);

	void update(float timeDelta);

	void destroyVideoClip(TheoraVideoClip* clip);	

	//! used by libtheoraplayer functions
	void logMessage(std::string msg);

protected:
	friend class TheoraWorkerThread;
	typedef std::vector<TheoraVideoClip*> ClipList;
	typedef std::vector<TheoraWorkerThread*> ThreadList;

	//! stores pointers to worker threads which are decoding video and audio
	ThreadList workerThreads;
	//! stores pointers to created video clips
	ClipList clips;

	//! stores pointer to clips that were docoded in the past in order to achieve fair scheduling
	std::list<TheoraVideoClip*> workLog;

	int mDefaultNumPrecachedFrames;

	TheoraMutex* workMutex;
	TheoraAudioInterfaceFactory* audioFactory;

	void createWorkerThreads(int n);
	void destroyWorkerThreads();

	float calcClipWorkTime(TheoraVideoClip* clip);

	/**
	* Called by TheoraWorkerThread to request a TheoraVideoClip instance to work on decoding
	*/
	TheoraVideoClip* requestWork(TheoraWorkerThread* caller);
};
#endif

