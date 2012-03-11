/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include <memory.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <theora/theoradec.h>
#include "TheoraVideoClip.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoFrame.h"
#include "TheoraFrameQueue.h"
#include "TheoraAudioInterface.h"
#include "TheoraTimer.h"
#include "TheoraDataSource.h"
#include "TheoraUtil.h"
#include "TheoraException.h"

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
	mAudioInterface(NULL),
	mNumDroppedFrames(0),
	mNumDisplayedFrames(0),
	mTheoraStreams(0),
	mVorbisStreams(0),
	mSeekFrame(-1),
	mDuration(-1),
	mNumFrames(-1),
	mFPS(1),
	mFrameDuration(0),
    mName(data_source->repr()),
    mStride(usePower2Stride),
    mAudioGain(1),
	mTheoraAudioPacketQueue(0),
    mOutputMode(output_mode),
    mRequestedOutputMode(output_mode),
    mAutoRestart(0),
    mEndOfFile(0),
    mRestarted(0),
	mIteration(0),
	mLastIteration(0)
{
	mAudioMutex=new TheoraMutex;

	mTimer=mDefaultTimer=new TheoraTimer();

	mFrameQueue=NULL;
	mAssignedWorkerThread=NULL;
	mNumPrecachedFrames=nPrecachedFrames;

	mInfo=new TheoraInfoStruct;

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

	if (!mStream) delete mStream;


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
        
        mAudioMutex->unlock();
	}
	delete mAudioMutex;

	destroyAllAudioPackets();

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

bool TheoraVideoClip::_readData()
{
	int audio_eos = 0, serno;
	float audio_time = 0;
	float time = mTimer->getTime();
	if (mRestarted) time = 0;

	for (;;)
	{
		char *buffer = ogg_sync_buffer(&mInfo->OggSyncState, 4096);
		int bytes_read = mStream->read(buffer, 4096);
		ogg_sync_wrote(&mInfo->OggSyncState, bytes_read);

		if (bytes_read < 4096)
		{
			if (bytes_read == 0)
			{
				if (!mAutoRestart) mEndOfFile = true;
				return 0;
			}
		}
		// when we fill the stream with enough pages, it'll start spitting out packets
		// which contain keyframes, delta frames or audio data
		while (ogg_sync_pageout(&mInfo->OggSyncState, &mInfo->OggPage) > 0)
		{
			serno = ogg_page_serialno(&mInfo->OggPage);
			if (serno == mInfo->TheoraStreamState.serialno) ogg_stream_pagein(&mInfo->TheoraStreamState, &mInfo->OggPage);
			if (mAudioInterface && serno == mInfo->VorbisStreamState.serialno)
			{
				ogg_int64_t g = ogg_page_granulepos(&mInfo->OggPage);
				audio_time = (float) vorbis_granule_time(&mInfo->VorbisDSPState, g);
				audio_eos = ogg_page_eos(&mInfo->OggPage);	
				ogg_stream_pagein(&mInfo->VorbisStreamState, &mInfo->OggPage);
			}
		}
		if (!(mAudioInterface && !audio_eos && audio_time < time + 1.0f))
			break;
	}
	return 1;
}

void TheoraVideoClip::decodeNextFrame()
{
	if (mEndOfFile) return;

	TheoraVideoFrame* frame = mFrameQueue->requestEmptyFrame();
	if (!frame) return; // max number of precached frames reached
	bool should_restart = 0;
	ogg_packet opTheora;
	ogg_int64_t granulePos;
	th_ycbcr_buffer buff;
	for (;;)
	{
		int ret = ogg_stream_packetout(&mInfo->TheoraStreamState, &opTheora);

		if (ret > 0)
		{
			if (th_decode_packetin(mInfo->TheoraDecoder, &opTheora, &granulePos) != 0) continue; // 0 means success
			float time = (float) th_granule_time(mInfo->TheoraDecoder, granulePos);
			unsigned long frame_number = (unsigned long) th_granule_frame(mInfo->TheoraDecoder, granulePos);

			if (time < mTimer->getTime() && !mRestarted)
			{
#ifdef _DEBUG
				th_writelog(mName + ": pre-dropped frame " + str(frame_number));
#endif
				mNumDisplayedFrames++;
				mNumDroppedFrames++;
				continue; // drop frame
			}
			frame->mTimeToDisplay = time - mFrameDuration;
			frame->mIteration     = mIteration;
			frame->_setFrameNumber(frame_number);
			th_decode_ycbcr_out(mInfo->TheoraDecoder, buff);
			frame->decode(buff);
			break;
		}
		else
		{
			if (!_readData())
			{
				frame->mInUse = 0;
				should_restart = mAutoRestart;
				break;
			}
		}
	}

	if (mAudioInterface != NULL)
	{
		mAudioMutex->lock();
		decodeAudio();
		mAudioMutex->unlock();
	}
	if (should_restart) _restart();
}

