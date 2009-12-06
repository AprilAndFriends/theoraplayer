/************************************************************************************
This source file is part of the TheoraVideoPlugin ExternalTextureSource PlugIn 
for OGRE3D (Object-oriented Graphics Rendering Engine)
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright � 2008-2009 Kresimir Spes (kreso@cateia.com)

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

#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreExternalTextureSource.h"
  #include "OgreFrameListener.h"
#else
  #include <Ogre/OgreExternalTextureSource.h>
  #include <Ogre/OgreFrameListener.h>
#endif
#include "TheoraExport.h"
#include <list>

namespace pt
{
	struct mutex;
}

namespace Ogre
{
	// forward class declarations
	class TheoraWorkerThread;
	class TheoraVideoClip;
	class TheoraAudioInterfaceFactory;
	/**
		This is the main class that interfaces with Ogre, parses material files
		and distributes decoding jobs among threads.
	*/
	class _OgreTheoraExport TheoraVideoManager : public Singleton<TheoraVideoManager>, 
		                                         public ExternalTextureSource,
												 public FrameListener
	{
		friend class TheoraWorkerThread;

		typedef std::list<TheoraWorkerThread*> ThreadList;
		//! stores pointers to worker threads which are decoding video and audio
		ThreadList mWorkerThreads;
		typedef std::list<TheoraVideoClip*> ClipList;
		//! stores pointers to created video clips		
		ClipList mClips;

		int mDefaultNumPrecachedFrames;
		//! whether the plugin has been initialised
		bool mbInit;

		pt::mutex* mWorkMutex;

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

		/**
			@remarks
				This function is called to init this plugin - do not call directly
		*/
		bool initialise();
		void shutDown();

		TheoraVideoClip* getVideoClipByName(String name);
		//! returns the first video assigned to the given material name
		TheoraVideoClip* getVideoClipByMaterialName(String material_name);


		/**
			@remarks
				Creates a texture into an already defined material
				All setting should have been set before calling this.
				Refer to base class ( ExternalTextureSource ) for details
			@param material_name
				Material  you are attaching a movie to.
		*/
		void createDefinedTexture(const String& material_name,
                                  const String& group_name = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		/**
			@remarks
				Destroys a Video Texture based on material name. Mostly Ogre uses this,
				you should use destroyVideoClip()
			@param material_name
				Material Name you are looking to remove a video clip from
		*/
		void destroyAdvancedTexture(const String& material_name,
                                    const String& groupName = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		/**
			@remarks
				Destroys a video clip
		*/
		void destroyVideoClip(TheoraVideoClip* clip);

		bool frameStarted(const FrameEvent& evt);

        bool setParameter(const String &name,const String &value);
        String getParameter(const String &name) const;
        
		void setAudioInterfaceFactory(TheoraAudioInterfaceFactory* factory);
		TheoraAudioInterfaceFactory* getAudioInterfaceFactory();

		int getNumWorkerThreads();
		void setNumWorkerThreads(int n);

		void setDefaultNumPrecachedFrames(int n);
		int getDefaultNumPrecachedFrames();

	};
}
#endif

