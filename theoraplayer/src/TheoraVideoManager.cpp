/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "TheoraAudioInterface.h"
#include "TheoraDataSource.h"
#include "TheoraVideoClip.h"
#include "TheoraVideoManager.h"
#include "TheoraFrameQueue.h"
#include "TheoraUtil.h"

#include "Exception.h"
#include "WorkerThread.h"
using namespace theoraplayer; // TODOth - remove this later

#ifdef __THEORA
	#include <theora/codec.h>
	#include <vorbis/codec.h>
	#include "Theora/TheoraVideoClip_Theora.h"
#endif
#ifdef __WEBM
	#include "WebM/TheoraVideoClip_WebM.h"
#endif
#ifdef __AVFOUNDATION
	#include "AVFoundation/TheoraVideoClip_AVFoundation.h"
#endif
#ifdef __FFMPEG
	#include "FFmpeg/TheoraVideoClip_FFmpeg.h"
#endif
#ifdef _ANDROID //libtheoraplayer addition for cpu feature detection
	#include "cpu-features.h"
#endif
// declaring function prototype here so I don't have to put it in a header file
// it only needs to be used by this plugin and called once
extern "C"
{
	void initYUVConversionModule();
}

//#define _DECODING_BENCHMARK //uncomment to test average decoding time on a given device


// --------------------------
//#define _SCHEDULING_DEBUG
#ifdef _SCHEDULING_DEBUG
float gThreadDiagnosticTimer = 0;
#endif
// --------------------------

#ifdef _DECODING_BENCHMARK
void benchmark(TheoraVideoClip* clip)
{
	int nPrecached = 256;
	int n = nPrecached;
	char msg[1024];
	clock_t t = clock();
	while (n > 0)
	{
		clip->waitForCache(1.0f, 1000000);
		n -= 32;
		clip->getFrameQueue()->clear();
	}
	float diff = ((float) (clock() - t) * 1000.0f) / CLOCKS_PER_SEC;
	sprintf(msg, "BENCHMARK: %s: Decoding %d frames took %.1fms (%.2fms average per frame)\n",clip->getName().cStr(), nPrecached, diff, diff / nPrecached);
	TheoraVideoManager::getSingleton().logMessage(msg);
	clip->seek(0);
}
#endif

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

void (*g_LogFuction)(std::string) = theora_writelog;

void TheoraVideoManager::setLogFunction(void (*fn)(std::string))
{
	g_LogFuction = fn;
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
	if (num_worker_threads < 1)
	{
		throw TheoraplayerException("Unable to create TheoraVideoManager, at least one worker thread is reqired");
	}

	g_ManagerSingleton = this;

	std::string msg = "Initializing Theora Playback Library (" + getVersionString() + ")\n";
#ifdef __THEORA
	msg += "  - libtheora version: " + std::string(th_version_string()) + "\n" +
		"  - libvorbis version: " +  std::string(vorbis_version_string()) + "\n";
#endif
#ifdef _ANDROID
	uint64_t features = libtheoraplayer_android_getCpuFeaturesExt();
	char s[128];
	sprintf(s, "  - Android: CPU Features: %u\n", (unsigned int) features);
	msg += s;
	if ((features & ANDROID_CPU_ARM_FEATURE_NEON) == 0)
	{
		msg += "  - Android: NEON features NOT SUPPORTED by CPU\n";
	}
	else
	{
		msg += "  - Android: Detected NEON CPU features\n";
	}
#endif

#ifdef __AVFOUNDATION
	msg += "  - using Apple AVFoundation classes.\n";
#endif
#ifdef __FFMPEG
	msg += "  - using FFmpeg library.\n";
#endif
	
	logMessage(msg + "------------------------------------");
	this->audioFactory = NULL;
	this->workMutex = new Mutex();

	// for CPU based yuv2rgb decoding
	initYUVConversionModule();

	createWorkerThreads(num_worker_threads);
}

TheoraVideoManager::~TheoraVideoManager()
{
	destroyWorkerThreads();
	Mutex::ScopeLock lock(this->workMutex);
	ClipList::iterator ci;
	for (ci = this->clips.begin(); ci != this->clips.end(); ++ci)
	{
		delete (*ci);
	}
	this->clips.clear();
	lock.release();
	delete this->workMutex;
}

void TheoraVideoManager::logMessage(std::string msg)
{
	g_LogFuction(msg);
}

TheoraVideoClip* TheoraVideoManager::getVideoClipByName(std::string name)
{
	TheoraVideoClip* clip = NULL;
	Mutex::ScopeLock lock(this->workMutex);
	foreach (TheoraVideoClip*, this->clips)
	{
		if ((*it)->getName() == name)
		{
			clip = *it;
			break;
		}
	}
	lock.release();
	return clip;
}