void TheoraVideoClip::_restart()
{
	bool paused = mTimer->isPaused();
	if (!paused) mTimer->pause();
	long granule=0;
	th_decode_ctl(mInfo->TheoraDecoder,TH_DECCTL_SET_GRANPOS,&granule,sizeof(granule));
	th_decode_free(mInfo->TheoraDecoder);
	mInfo->TheoraDecoder=th_decode_alloc(&mInfo->TheoraInfo,mInfo->TheoraSetup);
	ogg_stream_reset(&mInfo->TheoraStreamState);
	if (mAudioInterface)
	{
		// empty the DSP buffer
		//float **pcm;
		//int len = vorbis_synthesis_pcmout(&mInfo->VorbisDSPState,&pcm);
		//if (len) vorbis_synthesis_read(&mInfo->VorbisDSPState,len);
		ogg_packet opVorbis;
		while (ogg_stream_packetout(&mInfo->VorbisStreamState,&opVorbis) > 0)
		{
			if (vorbis_synthesis(&mInfo->VorbisBlock,&opVorbis) == 0)
				vorbis_synthesis_blockin(&mInfo->VorbisDSPState,&mInfo->VorbisBlock);
		}
		ogg_stream_reset(&mInfo->VorbisStreamState);
	}

	ogg_sync_reset(&mInfo->OggSyncState);
	mStream->seek(0);
	ogg_int64_t granulePos = 0;
	th_decode_ctl(mInfo->TheoraDecoder, TH_DECCTL_SET_GRANPOS, &granulePos, sizeof(granule));

	mEndOfFile=false;

	mRestarted=1;

	if (!paused) mTimer->play();
}

void TheoraVideoClip::restart()
{
	mEndOfFile=1; //temp, to prevent threads to decode while restarting
	while (mAssignedWorkerThread) _psleep(1); // wait for assigned thread to do it's work
	_restart();
	mTimer->seek(0);
	mFrameQueue->clear();
	mEndOfFile=0;
	mIteration=0;
	mRestarted=0;
	mSeekFrame = -1;
}

void TheoraVideoClip::update(float time_increase)
{
	if (mTimer->isPaused()) return;
	mTimer->update(time_increase);
	float time=mTimer->getTime();
	if (time >= mDuration)
	{
		if (mAutoRestart && mRestarted)
		{
			mIteration=!mIteration;
			mTimer->seek(time-mDuration);
			mRestarted=0;
			int n=0;
			for (;;)
			{
				TheoraVideoFrame* f=mFrameQueue->getFirstAvailableFrame();
				if (!f) break;
				if (f->mTimeToDisplay > 0.5f)
				{
					if (n == 0)
					{
						f->mTimeToDisplay=time-mDuration;
						f->mIteration=!f->mIteration;
					}
					else
						popFrame();
					n++;
				}
				else break;
			}
			if (n > 0) th_writelog("dropped "+str(n)+" end frames");
		}
		else return;
	}
}

float TheoraVideoClip::updateToNextFrame()
{
	TheoraVideoFrame* f=mFrameQueue->getFirstAvailableFrame();
	if (!f) return 0;

	float time=f->mTimeToDisplay-mTimer->getTime();
	update(time);
	return time;
}

void TheoraVideoClip::popFrame()
{
	mNumDisplayedFrames++;
	mFrameQueue->pop(); // after transfering frame data to the texture, free the frame
						// so it can be used again
}

