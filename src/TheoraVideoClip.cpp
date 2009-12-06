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
#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreTextureManager.h"
  #include "OgreMaterialManager.h"
  #include "OgreMaterial.h"
  #include "OgreTechnique.h"
  #include "OgreStringConverter.h"
  #include "OgreLogManager.h"
  #include "OgreHardwarePixelBuffer.h"
#else
  #include <Ogre/OgreTextureManager.h>
  #include <Ogre/OgreMaterialManager.h>
  #include <Ogre/OgreMaterial.h>
  #include <Ogre/OgreTechnique.h>
  #include <Ogre/OgreStringConverter.h>
  #include <Ogre/OgreLogManager.h>
  #include <Ogre/OgreHardwarePixelBuffer.h>
#endif
#include <ptypes/pasync.h>

#include "TheoraVideoClip.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoFrame.h"
#include "TheoraFrameQueue.h"
#include "TheoraAudioInterface.h"
#include "TheoraTimer.h"

int gNameCounter=0;

namespace Ogre
{
	int nextPow2(int x)
	{
		int y;
		for (y=1;y<x;y*=2);
		return y;

	}
	//! clears a portion of memory with an unsign
	void memset_uint(void* buffer,unsigned int colour,unsigned int size_in_bytes)
	{
		unsigned int* data=(unsigned int*) buffer;
		for (unsigned int i=0;i<size_in_bytes;i+=4)
		{
			*data=colour;
			data++;
		}
	}

	TheoraVideoClip::TheoraVideoClip(std::string name,int nPrecachedFrames):
		mTheoraStreams(0),
		mVorbisStreams(0),
		mSeekPos(-1),
		mDuration(-1),
		mName(name),
		mOutputMode(TH_RGB),
		mBackColourChanged(0),
		mAudioInterface(NULL),
		mAutoRestart(0),
		mAudioGain(1),
		mEndOfFile(0),
		mTheoraDecoder(0),
		mTheoraSetup(0),
		mTexture(0),
		mAudioSkipSeekFlag(0)
	{
		mAudioMutex=new pt::mutex();

		mTimer=mDefaultTimer=new TheoraTimer();

		mFrameQueue=NULL;
		mAssignedWorkerThread=NULL;
		mNumPrecachedFrames=nPrecachedFrames;

		//Ensure all structures get cleared out.
		memset(&mOggSyncState, 0, sizeof(ogg_sync_state));
		memset(&mOggPage, 0, sizeof(ogg_page));
		memset(&mVorbisStreamState, 0, sizeof(ogg_stream_state));
		memset(&mTheoraStreamState, 0, sizeof(ogg_stream_state));
		memset(&mTheoraInfo, 0, sizeof(th_info));
		memset(&mTheoraComment, 0, sizeof(th_comment));
		memset(&mVorbisInfo, 0, sizeof(vorbis_info));
		memset(&mVorbisDSPState, 0, sizeof(vorbis_dsp_state));
		memset(&mVorbisBlock, 0, sizeof(vorbis_block));
		memset(&mVorbisComment, 0, sizeof(vorbis_comment));
	}

	TheoraVideoClip::~TheoraVideoClip()
	{
		// wait untill a worker thread is done decoding the frame
		while (mAssignedWorkerThread)
		{
			pt::psleep(1);
		}

		delete mDefaultTimer;

		if (!(mStream.isNull()))
		{
			mStream->close();
			mStream.setNull();
		}

		if (mFrameQueue) delete mFrameQueue;

		if (mTheoraDecoder)
			th_decode_free(mTheoraDecoder);

		if (mTheoraSetup)
			th_setup_free(mTheoraSetup);

		if (!mTexture.isNull())
		{
			String name=mTexture->getName();
			mTexture.setNull();
			TextureManager::getSingleton().unload(name);
			
		}

		if (mAudioInterface)
		{
			mAudioMutex->lock(); // ensure a thread isn't using this mutex

		// probably not necesarry because all it does is memset to 0
		//	ogg_stream_clear(&mVorbisStreamState);
		//	vorbis_block_clear(&mVorbisBlock);
		//	vorbis_dsp_clear(&mVorbisDSPState);
		//	vorbis_comment_clear(&mVorbisComment);
		//	vorbis_info_clear(&mVorbisInfo);
			mAudioInterface->destroy(); // notify audio interface it's time to call it a day
		}
		delete mAudioMutex;

		//ogg_sync_clear(&mOggSyncState);
	}

