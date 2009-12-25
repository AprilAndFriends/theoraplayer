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
#include "TheoraVideoClip.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoFrame.h"
#include "TheoraFrameQueue.h"
#include "TheoraAudioInterface.h"
#include "TheoraTimer.h"
#include "TheoraDataSource.h"
#include "TheoraUtil.h"
#include "TheoraException.h"
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <theora/theoradec.h>

class TheoraInfoStruct
{
public:
	// ogg/vorbis/theora variables
	ogg_sync_state   OggSyncState;
	ogg_page         OggPage;
	ogg_stream_state VorbisStreamState;
	ogg_stream_state TheoraStreamState;
	//Theora State
	th_info        TheoraInfo;
	th_comment     TheoraComment;
	th_setup_info* TheoraSetup;
	th_dec_ctx*    TheoraDecoder;
	//Vorbis State
	vorbis_info      VorbisInfo;
	vorbis_dsp_state VorbisDSPState;
	vorbis_block     VorbisBlock;
	vorbis_comment   VorbisComment;

	TheoraInfoStruct()
	{
		TheoraDecoder=0;
		TheoraSetup=0;
	}
};

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

TheoraVideoClip::TheoraVideoClip(TheoraDataSource* data_source,
								 TheoraOutputMode output_mode,
								 int nPrecachedFrames,
								 bool usePower2Stride):
	mTheoraStreams(0),
	mVorbisStreams(0),
	mSeekPos(-1),
	mDuration(-1),
	mName(data_source->repr()),
	mOutputMode(output_mode),
	mAudioInterface(NULL),
	mAutoRestart(0),
	mAudioGain(1),
	mEndOfFile(0),
	mNumDroppedFrames(0),
	mNumDisplayedFrames(0),
	mAudioSkipSeekFlag(0),
	mIteration(0),
	mLastIteration(0),
	mRestarted(0),
	mStride(usePower2Stride)
{
	mAudioMutex=new TheoraMutex;

	mTimer=mDefaultTimer=new TheoraTimer();

	mFrameQueue=NULL;
	mAssignedWorkerThread=NULL;
	mNumPrecachedFrames=nPrecachedFrames;

	mInfo=new TheoraInfoStruct;
	//Ensure all structures get cleared out.
	memset(&mInfo->OggSyncState, 0, sizeof(ogg_sync_state));
	memset(&mInfo->OggPage, 0, sizeof(ogg_page));
	memset(&mInfo->VorbisStreamState, 0, sizeof(ogg_stream_state));
	memset(&mInfo->TheoraStreamState, 0, sizeof(ogg_stream_state));
	memset(&mInfo->TheoraInfo, 0, sizeof(th_info));
	memset(&mInfo->TheoraComment, 0, sizeof(th_comment));
	memset(&mInfo->VorbisInfo, 0, sizeof(vorbis_info));
	memset(&mInfo->VorbisDSPState, 0, sizeof(vorbis_dsp_state));
	memset(&mInfo->VorbisBlock, 0, sizeof(vorbis_block));
	memset(&mInfo->VorbisComment, 0, sizeof(vorbis_comment));

	load(data_source);
}