TheoraVideoFrame* TheoraVideoClip::getNextFrame()
{
	if (mSeekFrame != -1) return 0; // if we are about to seek, then the current frame queue is invalidated
	                                // (will be cleared when a worker thread does the actual seek)
	TheoraVideoFrame* frame;
	float time=mTimer->getTime();
	for (;;)
	{
		frame=mFrameQueue->getFirstAvailableFrame();
		if (!frame) return 0;
		if (frame->mTimeToDisplay > time) return 0;
		// only drop frames if you have more frames to show. otherwise even the late frame will do..
		if (time > frame->mTimeToDisplay + mFrameDuration && mFrameQueue->getReadyCount() > 1) 
		{
			if (mRestarted && frame->mTimeToDisplay < 2) return 0;
#ifdef _DEBUG
			th_writelog(mName+": dropped frame "+str(frame->getFrameNumber()));
#endif
			mNumDroppedFrames++;
			mNumDisplayedFrames++;
			mFrameQueue->pop();
		}
		else break;
	}

	mLastIteration=frame->mIteration;
	return frame;

}

void TheoraVideoClip::load(TheoraDataSource* source)
{
	th_writelog("-----");
	mStream = source;
	readTheoraVorbisHeaders();

	mInfo->TheoraDecoder = th_decode_alloc(&mInfo->TheoraInfo,mInfo->TheoraSetup);

	mWidth = mInfo->TheoraInfo.frame_width; // TODO: pic_width should be used and accounted for while decoding
	mHeight = mInfo->TheoraInfo.frame_height;
	mStride = (mStride == 1) ? mStride=_nextPow2(mWidth) : mWidth;
	mFPS = mInfo->TheoraInfo.fps_numerator / (float) mInfo->TheoraInfo.fps_denominator;

	th_writelog("width: " + str(mWidth) + ", height: " + str(mHeight) + ", fps: " + str((int) getFPS()));
	mFrameQueue = new TheoraFrameQueue(mNumPrecachedFrames, this);
	// find out the duration of the file by seeking to the end
	// having ogg decode pages, extract the granule pos from
	// the last theora page and seek back to beginning of the file
	for (int i = 1; i <= 50; i++)
	{
		ogg_sync_reset(&mInfo->OggSyncState);
		mStream->seek(mStream->size() - 4096 * i);

		char *buffer = ogg_sync_buffer(&mInfo->OggSyncState, 4096 * i);
		int bytes_read = mStream->read(buffer, 4096 * i);
		ogg_sync_wrote(&mInfo->OggSyncState, bytes_read);
		ogg_sync_pageseek(&mInfo->OggSyncState, &mInfo->OggPage);

		for (;;)
		{
			int ret = ogg_sync_pageout(&mInfo->OggSyncState, &mInfo->OggPage);
			if (ret == 0) break;
			// if page is not a theora page, skip it
			if (ogg_page_serialno(&mInfo->OggPage) != mInfo->TheoraStreamState.serialno) continue;

			ogg_int64_t granule = ogg_page_granulepos(&mInfo->OggPage);
			if (granule >= 0)
			{
				mNumFrames = (unsigned long) th_granule_frame(mInfo->TheoraDecoder, granule) + 1;
			}
			else if (mNumFrames > 0)
				mNumFrames++; // append delta frames at the end to get the exact numbe
		}
		if (mNumFrames > 0) break;

	}
	if (mNumFrames < 0)
		th_writelog("unable to determine file duration!");
	else
	{
		mDuration = mNumFrames / mFPS;
		th_writelog("duration: " + strf(mDuration) + " seconds");
	}
	// restore to beginning of stream.
	ogg_sync_reset(&mInfo->OggSyncState);
	mStream->seek(0);

	if (mVorbisStreams) // if there is no audio interface factory defined, even though the video
		                // clip might have audio, it will be ignored
	{
		vorbis_synthesis_init(&mInfo->VorbisDSPState, &mInfo->VorbisInfo);
		vorbis_block_init(&mInfo->VorbisDSPState, &mInfo->VorbisBlock);
		// create an audio interface instance if available
		TheoraAudioInterfaceFactory* audio_factory = TheoraVideoManager::getSingleton().getAudioInterfaceFactory();
		if (audio_factory) setAudioInterface(audio_factory->createInstance(this, mInfo->VorbisInfo.channels, mInfo->VorbisInfo.rate));
	}

	mFrameDuration = 1.0f / getFPS();
	th_writelog("-----");
}