	String TheoraVideoClip::getMaterialName()
	{
		return mMaterialName;
	}

	TheoraTimer* TheoraVideoClip::getTimer()
	{
		return mTimer;
	}

	void TheoraVideoClip::setTimer(TheoraTimer* timer)
	{
		if (!timer) mTimer=mDefaultTimer;
		else mTimer=timer;
	}

	void TheoraVideoClip::decodeNextFrame()
	{
		if (mEndOfFile || mTimer->isPaused() && getNumPrecachedFrames() > 0) return;
		TheoraVideoFrame* frame=mFrameQueue->requestEmptyFrame();
		if (!frame) return; // max number of precached frames reached
		long seek_granule=-1;
		ogg_packet opTheora;
		ogg_int64_t granulePos;
		th_ycbcr_buffer buff;

		//LogManager::getSingleton().logMessage("Decoding video "+mName);

		for(;;)
		{
			int ret=ogg_stream_packetout(&mTheoraStreamState,&opTheora);

			if (ret > 0)
			{
				if (th_decode_packetin(mTheoraDecoder, &opTheora,&granulePos ) != 0) continue; // 0 means success
				float time=th_granule_time(mTheoraDecoder,granulePos);
				
				if (mSeekPos == -2)
				{	
					if (!th_packet_iskeyframe(&opTheora)) continue; // get keyframe after seek
					else
					{
						mSeekPos=-3; // -3 means we need to ensure the frame is displayed (otherwise it won't be if the video is paused)
						
						//if we use audio we must maintain perfect sync
					/*	if (seek_granule != -1)
						{
							float vorbis_time=vorbis_granule_time(&mVorbisDSPState,seek_granule);
							mTimer->seek(vorbis_time);
						}
					*/
					}
					
				}
				
				if (time < mTimer->getTime()) continue; // drop frame
				frame->mTimeToDisplay=time;
				th_decode_ycbcr_out(mTheoraDecoder,buff);
				frame->decode(buff);
				break;
			}
			else
			{
				char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
				int bytesRead = mStream->read( buffer, 4096 );
				ogg_sync_wrote( &mOggSyncState, bytesRead );
				if (bytesRead < 4096)
				{
					if (bytesRead == 0)
					{
						mEndOfFile=true;
						return;
					}
				}
				while ( ogg_sync_pageout( &mOggSyncState, &mOggPage ) > 0 )
				{
					if (mTheoraStreams) ogg_stream_pagein(&mTheoraStreamState,&mOggPage);
					if (mAudioInterface)// && mSeekPos != -2)
					{
						
						if (mSeekPos == -2 && !mAudioSkipSeekFlag)
						{
							int g=ogg_page_granulepos(&mOggPage);
							if (g > -1) { mAudioSkipSeekFlag=1; continue; }
							if (g == -1) continue;
						}
						mAudioMutex->lock();
						ogg_stream_pagein(&mVorbisStreamState,&mOggPage);
						mAudioMutex->unlock();
					}
					/*
					else if (mAudioInterface && mSeekPos == -2)
					{
						if ( ogg_page_serialno(&mOggPage) == mVorbisStreamState.serialno)
						{
							int g=ogg_page_granulepos(&mOggPage);
							if (g != -1) seek_granule=g;
							
						}
					}
					*/
				}
			}
		}
	}