TheoraVideoClip::~TheoraVideoClip()
{
	// wait untill a worker thread is done decoding the frame
	while (mAssignedWorkerThread)
	{
		_psleep(1);
	}

	delete mDefaultTimer;

	if (!mStream)
	{
		delete mStream;
		mStream=0;
	}

	if (mFrameQueue) delete mFrameQueue;

	if (mInfo->TheoraDecoder)
		th_decode_free(mInfo->TheoraDecoder);

	if (mInfo->TheoraSetup)
		th_setup_free(mInfo->TheoraSetup);
	delete mInfo;

	if (mAudioInterface)
	{
		mAudioMutex->lock(); // ensure a thread isn't using this mutex

	// probably not necesarry because all it does is memset to 0
	//	ogg_stream_clear(&mInfo->VorbisStreamState);
	//	vorbis_block_clear(&mInfo->VorbisBlock);
	//	vorbis_dsp_clear(&mInfo->VorbisDSPState);
	//	vorbis_comment_clear(&mInfo->VorbisComment);
	//	vorbis_info_clear(&mInfo->VorbisInfo);
		mAudioInterface->destroy(); // notify audio interface it's time to call it a day
	}
	delete mAudioMutex;

	//ogg_sync_clear(&mInfo->OggSyncState);
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

	//writelog("Decoding video "+mName);

	for(;;)
	{
		int ret=ogg_stream_packetout(&mInfo->TheoraStreamState,&opTheora);

		if (ret > 0)
		{
			if (th_decode_packetin(mInfo->TheoraDecoder, &opTheora,&granulePos ) != 0) continue; // 0 means success
			float time=(float) th_granule_time(mInfo->TheoraDecoder,granulePos);
			unsigned long frame_number=(unsigned long) th_granule_frame(mInfo->TheoraDecoder,granulePos);
			if (time > mDuration) mDuration=time; // duration corrections

			if (mSeekPos == -2)
			{
				if (!th_packet_iskeyframe(&opTheora)) continue; // get keyframe after seek
				else
				{
					mSeekPos=-3; // -3 means we need to ensure the frame is displayed (otherwise it won't be if the video is paused)
					
					//if we use audio we must maintain perfect sync
				/*	if (seek_granule != -1)
					{
						float vorbis_time=vorbis_granule_time(&mInfo->VorbisDSPState,seek_granule);
						mTimer->seek(vorbis_time);
					}
				*/
				}
			}

			if (time < mTimer->getTime() && !mRestarted)
			{
				th_writelog("pre-dropped frame "+str(frame_number));
				mNumDisplayedFrames++;
				mNumDroppedFrames++;
				continue; // drop frame
			}
			frame->mTimeToDisplay=time;
			frame->mIteration=mIteration;
			frame->_setFrameNumber(frame_number);
			th_decode_ycbcr_out(mInfo->TheoraDecoder,buff);
			frame->decode(buff);
			//_psleep(rand()%20); // temp
			break;
		}
		else
		{
			char *buffer = ogg_sync_buffer( &mInfo->OggSyncState, 4096);
			int bytesRead = mStream->read( buffer, 4096 );
			ogg_sync_wrote(&mInfo->OggSyncState, bytesRead);
			if (bytesRead < 4096)
			{
				if (bytesRead == 0)
				{
					if (mAutoRestart) _restart();
					else mEndOfFile=true;
					frame->mInUse=0;
					return;
				}
			}
			while ( ogg_sync_pageout( &mInfo->OggSyncState, &mInfo->OggPage ) > 0 )
			{
				if (mTheoraStreams) ogg_stream_pagein(&mInfo->TheoraStreamState,&mInfo->OggPage);
				if (mAudioInterface)// && mSeekPos != -2)
				{
					
					if (mSeekPos == -2 && !mAudioSkipSeekFlag)
					{
						unsigned long g=(unsigned long) ogg_page_granulepos(&mInfo->OggPage);
						if (g > -1) { mAudioSkipSeekFlag=1; continue; }
						if (g == -1) continue;
					}
					mAudioMutex->lock();
					ogg_stream_pagein(&mInfo->VorbisStreamState,&mInfo->OggPage);
					mAudioMutex->unlock();
				}
				/*
				else if (mAudioInterface && mSeekPos == -2)
				{
					if ( ogg_page_serialno(&mInfo->OggPage) == mInfo->VorbisStreamState.serialno)
					{
						int g=ogg_page_granulepos(&mInfo->OggPage);
						if (g != -1) seek_granule=g;
						
					}
				}
				*/
			}
		}
	}
}

