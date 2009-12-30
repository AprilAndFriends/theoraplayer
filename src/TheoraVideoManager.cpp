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
#include "TheoraVideoManager.h"
#include "TheoraWorkerThread.h"
#include "TheoraVideoClip.h"
#include "TheoraAudioInterface.h"
#include "TheoraUtil.h"
#include "TheoraDataSource.h"

TheoraVideoManager* g_ManagerSingleton=0;
// declaring function prototype here so I don't have to put it in a header file
// it only needs to be used by this plugin and called once
void createYUVtoRGBtables();

void theora_writelog(std::string output)
{
	printf("%s\n",output.c_str());
}


TheoraVideoManager* TheoraVideoManager::getSingletonPtr(void)
{
    return g_ManagerSingleton;
}

TheoraVideoManager& TheoraVideoManager::getSingleton(void)
{  
    return *g_ManagerSingleton;  
}

TheoraVideoManager::TheoraVideoManager(int num_worker_threads) : 
	mDefaultNumPrecachedFrames(16)
{
	g_ManagerSingleton=this;
	mLogFuction=theora_writelog;

	mAudioFactory = NULL;
	mWorkMutex=new TheoraMutex();

	// for CPU yuv2rgb decoding
	createYUVtoRGBtables();
	createWorkerThreads(num_worker_threads);
}

TheoraVideoManager::~TheoraVideoManager()
{
	destroyWorkerThreads();

	ClipList::iterator ci;
	for (ci=mClips.begin(); ci != mClips.end();ci++)
		delete (*ci);
	mClips.clear();
	delete mWorkMutex;
}

void TheoraVideoManager::logMessage(std::string msg)
{
	mLogFuction(msg);
}

TheoraVideoClip* TheoraVideoManager::getVideoClipByName(std::string name)
{
	foreach(TheoraVideoClip*,mClips)
		if ((*it)->getName() == name) return *it;

	return 0;
}

void TheoraVideoManager::setAudioInterfaceFactory(TheoraAudioInterfaceFactory* factory)
{
	mAudioFactory=factory;
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
	clip = new TheoraVideoClip(data_source,output_mode,nPrecached,usePower2Stride);
	mClips.push_back(clip);
	mWorkMutex->unlock();
	return clip;
}

void TheoraVideoManager::destroyVideoClip(TheoraVideoClip* clip)
{
	if (clip)
	{
		mWorkMutex->lock();
		foreach(TheoraVideoClip*,mClips)
			if ((*it) == clip)
			{
				mClips.erase(it);
				break;
			}
		delete clip;
		mWorkMutex->unlock();
	}
}

TheoraVideoClip* TheoraVideoManager::requestWork(TheoraWorkerThread* caller)
{
	if (!mWorkMutex) return NULL;
	mWorkMutex->lock();
	TheoraVideoClip* c=NULL;

	float priority,last_priority=100000;

	foreach(TheoraVideoClip*,mClips)
	{
		if ((*it)->isBusy()) continue;
		priority=(*it)->getPriorityIndex();
		if (priority < last_priority)
		{
			last_priority=priority;
			c=*it;
		}
	}
	if (c) c->mAssignedWorkerThread=caller;
	
	mWorkMutex->unlock();
	return c;
}

void TheoraVideoManager::update(float time_increase)
{
	foreach(TheoraVideoClip*,mClips)
	{
		(*it)->update(time_increase);
		(*it)->decodedAudioCheck();
	}
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
	th_writelog("changing number of worker threats to: "+str(n));

	destroyWorkerThreads();
	createWorkerThreads(n);
}

std::string TheoraVideoManager::getVersionString()
{
	int a,b,c;
	getVersion(&a,&b,&c);
	std::string out=str(a)+"."+str(b);
	if (c != 0) out+="."+str(c);
	return out;
}

void TheoraVideoManager::getVersion(int* a,int* b,int* c)
{
	*a=0;
	*b=9;
	*c=0;
}
