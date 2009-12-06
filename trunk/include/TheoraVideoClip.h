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

#ifndef _TheoraVideoClip_h
#define _TheoraVideoClip_h

#include "TheoraExport.h"
#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreDataStream.h"
  #include "OgreTexture.h"
#else
  #include <Ogre/OgreDataStream.h>
  #include <Ogre/OgreTexture.h>
#endif
#include <theora/theoradec.h>
#include <vorbis/codec.h>

enum TheoraOutputMode
{
	TH_RGB,
	TH_GREY,
	TH_YUV
};

namespace pt
{
	struct mutex;
};

namespace Ogre
{
	class TheoraFrameQueue;
	class TheoraTimer;
	class TheoraAudioInterface;
	class TheoraWorkerThread;
	

	/**
		
	*/
	class _OgreTheoraExport TheoraVideoClip
	{
		friend class TheoraWorkerThread;
		friend class TheoraVideoFrame;
		friend class TheoraVideoManager;

		TheoraFrameQueue* mFrameQueue;
		TheoraAudioInterface* mAudioInterface;
		DataStreamPtr mStream;
		TexturePtr mTexture;

		TheoraTimer *mTimer,*mDefaultTimer;

		TheoraWorkerThread* mAssignedWorkerThread;

		// ogg/vorbis/theora variables
		ogg_sync_state   mOggSyncState;
		ogg_page         mOggPage;
		ogg_stream_state mVorbisStreamState;
		ogg_stream_state mTheoraStreamState;
		//Theora State
		th_info        mTheoraInfo;
		th_comment     mTheoraComment;
		th_setup_info* mTheoraSetup;
		th_dec_ctx*    mTheoraDecoder;
		//th_state     mTheoraState;
		//Vorbis State
		vorbis_info      mVorbisInfo;
		vorbis_dsp_state mVorbisDSPState;
		vorbis_block     mVorbisBlock;
		vorbis_comment   mVorbisComment;
		int mTheoraStreams, mVorbisStreams;	// Keeps track of Theora and Vorbis Streams

		int mNumPrecachedFrames;
		int mAudioSkipSeekFlag;

		String mName;
		int mWidth,mHeight;
		int mTexWidth,mTexHeight;
		float mDuration;

		float mAudioGain; //! multiplier for audio samples. between 0 and 1
		float mSeekPos; //! stores desired seek position. next worker thread will do the seeking and reset this var to -1
		TheoraOutputMode mOutputMode;

		// material binding information
		String mMaterialName;
		int mTechniqueLevel;
		int mPassLevel;
		int mTexLevel;
		bool mBackColourChanged;
		bool mAutoRestart;
		bool mEndOfFile;

		float mUserPriority;


		pt::mutex* mAudioMutex; //! syncs audio decoding and extraction

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
	public:
		TheoraVideoClip(std::string name,int nPrecachedFrames);
		~TheoraVideoClip();

		String getName();
		//! return the material name this video clip is assigned to
		String getMaterialName();

		//! return width in pixels of the video clip
		int getWidth();
		//! return height in pixels of the video clip
		int getHeight();
		//! return actual width that the texture uses (nearest power of two dimension)
		int getTextureWidth();
		//! return actual height that the texture uses (nearest power of two dimension)
		int getTextureHeight();
		//! return texture pointer
		TexturePtr getTexture();

		TheoraTimer* getTimer();
		void setTimer(TheoraTimer* timer);

		void createDefinedTexture(
			const String& name, const String& material_name,
			const String& group_name, int technique_level, int pass_level, 
			int tex_level);

		void decodeNextFrame();

		//! check if it's time to transfer another frame to the texture
		void blitFrameCheck(float time_increase);
		//! check if there is enough audio data decoded to submit to the audio interface
		void decodedAudioCheck();

		void load(const String& file_name,const String& group_name);

		void setAudioInterface(TheoraAudioInterface* iface);
		TheoraAudioInterface* getAudioInterface();

		void setNumPrecachedFrames(int n);
		int getNumPrecachedFrames();

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
		bool isPaused();
		void stop();
		void seek(float time);
	};
}
#endif