	void TheoraVideoClip::blitFrameCheck(float time_increase)
	{
		if (mTimer->isPaused() && mSeekPos != -3) return;
		TheoraVideoFrame* frame;
		mTimer->update(time_increase);
		
		while (true)
		{
			frame=mFrameQueue->getFirstAvailableFrame();
			if (!frame)
			{
				if (mEndOfFile && mAutoRestart)
				{
					long granule=0;
					th_decode_ctl(mTheoraDecoder,TH_DECCTL_SET_GRANPOS,&granule,sizeof(granule));
					th_decode_free(mTheoraDecoder);
					mTheoraDecoder=th_decode_alloc(&mTheoraInfo,mTheoraSetup);
					ogg_stream_reset(&mTheoraStreamState);
					ogg_sync_reset(&mOggSyncState);
					mStream->seek(0);
					mTimer->seek(0);
					mEndOfFile=false;
					mFrameQueue->clear();
				}
				return; // no frames ready
			}
			//if (frame->mTimeToDisplay-mTimer->getTime() > 1)
			//{
			//	mFrameQueue->pop(); // this happens on Auto restart if there are some leftover frames
			//	continue;
			//}
			if (frame->mTimeToDisplay > mTimer->getTime()) return;
			if (frame->mTimeToDisplay < mTimer->getTime()-0.1)
			{
				mFrameQueue->pop();
			}
			else break;
		}
		if (mSeekPos == -3)
			mSeekPos=-1; // -3 ensures the first frame after seek gets displayed even when the movie is paused

		// use blitFromMemory or smtg faster
		unsigned char* texData=(unsigned char*) mTexture->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
		memcpy(texData,frame->getBuffer(),mTexWidth*mHeight*4);
		if (mBackColourChanged)
		{
			memset_uint(texData+mTexWidth*mHeight*4,mFrameQueue->getBackColour(),mTexWidth*(mTexHeight-mHeight)*4);
			mBackColourChanged=false;
		}

		mTexture->getBuffer()->unlock();

		mFrameQueue->pop(); // after transfering frame data to the texture, free the frame
		                    // so it can be used again
	}

	void TheoraVideoClip::decodedAudioCheck()
	{
		if (!mAudioInterface || mTimer->isPaused()) return;

		mAudioMutex->lock();

		ogg_packet opVorbis;
		float **pcm;
		int len=0;
		while (1)
		{
			len = vorbis_synthesis_pcmout(&mVorbisDSPState,&pcm);
			if (!len)
			{
				if (ogg_stream_packetout(&mVorbisStreamState,&opVorbis) > 0)
				{
					if (vorbis_synthesis(&mVorbisBlock,&opVorbis) == 0)
						vorbis_synthesis_blockin(&mVorbisDSPState,&mVorbisBlock);
					continue;
				}
				else break;
			}
			if (mAudioGain < 1)
			{
				// gain applied, let's attenuate the samples
				for (int i=0;i<len;i++)
				{
					for (int j=0;j<mVorbisInfo.channels;j++)
					{
						pcm[j][i]*=mAudioGain;
					}
				}
			}
			mAudioInterface->insertData(pcm,len);
			vorbis_synthesis_read(&mVorbisDSPState,len);
		}
		
		mAudioMutex->unlock();
	}

	void TheoraVideoClip::createDefinedTexture(const String& name, const String& material_name,
                              const String& group_name, int technique_level, int pass_level, 
			                  int tex_level)
	{
		mName=name;
		load(name,group_name);

		mMaterialName=material_name;
		mTechniqueLevel=technique_level;
		mPassLevel=pass_level;
		mTexLevel=tex_level;
		// create texture
		
		String texname="theora_texture_"+StringConverter::toString(gNameCounter++);

		mTexture = TextureManager::getSingleton().createManual(texname,group_name,TEX_TYPE_2D,
			mTexWidth,mTexHeight,1,0,PF_X8R8G8B8,TU_DYNAMIC_WRITE_ONLY);
		// clear it to black
		unsigned char* texData=(unsigned char*) mTexture->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
		if (mOutputMode == TH_YUV)
			memset_uint(texData,0xFF008080,mTexWidth*mTexHeight*4); // (0,128,128) is YUV->RGB for Black
		else
			memset(texData,0,mTexWidth*mTexHeight*4);

		mTexture->getBuffer()->unlock();

		// attach it to a material
		MaterialPtr material = MaterialManager::getSingleton().getByName(mMaterialName);
		TextureUnitState* t = material->getTechnique(mTechniqueLevel)->\
			getPass(mPassLevel)->getTextureUnitState(mTexLevel);

		//Now, attach the texture to the material texture unit (single layer) and setup properties
		t->setTextureName(texname,TEX_TYPE_2D);
		t->setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_NONE);
		t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