void TheoraVideoManager::setAudioInterfaceFactory(TheoraAudioInterfaceFactory* factory)
{
	this->audioFactory = factory;
}

TheoraAudioInterfaceFactory* TheoraVideoManager::getAudioInterfaceFactory()
{
	return this->audioFactory;
}

TheoraVideoClip* TheoraVideoManager::createVideoClip(std::string filename,
													 TheoraOutputMode output_mode,
													 int numPrecachedOverride,
													 bool usePower2Stride)
{
	TheoraDataSource* src = new TheoraFileDataSource(filename);
	return createVideoClip(src, output_mode, numPrecachedOverride, usePower2Stride);
}

TheoraVideoClip* TheoraVideoManager::createVideoClip(TheoraDataSource* data_source,
													 TheoraOutputMode output_mode,
													 int numPrecachedOverride,
													 bool usePower2Stride)
{
	Mutex::ScopeLock lock(this->workMutex);

	TheoraVideoClip* clip = NULL;
	int nPrecached = numPrecachedOverride ? numPrecachedOverride : mDefaultNumPrecachedFrames;
	logMessage("Creating video from data source: " + data_source->toString() + " [" + str(nPrecached) + " precached frames].");
	
#ifdef __AVFOUNDATION
	TheoraFileDataSource* fileDataSource = dynamic_cast<TheoraFileDataSource*>(data_source);
	std::string filename;
	if (fileDataSource == NULL)
	{
		TheoraMemoryFileDataSource* memoryDataSource = dynamic_cast<TheoraMemoryFileDataSource*>(data_source);
		if (memoryDataSource != NULL)
		{
			filename = memoryDataSource->getFilename();
		}
		// if the user has his own data source, it's going to be a problem for AVAssetReader since it only supports reading from files...
	}
	else
	{
		filename = fileDataSource->getFilename();
	}

	if (filename.getSize() > 4 && filename.substr(filename.getSize() - 4, filename.getSize()) == ".mp4")
	{
		clip = new TheoraVideoClip_AVFoundation(data_source, output_mode, nPrecached, usePower2Stride);
	}
#endif
#if defined(__AVFOUNDATION) && defined(__THEORA)
	else
#endif
#ifdef __THEORA
		clip = new TheoraVideoClip_Theora(data_source, output_mode, nPrecached, usePower2Stride);
#endif
#ifdef __FFMPEG
		clip = new TheoraVideoClip_FFmpeg(data_source, output_mode, nPrecached, usePower2Stride);
#endif
#ifdef __WEBM
		clip = new TheoraVideoClip_WebM(data_source, output_mode, nPrecached, usePower2Stride);		
#endif
	if (clip != NULL)
	{
		try
		{
			clip->load(data_source);
		}
		catch (_Exception& e)
		{
			delete clip;
			throw e;
		}
		clip->decodeNextFrame(); // ensure the first frame is always preloaded and have the main thread do it to prevent potential thread starvation

		this->clips.push_back(clip);
	}
	else
	{
		th_writelog("Failed creating video clip: " + data_source->toString());
	}
	lock.release();
	
#ifdef _DECODING_BENCHMARK
	benchmark(clip);
#endif
	return clip;
}

void TheoraVideoManager::destroyVideoClip(TheoraVideoClip* clip)
{
	if (clip)
	{
		th_writelog("Destroying video clip: " + clip->getName());
		Mutex::ScopeLock lock(this->workMutex);
		bool reported = 0;
		while (clip->assignedWorkerThread)
		{
			if (!reported)
			{
				th_writelog(" - Waiting for WorkerThread to finish decoding in order to destroy");
				reported = 1;
			}
			_psleep(1);
		}
		if (reported)
		{
			th_writelog(" - WorkerThread done, destroying...");
		}
		
		// erase the clip from the clip list
		foreach (TheoraVideoClip*, this->clips)
		{
			if ((*it) == clip)
			{
				this->clips.erase(it);
				break;
			}
		}
		// remove all it's references from the work log
		this->workLog.remove(clip);

		// delete the actual clip
		delete clip;
#ifdef _DEBUG
		th_writelog("Destroyed video.");
#endif
		lock.release();
	}
}