void TheoraVideoClip::_restart()
{
	long granule=0;
	th_decode_ctl(mInfo->TheoraDecoder,TH_DECCTL_SET_GRANPOS,&granule,sizeof(granule));
	th_decode_free(mInfo->TheoraDecoder);
	mInfo->TheoraDecoder=th_decode_alloc(&mInfo->TheoraInfo,mInfo->TheoraSetup);
	ogg_stream_reset(&mInfo->TheoraStreamState);
	ogg_sync_reset(&mInfo->OggSyncState);
	mStream->seek(0);
	mEndOfFile=false;

	mRestarted=1;
}

void TheoraVideoClip::restart()
{
	_restart();
	mTimer->seek(0);
	mFrameQueue->clear();
}

void TheoraVideoClip::update(float time_increase)
{
	if (mTimer->isPaused() && mSeekPos != -3) return;
	float time=mTimer->getTime();
	if (time >= mDuration)
	{
		if (mAutoRestart && mRestarted)
		{
			mIteration=!mIteration;
			mTimer->seek(0);
			mRestarted=0;
			int n=0;
			for (;;)
			{
				TheoraVideoFrame* f=mFrameQueue->getFirstAvailableFrame();
				if (!f) break;
				if (f->mTimeToDisplay > 0.5f) { n++; popFrame(); }
				else break;
			}
			if (n > 0) th_writelog("dropped "+str(n)+" end frames");
		}
		else return;
	}
	if (time+time_increase > mDuration) time_increase=mDuration-time;
	mTimer->update(time_increase);
}

void TheoraVideoClip::popFrame()
{
	mNumDisplayedFrames++;
	mFrameQueue->pop(); // after transfering frame data to the texture, free the frame
						// so it can be used again
}

TheoraVideoFrame* TheoraVideoClip::getNextFrame()
{
	TheoraVideoFrame* frame;
	float time=mTimer->getTime();
	for (;;)
	{
		frame=mFrameQueue->getFirstAvailableFrame();
		if (!frame) return 0;
		if (frame->mTimeToDisplay > time) return 0;
		if (frame->mTimeToDisplay < time-0.1)
		{
			th_writelog("dropped frame "+str(frame->getFrameNumber()));
			mNumDroppedFrames++;
			mNumDisplayedFrames++;
			mFrameQueue->pop();
		}
		else break;
	}

	mLastIteration=frame->mIteration;
	return frame;

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
		len = vorbis_synthesis_pcmout(&mInfo->VorbisDSPState,&pcm);
		if (!len)
		{
			if (ogg_stream_packetout(&mInfo->VorbisStreamState,&opVorbis) > 0)
			{
				if (vorbis_synthesis(&mInfo->VorbisBlock,&opVorbis) == 0)
					vorbis_synthesis_blockin(&mInfo->VorbisDSPState,&mInfo->VorbisBlock);
				continue;
			}
			else break;
		}
		if (mAudioGain < 1)
		{
			// gain applied, let's attenuate the samples
			for (int i=0;i<len;i++)
			{
				for (int j=0;j<mInfo->VorbisInfo.channels;j++)
				{
					pcm[j][i]*=mAudioGain;
				}
			}
		}
		mAudioInterface->insertData(pcm,len);
		vorbis_synthesis_read(&mInfo->VorbisDSPState,len);
	}
	
	mAudioMutex->unlock();
}

