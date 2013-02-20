/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "TheoraVideoManager.h"
#include "TheoraWorkerThread.h"
#include "TheoraVideoClip.h"
#include "TheoraFrameQueue.h"
#include "TheoraAudioInterface.h"
#include "TheoraUtil.h"
#include "TheoraDataSource.h"
#include "TheoraException.h"
#ifdef __THEORA
	#include <theora/codec.h>
	#include <vorbis/codec.h>
	#include "TheoraVideoClip_Theora.h"
#endif
#ifdef __AVFOUNDATION
	#include "TheoraVideoClip_AVFoundation.h"
#endif
// declaring function prototype here so I don't have to put it in a header file
// it only needs to be used by this plugin and called once
extern "C"
{
	void initYUVConversionModule();
}

// --------------------------
//#define _SCHEDULING_DEBUG
#ifdef _SCHEDULING_DEBUG
float gThreadDiagnosticTimer = 0;
#endif
// --------------------------

struct TheoraWorkCandidate
{
	TheoraVideoClip* clip;
	float priority, queuedTime, workTime, entitledTime;
};

TheoraVideoManager* g_ManagerSingleton = NULL;

void theora_writelog(std::string output)
{
	printf("%s\n", output.c_str());
}

void (*g_LogFuction)(std::string)=theora_writelog;

void TheoraVideoManager::setLogFunction(void (*fn)(std::string))
{
	g_LogFuction=fn;
}

TheoraVideoManager* TheoraVideoManager::getSingletonPtr()
{
    return g_ManagerSingleton;
}

TheoraVideoManager& TheoraVideoManager::getSingleton()
{  
    return *g_ManagerSingleton;  
}

TheoraVideoManager::TheoraVideoManager(int num_worker_threads) : 
	mDefaultNumPrecachedFrames(8)
{
	if (num_worker_threads < 1) throw TheoraGenericException("Unable to create TheoraVideoManager, at least one worker thread is reqired");

	g_ManagerSingleton = this;

	logMessage("Initializing Theora Playback Library (" + getVersionString() + ")\n" +
#ifdef __THEORA
	           "  - libtheora version: " + th_version_string() + "\n" + 
	           "  - libvorbis version: " + vorbis_version_string() + "\n" +
#endif
#ifdef __AVFOUNDATION
			   "  - using Apple AVFoundation classes\n"
#endif
			   "------------------------------------");
	mAudioFactory = NULL;
	mWorkMutex = new TheoraMutex();

	// for CPU based yuv2rgb decoding
	initYUVConversionModule();

	createWorkerThreads(num_worker_threads);
}

TheoraVideoManager::~TheoraVideoManager()
{
	destroyWorkerThreads();

	ClipList::iterator ci;
	for (ci = mClips.begin(); ci != mClips.end(); ci++)
		delete (*ci);
	mClips.clear();
	delete mWorkMutex;
}

void TheoraVideoManager::logMessage(std::string msg)
{
	g_LogFuction(msg);
}

TheoraVideoClip* TheoraVideoManager::getVideoClipByName(std::string name)
{
	foreach(TheoraVideoClip*,mClips)
		if ((*it)->getName() == name) return *it;

	return 0;
}

void TheoraVideoManager::setAudioInterfaceFactory(TheoraAudioInterfaceFactory* factory)
{
	mAudioFactory = factory;
}

TheoraAudioInterfaceFactory* TheoraVideoManager::getAudioInterfaceFactory()
{
	return mAudioFactory;
}

TheoraVideoClip* TheoraVideoManager::createVideoClip(std::string filename,
													 TheoraOutputMode output_mode,
													 int numPrecachedOverride,
													 bool usePower2Stride)
{
	TheoraDataSource* src=new TheoraFileDataSource(filename);
	return createVideoClip(src,output_mode,numPrecachedOverride,usePower2Stride);
}