void TheoraVideoClip::readTheoraVorbisHeaders()
{
	ogg_packet tempOggPacket;
	bool done = false;
	bool decode_audio=TheoraVideoManager::getSingleton().getAudioInterfaceFactory() != NULL;
	//init Vorbis/Theora Layer
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

	ogg_sync_init(&mInfo->OggSyncState);
	th_comment_init(&mInfo->TheoraComment);
	th_info_init(&mInfo->TheoraInfo);
	vorbis_info_init(&mInfo->VorbisInfo);
	vorbis_comment_init(&mInfo->VorbisComment);

	while (!done)
	{
		char *buffer = ogg_sync_buffer( &mInfo->OggSyncState, 4096);
		int bytes_read = mStream->read( buffer, 4096 );
		ogg_sync_wrote( &mInfo->OggSyncState, bytes_read );

		if( bytes_read == 0 )
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
			int bytes_read = mStream->read( buffer, 4096 );
			ogg_sync_wrote( &mInfo->OggSyncState, bytes_read );

			if( bytes_read == 0 )
				throw TheoraGenericException("End of file found prematurely");
		}
	} //end while looking for all headers
//	writelog("Vorbis Headers: " + str(mVorbisHeaders) + " Theora Headers : " + str(mTheoraHeaders));
}

std::string TheoraVideoClip::getName()
{
	return mName;
}

bool TheoraVideoClip::isBusy()
{
	return mAssignedWorkerThread || mOutputMode != mRequestedOutputMode;
}

float TheoraVideoClip::decodeAudio()
{
	if (mRestarted) return -1;

	ogg_packet opVorbis;
	float **pcm;
	int len = 0;
	float timestamp = -1;
	bool read_past_timestamp = 0;

	for (;;)
	{
		len = vorbis_synthesis_pcmout(&mInfo->VorbisDSPState, &pcm);
		if (len == 0)
		{
			if (ogg_stream_packetout(&mInfo->VorbisStreamState, &opVorbis) > 0)
			{
				if (vorbis_synthesis(&mInfo->VorbisBlock, &opVorbis) == 0)
				{
					if (timestamp < 0 && opVorbis.granulepos >= 0)
					{
						timestamp = (float) vorbis_granule_time(&mInfo->VorbisDSPState, opVorbis.granulepos);
					}
					else if (timestamp >= 0) read_past_timestamp = 1;
					vorbis_synthesis_blockin(&mInfo->VorbisDSPState, &mInfo->VorbisBlock);
				}
				continue;
			}
			else break;
		}
		addAudioPacket(pcm, len);
		if (read_past_timestamp) timestamp += (float) len / mInfo->VorbisInfo.rate;
		vorbis_synthesis_read(&mInfo->VorbisDSPState, len); // tell vorbis we read a number of samples
	}
	return timestamp;
}

void TheoraVideoClip::decodedAudioCheck()
{
	if (!mAudioInterface || mTimer->isPaused()) return;

	mAudioMutex->lock();

	for (TheoraAudioPacket* p = popAudioPacket(); p != NULL; p = popAudioPacket())
	{
		mAudioInterface->insertData(p->pcm, p->num_samples);
		destroyAudioPacket(p);
	}
	mAudioMutex->unlock();
}

float TheoraVideoClip::getAudioPacketQueueLength()
{
	float len = 0;
	for (TheoraAudioPacket* p = mTheoraAudioPacketQueue; p != NULL; p = p->next)
		len += p->num_samples;

	return len / (mInfo->VorbisInfo.rate * mInfo->VorbisInfo.channels);
}