void TheoraVideoClip::load(TheoraDataSource* source)
{
	mStream=source;
	readTheoraVorbisHeaders();


	mInfo->TheoraDecoder=th_decode_alloc(&mInfo->TheoraInfo,mInfo->TheoraSetup);

	mWidth=mInfo->TheoraInfo.frame_width;
	mHeight=mInfo->TheoraInfo.frame_height;
	mStride=(mStride == 1) ? mStride=nextPow2(mWidth) : mWidth;

	mFrameQueue=new TheoraFrameQueue(mNumPrecachedFrames,this);
	

	//return;
	// find out the duration of the file by seeking to the end
	// having ogg decode pages, extract the granule pos from
	// the last theora page and seek back to beginning of the file

	long stream_pos=mStream->tell();

	for (int i=1;i<=10;i++)
	{
		ogg_sync_reset(&mInfo->OggSyncState);
		mStream->seek(mStream->size()-4096*i);

		char *buffer = ogg_sync_buffer( &mInfo->OggSyncState, 4096*i);
		int bytesRead = mStream->read( buffer, 4096*i);
		ogg_sync_wrote( &mInfo->OggSyncState, bytesRead );
		long offset=ogg_sync_pageseek(&mInfo->OggSyncState,&mInfo->OggPage);

		while (1)
		{
			int ret=ogg_sync_pageout( &mInfo->OggSyncState, &mInfo->OggPage );
			if (ret < 0)
				ret=ogg_sync_pageout( &mInfo->OggSyncState, &mInfo->OggPage );
			if ( ret < 0) break;

			int eos=ogg_page_eos(&mInfo->OggPage);
			if (eos > 0) break;

			int serno=ogg_page_serialno(&mInfo->OggPage);
			// if page is not a theora page, skip it
			if (serno != mInfo->TheoraStreamState.serialno) continue;

			unsigned long granule=(unsigned long) ogg_page_granulepos(&mInfo->OggPage);
			if (granule >= 0)
				mDuration=(float) th_granule_time(mInfo->TheoraDecoder,granule);
		}
		if (mDuration > 0) break;

	}
	if (mDuration < 0)
	{
		th_writelog("TheoraVideoPlugin: unable to determine file duration!");
	}
	// restore to beginning of stream.
	// the following solution is temporary and hacky, will be replaced soon

	ogg_sync_reset(&mInfo->OggSyncState);
	mStream->seek(0);
	memset(&mInfo->OggSyncState, 0, sizeof(ogg_sync_state));
	memset(&mInfo->OggPage, 0, sizeof(ogg_page));
	memset(&mInfo->VorbisStreamState, 0, sizeof(ogg_stream_state));
	memset(&mInfo->TheoraStreamState, 0, sizeof(ogg_stream_state));
	memset(&mInfo->TheoraInfo, 0, sizeof(th_info));
	memset(&mInfo->TheoraComment, 0, sizeof(th_comment));
	//memset(&mTheoraState, 0, sizeof(th_state));
	memset(&mInfo->VorbisInfo, 0, sizeof(vorbis_info));
	memset(&mInfo->VorbisDSPState, 0, sizeof(vorbis_dsp_state));
	memset(&mInfo->VorbisBlock, 0, sizeof(vorbis_block));
	memset(&mInfo->VorbisComment, 0, sizeof(vorbis_comment));
	mTheoraStreams=mVorbisStreams=0;
	readTheoraVorbisHeaders();

	// END HACKY CODE

	if (mVorbisStreams) // if there is no audio interface factory defined, even though the video
		                // clip might have audio, it will be ignored
	{
		vorbis_synthesis_init(&mInfo->VorbisDSPState,&mInfo->VorbisInfo);
		vorbis_block_init(&mInfo->VorbisDSPState,&mInfo->VorbisBlock);
		// create an audio interface instance if available
		TheoraAudioInterfaceFactory* audio_factory=TheoraVideoManager::getSingleton().getAudioInterfaceFactory();
		if (audio_factory) setAudioInterface(audio_factory->createInstance(this,mInfo->VorbisInfo.channels,mInfo->VorbisInfo.rate));
	}
}