TheoraVideoClip* TheoraVideoManager::createVideoClip(TheoraDataSource* data_source,
													 TheoraOutputMode output_mode,
													 int numPrecachedOverride,
													 bool usePower2Stride)
{
	mWorkMutex->lock();

	TheoraVideoClip* clip = NULL;
	int nPrecached = numPrecachedOverride ? numPrecachedOverride : mDefaultNumPrecachedFrames;
	logMessage("Creating video from data source: "+data_source->repr());
	
#ifdef __AVFOUNDATION
	TheoraFileDataSource* fileDataSource = dynamic_cast<TheoraFileDataSource*>(data_source);
	std::string filename;
	if (fileDataSource == NULL)
	{
		TheoraMemoryFileDataSource* memoryDataSource = dynamic_cast<TheoraMemoryFileDataSource*>(data_source);
		if (memoryDataSource != NULL) filename = memoryDataSource->getFilename();
		// if the user has his own data source, it's going to be a problem for AVAssetReader since it only supports reading from files...
	}
	else filename = fileDataSource->getFilename();

	if (filename.size() > 4 && filename.substr(filename.size() - 4, filename.size()) == ".mp4")
	{
		clip = new TheoraVideoClip_AVFoundation(data_source,output_mode,nPrecached,usePower2Stride);
	}
#endif
#if defined(__AVFOUNDATION) && defined(__THEORA)
	else
#endif
#ifdef __THEORA
		clip = new TheoraVideoClip_Theora(data_source,output_mode,nPrecached,usePower2Stride);
#endif
	clip->load(data_source);
	mClips.push_back(clip);
	mWorkMutex->unlock();
	return clip;
}

void TheoraVideoManager::destroyVideoClip(TheoraVideoClip* clip)
{
	if (clip)
	{
		th_writelog("Destroying video clip: " + clip->getName());
		mWorkMutex->lock();
		bool reported = 0;
		while (clip->mAssignedWorkerThread)
		{
			if (!reported)
			{
				th_writelog("Waiting for WorkerThread to finish decoding in order to destroy");
				reported = 1;
			}
			_psleep(1);
		}
		if (reported) th_writelog("WorkerThread done, destroying..");
		
		// erase the clip from the clip list
		foreach (TheoraVideoClip*, mClips)
		{
			if ((*it) == clip)
			{
				mClips.erase(it);
				break;
			}
		}
		// remove all it's references from the work log
		mWorkLog.remove(clip);

		// delete the actual clip
		delete clip;
		
		th_writelog("Destroyed video.");
		mWorkMutex->unlock();
	}
}