		// scale tex coords to fit the 0-1 uv range
		Matrix4 mat=Matrix4::IDENTITY;
		mat.setScale(Vector3((float) mWidth/mTexWidth, (float) mHeight/mTexHeight,1));
		t->setTextureTransform(mat);
	}

	void TheoraVideoClip::load(const String& file_name,const String& group_name)
	{
		if (!(mStream.isNull()))
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "ogg_video "+file_name+" already loded!",
			             "TheoraVideoClip::load" );
	
		//mEndOfFile = false;
		mStream = ResourceGroupManager::getSingleton().openResource( file_name, group_name );


		readTheoraVorbisHeaders();


		mTheoraDecoder=th_decode_alloc(&mTheoraInfo,mTheoraSetup);

		mWidth=mTheoraInfo.frame_width;
		mHeight=mTheoraInfo.frame_height;
		mTexWidth = nextPow2(mWidth);
		mTexHeight= nextPow2(mHeight);

		mFrameQueue=new TheoraFrameQueue(mNumPrecachedFrames,this);
		setOutputMode(mOutputMode); // clear the frame backgrounds

		//return;
		// find out the duration of the file by seeking to the end
		// having ogg decode pages, extract the granule pos from
		// the last theora page and seek back to beginning of the file

		long stream_pos=mStream->tell();

		for (int i=1;i<=10;i++)
		{
			ogg_sync_reset(&mOggSyncState);
			mStream->seek(mStream->size()-4096*i);

			char *buffer = ogg_sync_buffer( &mOggSyncState, 4096*i);
			int bytesRead = mStream->read( buffer, 4096*i);
			ogg_sync_wrote( &mOggSyncState, bytesRead );
			long offset=ogg_sync_pageseek(&mOggSyncState,&mOggPage);

			while (1)
			{
				int ret=ogg_sync_pageout( &mOggSyncState, &mOggPage );
				if (ret < 0)
					ret=ogg_sync_pageout( &mOggSyncState, &mOggPage );
				if ( ret < 0) break;

				int eos=ogg_page_eos(&mOggPage);
				if (eos > 0) break;

				int serno=ogg_page_serialno(&mOggPage);
				// if page is not a theora page, skip it
				if (serno != mTheoraStreamState.serialno) continue;

				long granule=ogg_page_granulepos(&mOggPage);
				if (granule >= 0)
					mDuration=th_granule_time(mTheoraDecoder,granule);
			}
			if (mDuration > 0) break;

		}
		if (mDuration < 0)
		{
			LogManager::getSingleton().logMessage("TheoraVideoPlugin: unable to determine file duration!");
		}
		// restore to beginning of stream.
		// the following solution is temporary and hacky, will be replaced soon

		ogg_sync_reset(&mOggSyncState);
		mStream->seek(0);
		memset(&mOggSyncState, 0, sizeof(ogg_sync_state));
		memset(&mOggPage, 0, sizeof(ogg_page));
		memset(&mVorbisStreamState, 0, sizeof(ogg_stream_state));
		memset(&mTheoraStreamState, 0, sizeof(ogg_stream_state));
		memset(&mTheoraInfo, 0, sizeof(th_info));
		memset(&mTheoraComment, 0, sizeof(th_comment));
		//memset(&mTheoraState, 0, sizeof(th_state));
		memset(&mVorbisInfo, 0, sizeof(vorbis_info));
		memset(&mVorbisDSPState, 0, sizeof(vorbis_dsp_state));
		memset(&mVorbisBlock, 0, sizeof(vorbis_block));
		memset(&mVorbisComment, 0, sizeof(vorbis_comment));
		mTheoraStreams=mVorbisStreams=0;
		readTheoraVorbisHeaders();

		// END HACKY CODE

		if (mVorbisStreams) // if there is no audio interface factory defined, even though the video
			                // clip might have audio, it will be ignored
		{
			vorbis_synthesis_init(&mVorbisDSPState,&mVorbisInfo);
			vorbis_block_init(&mVorbisDSPState,&mVorbisBlock);
			// create an audio interface instance if available
			TheoraAudioInterfaceFactory* audio_factory=TheoraVideoManager::getSingleton().getAudioInterfaceFactory();
			if (audio_factory) setAudioInterface(audio_factory->createInstance(this,mVorbisInfo.channels,mVorbisInfo.rate));
		}
	}

	void TheoraVideoClip::readTheoraVorbisHeaders()
	{
		ogg_packet tempOggPacket;
		bool done = false;
		bool decode_audio=TheoraVideoManager::getSingleton().getAudioInterfaceFactory() != NULL;
		//init Vorbis/Theora Layer
		ogg_sync_init(&mOggSyncState);
		th_comment_init(&mTheoraComment);
		th_info_init(&mTheoraInfo);
		vorbis_info_init(&mVorbisInfo);
		vorbis_comment_init(&mVorbisComment);

		while (!done)
		{
			char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
			int bytesRead = mStream->read( buffer, 4096 );
			ogg_sync_wrote( &mOggSyncState, bytesRead );
		
			if( bytesRead == 0 )
				break;
		
			while( ogg_sync_pageout( &mOggSyncState, &mOggPage ) > 0 )
			{
				ogg_stream_state OggStateTest;
	    		
				//is this an initial header? If not, stop
				if( !ogg_page_bos( &mOggPage ) )
				{
					//This is done blindly, because stream only accept them selfs
					if (mTheoraStreams) 
						ogg_stream_pagein( &mTheoraStreamState, &mOggPage );
					if (mVorbisStreams) 
						ogg_stream_pagein( &mVorbisStreamState, &mOggPage );
					
					done=true;
					break;
				}
		
				ogg_stream_init( &OggStateTest, ogg_page_serialno( &mOggPage ) );
				ogg_stream_pagein( &OggStateTest, &mOggPage );
				ogg_stream_packetout( &OggStateTest, &tempOggPacket );

				//identify the codec
				int ret;
				if( !mTheoraStreams)
				{
					ret=th_decode_headerin( &mTheoraInfo, &mTheoraComment, &mTheoraSetup, &tempOggPacket);

					if (ret > 0)
					{
						//This is the Theora Header
						memcpy( &mTheoraStreamState, &OggStateTest, sizeof(OggStateTest));
						mTheoraStreams = 1;
						continue;
					}
				}
				if (decode_audio && !mVorbisStreams &&
					vorbis_synthesis_headerin(&mVorbisInfo, &mVorbisComment, &tempOggPacket) >=0 )
				{
					//This is vorbis header
					memcpy( &mVorbisStreamState, &OggStateTest, sizeof(OggStateTest));
					mVorbisStreams = 1;
					continue;
				}
				
				//Hmm. I guess it's not a header we support, so erase it
				ogg_stream_clear(&OggStateTest);
			}
		}

		while ((mTheoraStreams && (mTheoraStreams < 3)) ||
			   (mVorbisStreams && (mVorbisStreams < 3)) )
		{
			//Check 2nd'dary headers... Theora First
			int iSuccess;
			while( mTheoraStreams && 
				 ( mTheoraStreams < 3) && 
				 ( iSuccess = ogg_stream_packetout( &mTheoraStreamState, &tempOggPacket)) ) 
			{
				if( iSuccess < 0 ) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Error parsing Theora stream headers.",
						"TheoraVideoClip::readTheoraVorbisHeaders" );

				if( !th_decode_headerin(&mTheoraInfo, &mTheoraComment, &mTheoraSetup, &tempOggPacket) )
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "invalid stream",
						"TheoraVideoClip::readTheoraVorbisHeaders ");

				mTheoraStreams++;			
			} //end while looking for more theora headers
		
			//look 2nd vorbis header packets
			while(// mVorbisStreams && 
				 ( mVorbisStreams < 3 ) && 
				 ( iSuccess=ogg_stream_packetout( &mVorbisStreamState, &tempOggPacket))) 
			{
				if(iSuccess < 0) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Error parsing vorbis stream headers",
						"TheoraVideoClip::readTheoraVorbisHeaders ");

				if(vorbis_synthesis_headerin( &mVorbisInfo, &mVorbisComment,&tempOggPacket)) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "invalid stream",
						"TheoraVideoClip::readTheoraVorbisHeaders ");

				mVorbisStreams++;
			} //end while looking for more vorbis headers
		
			//Not finished with Headers, get some more file data
			if( ogg_sync_pageout( &mOggSyncState, &mOggPage ) > 0 )
			{
				if(mTheoraStreams) 
					ogg_stream_pagein( &mTheoraStreamState, &mOggPage );
				if(mVorbisStreams) 
					ogg_stream_pagein( &mVorbisStreamState, &mOggPage );
			}
			else
			{
				char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
				int bytesRead = mStream->read( buffer, 4096 );
				ogg_sync_wrote( &mOggSyncState, bytesRead );

				if( bytesRead == 0 )
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "End of file found prematurely",
						"TheoraVideoClip::parseVorbisTheoraHeaders " );
			}
		} //end while looking for all headers

		String temp1 = StringConverter::toString( mVorbisStreams );
		String temp2 = StringConverter::toString( mTheoraStreams );
		LogManager::getSingleton().logMessage("Vorbis Headers: " + temp1 + " Theora Headers : " + temp2);
	}

	String TheoraVideoClip::getName()
	{
		return mName;
	}

	TheoraOutputMode TheoraVideoClip::getOutputMode()
	{
		return mOutputMode;
	}

	void TheoraVideoClip::setOutputMode(TheoraOutputMode mode)
	{
		// Yuv black is (0,128,128) and grey/rgb (0,0,0) so we need to make sure we
		// clear our frames to that colour so we won't get border pixels in different colour
		if (mode == TH_YUV) mFrameQueue->fillBackColour(0xFF008080);
		else                mFrameQueue->fillBackColour(0xFF000000);
		mOutputMode=mode;
		mBackColourChanged=true;
	}

	float TheoraVideoClip::getTimePosition()
	{
		return mTimer->getTime();
	}
	int TheoraVideoClip::getNumPrecachedFrames()
	{
		return mFrameQueue->getUsedCount();
	}

	float TheoraVideoClip::getDuration()
	{
		return mDuration;
	}

	void TheoraVideoClip::play()
	{
		mTimer->play();
	}

	void TheoraVideoClip::pause()
	{
		mTimer->pause();
	}

	bool TheoraVideoClip::isPaused()
	{
		return mTimer->isPaused();
	}
    
    bool TheoraVideoClip::isDone()
    {
        return mEndOfFile && !mFrameQueue->getFirstAvailableFrame();
    }

	void TheoraVideoClip::stop()
	{
		pause();
		seek(0);
	}

	void TheoraVideoClip::doSeek()
	{
		int i,seek_min=0, seek_max=mStream->size();
		float time;
		ogg_int64_t granule,th_granule;

		mFrameQueue->clear();
		ogg_stream_reset(&mTheoraStreamState);
		th_decode_free(mTheoraDecoder);
		if (mAudioInterface)
		{
			vorbis_synthesis_restart(&mVorbisDSPState);
			ogg_stream_reset(&mVorbisStreamState);
			mAudioMutex->lock();
		}
		mTheoraDecoder=th_decode_alloc(&mTheoraInfo,mTheoraSetup);

		for (i=0;i<10;i++) // max 10 seek operations
		{
			granule=0;
			ogg_sync_reset( &mOggSyncState );
			mStream->seek((seek_min+seek_max)/2);

			memset(&mOggPage, 0, sizeof(ogg_page));
			ogg_sync_pageseek(&mOggSyncState,&mOggPage);
			while (1)
			{
				int ret=ogg_sync_pageout( &mOggSyncState, &mOggPage );
				if (ret == 1)
				{
					//ogg_stream_pagein( &mTheoraStreamState, &mOggPage );

					int serno=ogg_page_serialno(&mOggPage);
					if (serno == mTheoraStreamState.serialno)
					{
						int g=ogg_page_granulepos(&mOggPage);
						if (g > -1) th_granule=g;
					}
					// if audio is available, use vorbis for time positioning
					// othervise use theora
					if ( mAudioInterface && serno == mVorbisStreamState.serialno ||
						     !mAudioInterface && serno == mTheoraStreamState.serialno)
					{
						if (granule <= 0) granule=ogg_page_granulepos(&mOggPage);
						if (granule >= 0 && th_granule >= 0) break;
					}
					else continue; // unknown page (could be flac or whatever)
					int eos=ogg_page_eos(&mOggPage);
					if (eos > 0) break;
				}
				else
				{
					char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
					int bytesRead = mStream->read( buffer, 4096);
					if (bytesRead < 4096) break;
					ogg_sync_wrote( &mOggSyncState, bytesRead );
				}
			}
			if (mAudioInterface)
				time=vorbis_granule_time(&mVorbisDSPState,granule);
			else
				time=th_granule_time(mTheoraDecoder,granule);
			if (time <= mSeekPos && time-mSeekPos < 0.5 && time-mSeekPos >= 0) break; // ok, we're close enough
			
			if (time < mSeekPos) seek_min=(seek_min+seek_max)/2;
			else				 seek_max=(seek_min+seek_max)/2;
		}
		ogg_sync_reset( &mOggSyncState );
		th_decode_ctl(mTheoraDecoder,TH_DECCTL_SET_GRANPOS,&th_granule,sizeof(th_granule));
		mTimer->seek(time); // this will be changed in decodeNextFrame when seeking to the next keyframe
		mStream->seek((seek_min+seek_max)/2);
		mSeekPos=-2;

		mAudioSkipSeekFlag=0;

		if (mAudioInterface) mAudioMutex->unlock();
	}

	void TheoraVideoClip::seek(float time)
	{
		mSeekPos=time;
		mEndOfFile=false;
	}

	float TheoraVideoClip::getPriority()
	{
		// prioritize based on number of precached frames
		// this way, we get even distribution of work among worker threads
		// in the future this function will involve to include user priorities
		// and to de-prioritize paused videos etc
		return getNumPrecachedFrames()*10;
	}

	void TheoraVideoClip::setAudioInterface(TheoraAudioInterface* iface)
	{
		mAudioInterface=iface;
	}
	
	TheoraAudioInterface* TheoraVideoClip::getAudioInterface()
	{
		return mAudioInterface;
	}

	void TheoraVideoClip::setAudioGain(float gain)
	{
		if (gain > 1) mAudioGain=1;
		if (gain < 0) mAudioGain=0;
		else          mAudioGain=gain;
	}

	float TheoraVideoClip::getAudioGain()
	{
		return mAudioGain;
	}

	void TheoraVideoClip::setAutoRestart(bool value)
	{
		mAutoRestart=value;
		if (value) mEndOfFile=false;
	}

	bool TheoraVideoClip::getAutoRestart()
	{
		return mAutoRestart;
	}

	int TheoraVideoClip::getWidth()
	{
		return mWidth;	
	}

	int TheoraVideoClip::getHeight()
	{
		return mHeight;
	}

	int TheoraVideoClip::getTextureWidth()
	{
		return mTexWidth;
	}

	int TheoraVideoClip::getTextureHeight()
	{
		return mTexHeight;
	}

	TexturePtr TheoraVideoClip::getTexture()
	{
		return mTexture;
	}

} // end namespace Ogre
