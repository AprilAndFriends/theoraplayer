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

TheoraVideoManager* TheoraVideoManager::getSingletonPtr(void)
{
    return g_ManagerSingleton;
}
TheoraVideoManager& TheoraVideoManager::getSingleton(void)
{  
    return *g_ManagerSingleton;  
}

TheoraVideoManager::TheoraVideoManager()
{
	mAudioFactory = NULL;
	mWorkMutex=new TheoraMutex();

	// for CPU yuv2rgb decoding
	createYUVtoRGBtables();

	// create worker threads
	TheoraWorkerThread* t;
	for (int i=0;i<1;i++)
	{
		t=new TheoraWorkerThread();
		t->startThread();
		mWorkerThreads.push_back(t);
	}
}

TheoraVideoManager::~TheoraVideoManager()
{
	mWorkMutex->lock(); // to avoid sync problems. in case a thread is asking for work, and we delete the mutex halfway through

	ThreadList::iterator ti;
	for (ti=mWorkerThreads.begin(); ti != mWorkerThreads.end();ti++)
		delete (*ti);
	mWorkerThreads.clear();

	ClipList::iterator ci;
	for (ci=mClips.begin(); ci != mClips.end();ci++)
		delete (*ci);
	mClips.clear();
	delete mWorkMutex;
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

TheoraVideoClip* TheoraVideoManager::createVideoClip(std::string filename)
{
	TheoraDataSource* src=new TheoraFileDataSource(filename);
	return createVideoClip(src);
}

TheoraVideoClip* TheoraVideoManager::createVideoClip(TheoraDataSource* data_source)
{
	TheoraVideoClip* clip = NULL;
	writelog("Creating video from data source: "+data_source->repr());
	clip = new TheoraVideoClip(data_source,32);
	mClips.push_back(clip);
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
/*
bool TheoraVideoManager::frameStarted(const FrameEvent& evt)
{
	ClipList::iterator ci;
	for (ci=mClips.begin(); ci != mClips.end();ci++)
	{
		(*ci)->blitFrameCheck(evt.timeSinceLastFrame);
		(*ci)->decodedAudioCheck();
	}
	return true;
}
*/

TheoraVideoClip* TheoraVideoManager::requestWork(TheoraWorkerThread* caller)
{
	if (!mWorkMutex) return NULL;
	mWorkMutex->lock();
	TheoraVideoClip* c=NULL;

	static unsigned int counter=0;
	if (counter >= mClips.size()) counter=0;
	int i=counter;
	counter++;
	ClipList::iterator it;
	if (mClips.size() == 0) c=NULL;
	else
	{
		for (it=mClips.begin(); it != mClips.end(); it++)
		{
			/* priority based scheduling, unstable and experimental at the moment
			int p,lp=0xfffffff;
			p=(*it)->getPriority();
			if (p < lp)
			{
				lp=p;
				c=*it;
			}*/
			if (i == 0) { c=*it; break; }
			i--;

		}
		c->mAssignedWorkerThread=caller;
	}
	mWorkMutex->unlock();
	return c;
}