TheoraVideoClip* TheoraVideoManager::requestWork(WorkerThread* caller)
{
	if (!this->workMutex)
	{
		return NULL;
	}
	Mutex::ScopeLock lock(this->workMutex);

	TheoraVideoClip* selectedClip = NULL;
	float maxQueuedTime = 0, totalAccessCount = 0, prioritySum = 0, diff, maxDiff = -1;
	int nReadyFrames;
	std::vector<TheoraWorkCandidate> candidates;
	TheoraVideoClip* clip;
	TheoraWorkCandidate candidate;

	// first pass is for playing videos, but if no such videos are available for decoding
	// paused videos are selected in the second pass.
	// Note that paused videos that are waiting for cache are considered equal to playing
	// videos in the scheduling context

	for (int i = 0; i < 2 && candidates.size() == 0; ++i)
	{
		foreach (TheoraVideoClip*, this->clips)
		{
			clip = *it;
			if (clip->isBusy() || (i == 0 && clip->isPaused() && !clip->waitingForCache))
			{
				continue;
			}
			nReadyFrames = clip->getNumReadyFrames();
			if (nReadyFrames == clip->getFrameQueue()->getSize())
			{
				continue;
			}

			candidate.clip = clip;
			candidate.priority = clip->getPriority();
			candidate.queuedTime = (float) nReadyFrames / (clip->getFps() * clip->getPlaybackSpeed());
			candidate.workTime = (float) clip->threadAccessCount;
			
			totalAccessCount += candidate.workTime;
			if (maxQueuedTime < candidate.queuedTime)
			{
				maxQueuedTime = candidate.queuedTime;
			}

			candidates.push_back(candidate);
		}
	}

	// prevent division by zero
	if (totalAccessCount == 0)
	{
		totalAccessCount = 1;
	}
	if (maxQueuedTime == 0)
	{
		maxQueuedTime = 1;
	}

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
		selectedClip->assignedWorkerThread = caller;
		
		int nClips = (int) this->clips.size();
		unsigned int maxWorkLogSize = (nClips - 1) * 50;

		if (nClips > 1)
		{
			this->workLog.push_front(selectedClip);
			++selectedClip->threadAccessCount;
		}
		
		TheoraVideoClip* c;
		while (this->workLog.size() > maxWorkLogSize)
		{
			c = this->workLog.back();
			this->workLog.pop_back();
			--c->threadAccessCount;
		}
#ifdef _SCHEDULING_DEBUG
		if (this->clips.getSize() > 1)
		{
			int accessCount = this->workLog.getSize();
			if (gThreadDiagnosticTimer > 2.0f)
			{
				gThreadDiagnosticTimer = 0;
				std::string logstr = "-----\nTheora Playback Library debug CPU time analysis (" + str(accessCount) + "):\n";
				int percent;
				foreach (TheoraVideoClip*, this->clips)
				{
					percent = ((float) (*it)->threadAccessCount / this->workLog.getSize()) * 100.0f;
					logstr += (*it)->getName() + " (" + str((*it)->getPriority()) + "): " + str((*it)->threadAccessCount) + ", " + str(percent) + "%\n";
				}
				logstr += "-----";
				th_writelog(logstr);
			}
		}
#endif
	}

	lock.release();
	return selectedClip;
}

void TheoraVideoManager::update(float timeDelta)
{
	Mutex::ScopeLock lock(this->workMutex);
	foreach (TheoraVideoClip*, this->clips)
	{
		(*it)->update(timeDelta);
		(*it)->decodedAudioCheck();
	}
	lock.release();
#ifdef _SCHEDULING_DEBUG
	gThreadDiagnosticTimer += timeDelta;
#endif
}

int TheoraVideoManager::getNumWorkerThreads()
{
	return (int) this->workerThreads.size();
}

void TheoraVideoManager::createWorkerThreads(int n)
{
	WorkerThread* t = NULL;
	for (int i = 0; i < n; ++i)
	{
		t = new WorkerThread();
		t->start();
		this->workerThreads.push_back(t);
	}
}

void TheoraVideoManager::destroyWorkerThreads()
{
	foreach (WorkerThread*, this->workerThreads)
	{
		(*it)->join();
		delete (*it);
	}
	this->workerThreads.clear();
}

void TheoraVideoManager::setNumWorkerThreads(int n)
{
	if (n == getNumWorkerThreads())
	{
		return;
	}
	if (n < 1)
	{
		throw TheoraplayerException("Unable to change the number of worker threads in TheoraVideoManager, at least one worker thread is reqired");
	}

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
		if (c < 0)
		{
			out += " RC" + str(-c);
		}
		else
		{
			out += "." + str(c);
		}
	}
	return out;
}

void TheoraVideoManager::getVersion(int* a, int* b, int* c) // TODOth, return a struct instead of the current solution.
{
	*a = 2;
	*b = 0;
	*c = 0;
}

std::vector<std::string> TheoraVideoManager::getSupportedDecoders()
{
	std::vector<std::string> lst;
#ifdef __THEORA
	lst.push_back("Theora");
#endif
#ifdef __WEBM
	lst.push_back("WebM");
#endif
#ifdef __AVFOUNDATION
	lst.push_back("AVFoundation");
#endif
#ifdef __FFMPEG
	lst.push_back("FFmpeg");
#endif
	
	return lst;
}
