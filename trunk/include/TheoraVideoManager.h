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

#ifndef _TheoraVideoManager_h
#define _TheoraVideoManager_h

#include <vector>
#include "TheoraExport.h"


// forward class declarations
class TheoraWorkerThread;
class TheoraMutex;
class TheoraDataSource;
class TheoraVideoClip;
class TheoraAudioInterfaceFactory;
/**
	This is the main singleton class that handles all playback/sync operations
*/
class TheoraPlayerExport TheoraVideoManager
{
	friend class TheoraWorkerThread;
	typedef std::vector<TheoraVideoClip*> ClipList;
	typedef std::vector<TheoraWorkerThread*> ThreadList;

	//! stores pointers to worker threads which are decoding video and audio
	ThreadList mWorkerThreads;
	//! stores pointers to created video clips		
	ClipList mClips;
	int mDefaultNumPrecachedFrames;
	//! whether the plugin has been initialised
	bool mbInit;

	TheoraMutex* mWorkMutex;

	TheoraAudioInterfaceFactory* mAudioFactory;

	/**
	 * Called by TheoraWorkerThread to request a TheoraVideoClip instance to work on decoding
	 */
	TheoraVideoClip* requestWork(TheoraWorkerThread* caller);
public:
	TheoraVideoManager();
	~TheoraVideoManager();

	static TheoraVideoManager& getSingleton(void);
	static TheoraVideoManager* getSingletonPtr(void);

	TheoraVideoClip* getVideoClipByName(std::string name);

	TheoraVideoClip* createVideoClip(std::string filename);
	TheoraVideoClip* createVideoClip(TheoraDataSource* data_source);

	/**
		@remarks
			Destroys a video clip
	*/
	void destroyVideoClip(TheoraVideoClip* clip);
    
	void setAudioInterfaceFactory(TheoraAudioInterfaceFactory* factory);
	TheoraAudioInterfaceFactory* getAudioInterfaceFactory();

	int getNumWorkerThreads();
	void setNumWorkerThreads(int n);

	void setDefaultNumPrecachedFrames(int n);
	int getDefaultNumPrecachedFrames();

};
#endif