TheoraVideoClip* TheoraVideoManager::requestWork(TheoraWorkerThread* caller)
{
	if (!mWorkMutex) return NULL;
	mWorkMutex->lock();

	TheoraVideoClip* selectedClip = NULL;
	float maxQueuedTime = 0, totalAccessCount = 0, prioritySum = 0, diff, maxDiff = -1;
	int nReadyFrames;
	std::vector<TheoraWorkCandidate> candidates;
	TheoraVideoClip* clip;
	TheoraWorkCandidate candidate;

	// first pass is for playing videos, but if no such videos are available for decoding
	// paused videos are selected in the second pass. Paused videos that are waiting for cache
	// are considered as playing in the scheduling context

	for (int i = 0; i < 2 && candidates.size() == 0; i++)
	{
		foreach (TheoraVideoClip*, mClips)
		{
			clip = *it;
			if (clip->isBusy() || (i == 0 && clip->isPaused() && !clip->mWaitingForCache)) continue;
			nReadyFrames = clip->getNumReadyFrames();
			if (nReadyFrames == clip->getFrameQueue()->getSize()) continue;

			candidate.clip = clip;
			candidate.priority = clip->getPriority();
			candidate.queuedTime = (float) nReadyFrames / (clip->getFPS() * clip->getPlaybackSpeed());
			candidate.workTime = (float) clip->mThreadAccessCount;
			
			totalAccessCount += candidate.workTime;
			if (maxQueuedTime < candidate.queuedTime) maxQueuedTime = candidate.queuedTime;

			candidates.push_back(candidate);
		}
	}

	// prevent division by zero
	if (totalAccessCount == 0) totalAccessCount = 1;
	if (maxQueuedTime == 0) maxQueuedTime = 1;

	// normalize candidate values
	foreach (TheoraWorkCandidate, candidates)
	{
		it->workTime /= totalAccessCount;
		// adjust user priorities to favor clips that have fewer frames queued
		it->priority *= 1.0f - (it->queuedTime / maxQueuedTime) * 0.5f;
		prioritySum += it->priority;
	}
	foreach (TheoraWorkCandidate, candidates)
	{
		it->entitledTime = it->priority / prioritySum;
	}

	// now, based on how much access time has been given to each clip in the work log
	// and how much time should be given to each clip based on calculated priorities,
	// we choose a best suited clip for this worker thread to decode next
	foreach (TheoraWorkCandidate, candidates)
	{
		diff = it->entitledTime - it->workTime;

		if (maxDiff < diff)
		{
			maxDiff = diff;
			selectedClip = it->clip;
		}
	}

	if (selectedClip)
	{
		selectedClip->mAssignedWorkerThread = caller;
		
		int nClips = mClips.size();
		unsigned int maxWorkLogSize = (nClips - 1) * 50;

		if (nClips > 1)
		{
			mWorkLog.push_front(selectedClip);
			selectedClip->mThreadAccessCount++;
		}
		
		TheoraVideoClip* c;
		while (mWorkLog.size() > maxWorkLogSize)
		{
			c = mWorkLog.back();
			mWorkLog.pop_back();
			c->mThreadAccessCount--;
		}
#ifdef _SCHEDULING_DEBUG
		if (mClips.size() > 1)
		{
			int accessCount = mWorkLog.size();
			if (gThreadDiagnosticTimer > 2.0f)
			{
				gThreadDiagnosticTimer = 0;
				std::string logstr = "-----\nTheora Playback Library debug CPU time analysis (" + str(accessCount) + "):\n";
				int percent;
				foreach (TheoraVideoClip*, mClips)
				{
					percent = ((float) (*it)->mThreadAccessCount / mWorkLog.size()) * 100.0f;
					logstr += (*it)->getName() + " (" + str((*it)->getPriority()) + "): " + str((*it)->mThreadAccessCount) + ", " + str(percent) + "%\n";
				}
				logstr += "-----";
				th_writelog(logstr);
			}
		}
#endif

	}

	mWorkMutex->unlock();
	return selectedClip;
}

void TheoraVideoManager::update(float time_increase)
{
	foreach (TheoraVideoClip*, mClips)
	{
		(*it)->update(time_increase);
		(*it)->decodedAudioCheck();
	}
#ifdef _SCHEDULING_DEBUG
	gThreadDiagnosticTimer += time_increase;
#endif
}

int TheoraVideoManager::getNumWorkerThreads()
{
	return mWorkerThreads.size();
}

void TheoraVideoManager::createWorkerThreads(int n)
{
	TheoraWorkerThread* t;
	for (int i=0;i<n;i++)
	{
		t=new TheoraWorkerThread();
		t->startThread();
		mWorkerThreads.push_back(t);
	}
}

void TheoraVideoManager::destroyWorkerThreads()
{
	foreach(TheoraWorkerThread*,mWorkerThreads)
	{
		(*it)->waitforThread();
		delete (*it);
	}
	mWorkerThreads.clear();
}

void TheoraVideoManager::setNumWorkerThreads(int n)
{
	if (n == getNumWorkerThreads()) return;
	if (n < 1) throw TheoraGenericException("Unable to change the number of worker threads in TheoraVideoManager, at least one worker thread is reqired");

	th_writelog("changing number of worker threats to: "+str(n));

	destroyWorkerThreads();
	createWorkerThreads(n);
}

std::string TheoraVideoManager::getVersionString()
{
	int a, b, c;
	getVersion(&a, &b, &c);
	std::string out = str(a) + "." + str(b);
	if (c != 0)
	{
		if (c < 0) out += " RC" + str(-c);
		else       out += "." + str(c);
	}
	return out;
}

void TheoraVideoManager::getVersion(int* a, int* b, int* c) // TODO, return a struct instead of the current solution.
{
	*a = 1;
	*b = 0;
	*c = -4;
}

std::vector<std::string> TheoraVideoManager::getSupportedDecoders()
{
	std::vector<std::string> lst;
#ifdef __THEORA
	lst.push_back("Theora");
#endif
#ifdef __AVFOUNDATION
	lst.push_back("AVFoundation");
#endif
	
	return lst;
}