void TheoraVideoClip::addAudioPacket(float** buffer, int num_samples)
{
	TheoraAudioPacket* packet = new TheoraAudioPacket;

	float* data = new float[num_samples * mInfo->VorbisInfo.channels];
	float* dataptr = data;
	int i,j;

	if (mAudioGain < 1.0f)
	{
		// apply gain, let's attenuate the samples
		for (i = 0; i < num_samples; i++)
			for (j = 0; j < mInfo->VorbisInfo.channels; j++, dataptr++)
				*dataptr = buffer[i][j] * mAudioGain;
	}
	else
	{
		// do a simple copy, faster then the above method, when gain is 1.0f
		for (i = 0; i < num_samples; i++)
			for (j = 0; j < mInfo->VorbisInfo.channels; j++, dataptr++)
				*dataptr = buffer[j][i];
	}

	packet->pcm = data;
	packet->num_samples = num_samples * mInfo->VorbisInfo.channels;
	packet->next = NULL;

	if (mTheoraAudioPacketQueue == NULL) mTheoraAudioPacketQueue = packet;
	else
	{
		TheoraAudioPacket* last = mTheoraAudioPacketQueue;
		for (TheoraAudioPacket* p = last; p != NULL; p = p->next)
			last = p;
		last->next = packet;
	}
}

TheoraAudioPacket* TheoraVideoClip::popAudioPacket()
{
	if (mTheoraAudioPacketQueue == NULL) return NULL;
	TheoraAudioPacket* p = mTheoraAudioPacketQueue;
	mTheoraAudioPacketQueue = mTheoraAudioPacketQueue->next;
	return p;
}

void TheoraVideoClip::destroyAudioPacket(TheoraAudioPacket* p)
{
	if (p == NULL) return;
	delete [] p->pcm;
	delete p;
}

void TheoraVideoClip::destroyAllAudioPackets()
{
	for (TheoraAudioPacket* p = popAudioPacket(); p != NULL; p = popAudioPacket())
		destroyAudioPacket(p);
}

TheoraOutputMode TheoraVideoClip::getOutputMode()
{
	return mOutputMode;
}

void TheoraVideoClip::setOutputMode(TheoraOutputMode mode)
{
	if (mOutputMode == mode) return;
	mRequestedOutputMode=mode;
	while (mAssignedWorkerThread) _psleep(1);
	// discard current frames and recreate them
	mFrameQueue->setSize(mFrameQueue->getSize());
	mOutputMode=mRequestedOutputMode;
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
	if (mSeekFrame != -1) return 0; // we are about to seek, consider frame queue empty even though it will be emptied upon seek
	return mFrameQueue->getReadyCount();
}

float TheoraVideoClip::getDuration()
{
	return mDuration;
}