void TheoraVideoClip::readTheoraVorbisHeaders()
{
	ogg_packet tempOggPacket;
	bool done = false;
	bool decode_audio=TheoraVideoManager::getSingleton().getAudioInterfaceFactory() != NULL;
	//init Vorbis/Theora Layer
	ogg_sync_init(&mInfo->OggSyncState);
	th_comment_init(&mInfo->TheoraComment);
	th_info_init(&mInfo->TheoraInfo);
	vorbis_info_init(&mInfo->VorbisInfo);
	vorbis_comment_init(&mInfo->VorbisComment);

	while (!done)
	{
		char *buffer = ogg_sync_buffer( &mInfo->OggSyncState, 4096);
		int bytesRead = mStream->read( buffer, 4096 );
		ogg_sync_wrote( &mInfo->OggSyncState, bytesRead );
	
		if( bytesRead == 0 )
			break;
	
		while( ogg_sync_pageout( &mInfo->OggSyncState, &mInfo->OggPage ) > 0 )
		{
			ogg_stream_state OggStateTest;
    		
			//is this an initial header? If not, stop
			if( !ogg_page_bos( &mInfo->OggPage ) )
			{
				//This is done blindly, because stream only accept them selfs
				if (mTheoraStreams) 
					ogg_stream_pagein( &mInfo->TheoraStreamState, &mInfo->OggPage );
				if (mVorbisStreams) 
					ogg_stream_pagein( &mInfo->VorbisStreamState, &mInfo->OggPage );
				
				done=true;
				break;
			}
	
			ogg_stream_init( &OggStateTest, ogg_page_serialno( &mInfo->OggPage ) );
			ogg_stream_pagein( &OggStateTest, &mInfo->OggPage );
			ogg_stream_packetout( &OggStateTest, &tempOggPacket );

			//identify the codec
			int ret;
			if( !mTheoraStreams)
			{
				ret=th_decode_headerin( &mInfo->TheoraInfo, &mInfo->TheoraComment, &mInfo->TheoraSetup, &tempOggPacket);

				if (ret > 0)
				{
					//This is the Theora Header
					memcpy( &mInfo->TheoraStreamState, &OggStateTest, sizeof(OggStateTest));
					mTheoraStreams = 1;
					continue;
				}
			}
			if (decode_audio && !mVorbisStreams &&
				vorbis_synthesis_headerin(&mInfo->VorbisInfo, &mInfo->VorbisComment, &tempOggPacket) >=0 )
			{
				//This is vorbis header
				memcpy( &mInfo->VorbisStreamState, &OggStateTest, sizeof(OggStateTest));
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
			 ( iSuccess = ogg_stream_packetout( &mInfo->TheoraStreamState, &tempOggPacket)) ) 
		{
			if( iSuccess < 0 ) 
				throw TheoraGenericException("Error parsing Theora stream headers.");

			if( !th_decode_headerin(&mInfo->TheoraInfo, &mInfo->TheoraComment, &mInfo->TheoraSetup, &tempOggPacket) )
				throw TheoraGenericException("invalid theora stream");

			mTheoraStreams++;			
		} //end while looking for more theora headers
	
		//look 2nd vorbis header packets
		while(// mVorbisStreams && 
			 ( mVorbisStreams < 3 ) && 
			 ( iSuccess=ogg_stream_packetout( &mInfo->VorbisStreamState, &tempOggPacket))) 
		{
			if(iSuccess < 0) 
				throw TheoraGenericException("Error parsing vorbis stream headers");

			if(vorbis_synthesis_headerin( &mInfo->VorbisInfo, &mInfo->VorbisComment,&tempOggPacket)) 
				throw TheoraGenericException("invalid stream");

			mVorbisStreams++;
		} //end while looking for more vorbis headers
	
		//Not finished with Headers, get some more file data
		if( ogg_sync_pageout( &mInfo->OggSyncState, &mInfo->OggPage ) > 0 )
		{
			if(mTheoraStreams) 
				ogg_stream_pagein( &mInfo->TheoraStreamState, &mInfo->OggPage );
			if(mVorbisStreams) 
				ogg_stream_pagein( &mInfo->VorbisStreamState, &mInfo->OggPage );
		}
		else
		{
			char *buffer = ogg_sync_buffer( &mInfo->OggSyncState, 4096);
			int bytesRead = mStream->read( buffer, 4096 );
			ogg_sync_wrote( &mInfo->OggSyncState, bytesRead );

			if( bytesRead == 0 )
				throw TheoraGenericException("End of file found prematurely");
		}
	} //end while looking for all headers
//	writelog("Vorbis Headers: " + str(mVorbisHeaders) + " Theora Headers : " + str(mTheoraHeaders));
}

std::string TheoraVideoClip::getName()
{
	return mName;
}

TheoraOutputMode TheoraVideoClip::getOutputMode()
{
	return mOutputMode;
}

void TheoraVideoClip::setOutputMode(TheoraOutputMode mode)
{
	if (mFrameQueue) th_writelog("Warning: Can't change output mode once video has been loaded!");
	else mOutputMode=mode;
}

float TheoraVideoClip::getTimePosition()
{
	return mTimer->getTime();
}
int TheoraVideoClip::getNumPrecachedFrames()
{
	return mFrameQueue->getSize();
}

void TheoraVideoClip::setNumPrecachedFrames(int n)
{
	if (mFrameQueue->getSize() != n)
		mFrameQueue->setSize(n);
}

int TheoraVideoClip::getNumReadyFrames()
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
	ogg_stream_reset(&mInfo->TheoraStreamState);
	th_decode_free(mInfo->TheoraDecoder);
	if (mAudioInterface)
	{
		vorbis_synthesis_restart(&mInfo->VorbisDSPState);
		ogg_stream_reset(&mInfo->VorbisStreamState);
		mAudioMutex->lock();
	}
	mInfo->TheoraDecoder=th_decode_alloc(&mInfo->TheoraInfo,mInfo->TheoraSetup);

	for (i=0;i<10;i++) // max 10 seek operations
	{
		granule=0;
		ogg_sync_reset( &mInfo->OggSyncState );
		mStream->seek((seek_min+seek_max)/2);

		memset(&mInfo->OggPage, 0, sizeof(ogg_page));
		ogg_sync_pageseek(&mInfo->OggSyncState,&mInfo->OggPage);
		while (1)
		{
			int ret=ogg_sync_pageout( &mInfo->OggSyncState, &mInfo->OggPage );
			if (ret == 1)
			{
				//ogg_stream_pagein( &mInfo->TheoraStreamState, &mInfo->OggPage );

				int serno=ogg_page_serialno(&mInfo->OggPage);
				if (serno == mInfo->TheoraStreamState.serialno)
				{
					unsigned long g=(unsigned long) ogg_page_granulepos(&mInfo->OggPage);
					if (g > -1) th_granule=g;
				}
				// if audio is available, use vorbis for time positioning
				// othervise use theora
				if ( mAudioInterface && serno == mInfo->VorbisStreamState.serialno ||
					     !mAudioInterface && serno == mInfo->TheoraStreamState.serialno)
				{
					if (granule <= 0) granule=ogg_page_granulepos(&mInfo->OggPage);
					if (granule >= 0 && th_granule >= 0) break;
				}
				else continue; // unknown page (could be flac or whatever)
				int eos=ogg_page_eos(&mInfo->OggPage);
				if (eos > 0) break;
			}
			else
			{
				char *buffer = ogg_sync_buffer( &mInfo->OggSyncState, 4096);
				int bytesRead = mStream->read( buffer, 4096);
				if (bytesRead < 4096) break;
				ogg_sync_wrote( &mInfo->OggSyncState, bytesRead );
			}
		}
		if (mAudioInterface)
			time=(float) vorbis_granule_time(&mInfo->VorbisDSPState,granule);
		else
			time=(float) th_granule_time(mInfo->TheoraDecoder,granule);
		if (time <= mSeekPos && time-mSeekPos < 0.5 && time-mSeekPos >= 0) break; // ok, we're close enough
		
		if (time < mSeekPos) seek_min=(seek_min+seek_max)/2;
		else				 seek_max=(seek_min+seek_max)/2;
	}
	ogg_sync_reset( &mInfo->OggSyncState );
	th_decode_ctl(mInfo->TheoraDecoder,TH_DECCTL_SET_GRANPOS,&th_granule,sizeof(th_granule));
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
	// TODO
	return getNumPrecachedFrames()*10.0f;
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