float TheoraVideoClip::getFPS()
{
	return mFPS;
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

void TheoraVideoClip::setPlaybackSpeed(float speed)
{
    mTimer->setSpeed(speed);
}

float TheoraVideoClip::getPlaybackSpeed()
{
    return mTimer->getSpeed();
}

long TheoraVideoClip::seekPage(long targetFrame, bool return_keyframe)
{
	int i,seek_min = 0, seek_max = mStream->size();
	long frame;
	ogg_int64_t granule = 0;

	if (targetFrame == 0) mStream->seek(0);
	for (i = (targetFrame == 0) ? 100 : 0; i < 100; i++)
	{
		ogg_sync_reset(&mInfo->OggSyncState);
		mStream->seek((seek_min + seek_max) / 2); // do a binary search
		memset(&mInfo->OggPage, 0, sizeof(ogg_page));
		ogg_sync_pageseek(&mInfo->OggSyncState, &mInfo->OggPage);

		for (;i < 1000;)
		{
			int ret = ogg_sync_pageout(&mInfo->OggSyncState, &mInfo->OggPage);
			if (ret == 1)
			{
				int serno = ogg_page_serialno(&mInfo->OggPage);
				if (serno == mInfo->TheoraStreamState.serialno)
				{
					granule = ogg_page_granulepos(&mInfo->OggPage);
					if (granule >= 0)
					{
						frame = (long) th_granule_frame(mInfo->TheoraDecoder, granule);
						if (frame < targetFrame && targetFrame - frame < 10)
						{
							// we're close enough, let's break this.
							i = 1000;
							break;
						}
						// we're not close enough, let's shorten the borders of the binary search
						if (targetFrame - 1 > frame) seek_min = (seek_min + seek_max) / 2;
						else				         seek_max = (seek_min + seek_max) / 2;
						break;
					}
				}
			}
			else
			{
				char *buffer = ogg_sync_buffer(&mInfo->OggSyncState, 4096);
				int bytes_read = mStream->read(buffer, 4096);
				if (bytes_read == 0) break;
				ogg_sync_wrote(&mInfo->OggSyncState, bytes_read);
			}
		}
	}
	if (return_keyframe) return (long) (granule >> mInfo->TheoraInfo.keyframe_granule_shift);

	ogg_sync_reset(&mInfo->OggSyncState);
	memset(&mInfo->OggPage, 0, sizeof(ogg_page));
	ogg_sync_pageseek(&mInfo->OggSyncState, &mInfo->OggPage);
	if (targetFrame == 0) return -1;
	mStream->seek((seek_min + seek_max) / 2); // do a binary search
	return -1;
}

void TheoraVideoClip::doSeek()
{
#if _DEBUG
	th_writelog(mName + " [seek]: seeking to frame " + str(mSeekFrame));
#endif
	int frame;
	float time = mSeekFrame / getFPS();
	mTimer->seek(time);
	bool paused = mTimer->isPaused();
	if (!paused) mTimer->pause(); // pause until seeking is done

	mEndOfFile = 0;
	mRestarted = 0;

	mFrameQueue->clear();
	// reset the video decoder.
	ogg_stream_reset(&mInfo->TheoraStreamState);
	th_decode_free(mInfo->TheoraDecoder);
	mInfo->TheoraDecoder = th_decode_alloc(&mInfo->TheoraInfo, mInfo->TheoraSetup);

	if (mAudioInterface)
	{
		mAudioMutex->lock();
		ogg_stream_reset(&mInfo->VorbisStreamState);
		vorbis_synthesis_restart(&mInfo->VorbisDSPState);
		destroyAllAudioPackets();
	}
	// first seek to desired frame, then figure out the location of the
	// previous keyframe and seek to it.
	// then by setting the correct time, the decoder will skip N frames untill
	// we get the frame we want.
	frame = seekPage(mSeekFrame, 1); // find the keyframe nearest to the target frame
#ifdef _DEBUG
//		th_writelog(mName + " [seek]: nearest keyframe for frame " + str(mSeekFrame) + " is frame: " + str(frame));
#endif
	seekPage(std::max(0, frame - 1), 0);

	ogg_packet opTheora;
	ogg_int64_t granulePos;
	bool granule_set = 0;
	if (frame <= 1)
	{
		if (mInfo->TheoraInfo.version_major == 3 && mInfo->TheoraInfo.version_minor == 2 && mInfo->TheoraInfo.version_subminor == 0)
			granulePos = 0;
		else
			granulePos = 1; // because of difference in granule interpretation in theora streams 3.2.0 and newer ones
		th_decode_ctl(mInfo->TheoraDecoder, TH_DECCTL_SET_GRANPOS, &granulePos, sizeof(granulePos));
		granule_set = 1;
	}

	// now that we've found the keyframe that preceeds our desired frame, lets keep on decoding frames until we
	// reach our target frame.

	for (;mSeekFrame != 0;)
	{
		int ret = ogg_stream_packetout(&mInfo->TheoraStreamState, &opTheora);
		if (ret > 0)
		{
			if (!granule_set)
			{
				// theora decoder requires to set the granule pos after seek to be able to determine the current frame
				if (opTheora.granulepos >= 0)
				{
					th_decode_ctl(mInfo->TheoraDecoder, TH_DECCTL_SET_GRANPOS, &opTheora.granulepos, sizeof(opTheora.granulepos));
					granule_set = 1;
				}
				else continue; // ignore prev delta frames until we hit a keyframe
			}
			if (th_decode_packetin(mInfo->TheoraDecoder, &opTheora, &granulePos) != 0) continue; // 0 means success
			frame = (int) th_granule_frame(mInfo->TheoraDecoder, granulePos);
			if (frame >= mSeekFrame - 1) break;
		}
		else
		{
			if (!_readData())
			{
				th_writelog(mName + " [seek]: fineseeking failed, _readData failed!");
				if (mAudioInterface) mAudioMutex->unlock();
				return;
			}
		}
	}
#ifdef _DEBUG
//	th_writelog(mName + " [seek]: fineseeked to frame " + str(frame + 1) + ", requested: " + str(mSeekFrame));
#endif
	if (mAudioInterface)
	{
		// read audio data until we reach a timestamp. this usually takes only one iteration, but just in case let's
		// wrap it in a loop
		float timestamp;
		for (;;)
		{
			timestamp = decodeAudio();
			if (timestamp >= 0) break;
			else _readData();
		}
		float rate = (float) mInfo->VorbisInfo.rate * mInfo->VorbisInfo.channels;
		float queued_time = getAudioPacketQueueLength();
		// at this point there are only 2 possibilities: either we have too much packets and we have to delete
		// the first N ones, or we don't have enough, so let's fill the gap with silence.
 		if (time > timestamp - queued_time)
		{
			while (mTheoraAudioPacketQueue != NULL)
			{
				if (time > timestamp - queued_time + mTheoraAudioPacketQueue->num_samples / rate)
				{
					queued_time -= mTheoraAudioPacketQueue->num_samples / rate;
					destroyAudioPacket(popAudioPacket());
				}
				else
				{
					int n_trim = (int) ((timestamp - queued_time + mTheoraAudioPacketQueue->num_samples / rate - time) * rate);
					if (mTheoraAudioPacketQueue->num_samples - n_trim <= 0)
						destroyAudioPacket(popAudioPacket()); // if there's no data to be left, just destroy it
					else
					{
						for (int i = n_trim, j = 0; i < mTheoraAudioPacketQueue->num_samples; i++, j++)
							mTheoraAudioPacketQueue->pcm[j] = mTheoraAudioPacketQueue->pcm[i];
						mTheoraAudioPacketQueue->num_samples -= n_trim;
					}
					break;
				}
			}
		}
		else
		{
			// expand the first packet with silence.
			if (mTheoraAudioPacketQueue) // just in case!
			{
				int i, j, nmissing = (int) ((timestamp - queued_time - time) * rate);
				if (nmissing > 0)
				{
					float* samples = new float[nmissing + mTheoraAudioPacketQueue->num_samples];
					for (i = 0; i < nmissing; i++) samples[i] = 0;
					for (j = 0; i < nmissing + mTheoraAudioPacketQueue->num_samples; i++, j++)
						samples[i] = mTheoraAudioPacketQueue->pcm[j];
					delete [] mTheoraAudioPacketQueue->pcm;
					mTheoraAudioPacketQueue->pcm = samples;
				}
			}
		}

		mAudioMutex->unlock();
	}
	if (!paused) mTimer->play();
	mSeekFrame = -1;
}

void TheoraVideoClip::seek(float time)
{
	seekToFrame((int) (time * getFPS()));
}

void TheoraVideoClip::seekToFrame(int frame)
{
	mSeekFrame = frame;
	mEndOfFile = false;
}

void TheoraVideoClip::waitForCache(float desired_cache_factor, float max_wait_time)
{
	bool paused = mTimer->isPaused();
	if (!paused) mTimer->pause();
	int elapsed = 0;
	int desired_num_precached_frames = (int) (desired_cache_factor * getNumPrecachedFrames());
	while (getNumReadyFrames() < desired_num_precached_frames)
	{
		_psleep(10);
		elapsed += 10;
		if (elapsed >= max_wait_time * 1000) break;
	}
	if (!paused) mTimer->play();
}

float TheoraVideoClip::getPriority()
{
	// TODO
	return getNumPrecachedFrames()*10.0f;
}

float TheoraVideoClip::getPriorityIndex()
{
	float priority=(float) getNumReadyFrames();
	if (mTimer->isPaused()) priority+=getNumPrecachedFrames()/2;
	return priority;
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
