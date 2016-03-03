/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#ifdef __THEORA
#include <memory.h>
#include <algorithm>
#include "TheoraVideoManager.h"
#include "TheoraFrameQueue.h"
#include "TheoraVideoFrame.h"
#include "TheoraAudioInterface.h"
#include "TheoraTimer.h"
#include "TheoraDataSource.h"
#include "TheoraUtil.h"
#include "TheoraException.h"
#include "TheoraVideoClip_Theora.h"
#include "TheoraPixelTransform.h"

#include "Mutex.h"

TheoraVideoClip_Theora::TheoraVideoClip_Theora(TheoraDataSource* data_source,
										TheoraOutputMode output_mode,
										int nPrecachedFrames,
										bool usePower2Stride):
	TheoraVideoClip(data_source, output_mode, nPrecachedFrames, usePower2Stride),
	TheoraAudioPacketQueue()
{
	this->info.TheoraDecoder = NULL;
	this->info.TheoraSetup = NULL;
	this->vorbisStreams = this->theoraStreams = 0;
	this->readAudioSamples = 0;
	this->lastDecodedFrameNumber = 0;
}

TheoraVideoClip_Theora::~TheoraVideoClip_Theora()
{
	if (this->info.TheoraDecoder)
	{
		th_decode_free(this->info.TheoraDecoder);
		th_setup_free(this->info.TheoraSetup);

		if (this->audioInterface)
		{
			vorbis_dsp_clear(&this->info.VorbisDSPState);
			vorbis_block_clear(&this->info.VorbisBlock);
		}

		ogg_stream_clear(&this->info.TheoraStreamState);
		th_comment_clear(&this->info.TheoraComment);
		th_info_clear(&this->info.TheoraInfo);
		
		ogg_stream_clear(&this->info.VorbisStreamState);
		vorbis_comment_clear(&this->info.VorbisComment);
		vorbis_info_clear(&this->info.VorbisInfo);
		
		ogg_sync_clear(&this->info.OggSyncState);
	}
}

bool TheoraVideoClip_Theora::_readData()
{
	int audio_eos = 0, serno;
	float audio_time = 0;
	float time = this->timer->getTime();
	if (this->restarted)
	{
		time = 0;
	}
	
	for (;;)
	{
		char *buffer = ogg_sync_buffer(&this->info.OggSyncState, 4096);
		int bytes_read = this->stream->read(buffer, 4096);
		ogg_sync_wrote(&this->info.OggSyncState, bytes_read);
		
		if (bytes_read < 4096)
		{
			if (bytes_read == 0)
			{
				if (!this->autoRestart)
				{
					this->endOfFile = true;
					th_writelog(this->name + " finished playing");
				}
				return 0;
			}
		}
		// when we fill the stream with enough pages, it'll start spitting out packets
		// which contain keyframes, delta frames or audio data
		while (ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage) > 0)
		{
			serno = ogg_page_serialno(&this->info.OggPage);
			if (serno == this->info.TheoraStreamState.serialno) ogg_stream_pagein(&this->info.TheoraStreamState, &this->info.OggPage);
			if (this->audioInterface && serno == this->info.VorbisStreamState.serialno)
			{
				ogg_int64_t g = ogg_page_granulepos(&this->info.OggPage);
				audio_time = (float)vorbis_granule_time(&this->info.VorbisDSPState, g);
				audio_eos = ogg_page_eos(&this->info.OggPage);
				ogg_stream_pagein(&this->info.VorbisStreamState, &this->info.OggPage);
			}
		}
		if (!(this->audioInterface && !audio_eos && audio_time < time + 1.0f))
		{
			break;
		}
	}
	return 1;
}

bool TheoraVideoClip_Theora::decodeNextFrame()
{
	if (this->endOfFile)
	{
		return 0;
	}
	
	TheoraVideoFrame* frame = this->frameQueue->requestEmptyFrame();
	if (!frame) return 0; // max number of precached frames reached
	bool should_restart = 0;
	ogg_packet opTheora;
	ogg_int64_t granulePos;
	th_ycbcr_buffer buff;
	int ret, nAttempts;
	for (;;)
	{
		// ogg_stream_packetout can return -1 and the official docs suggest to do subsequent calls until it succeeds
		// because the data is out of sync. still will limit the number of attempts just in case
		for (ret = -1, nAttempts = 0; ret < 0 && nAttempts < 100; nAttempts++)
		{
			ret = ogg_stream_packetout(&this->info.TheoraStreamState, &opTheora);
		}
		
		if (ret > 0)
		{
			int status = th_decode_packetin(this->info.TheoraDecoder, &opTheora, &granulePos);
			if (status != 0 && status != TH_DUPFRAME) continue; // 0 means success

			float time = (float)th_granule_time(this->info.TheoraDecoder, granulePos);
			unsigned long frame_number = (unsigned long)th_granule_frame(this->info.TheoraDecoder, granulePos);
			
			if (time < this->timer->getTime() && !this->restarted && frame_number % 16 != 0)
			{
				// %16 operation is here to prevent a playback halt during video playback if the decoder can't keep up with demand.
#ifdef _DEBUG_FRAMEDROP
				th_writelog(mName + ": pre-dropped frame " + str((int) frame_number));
#endif
				++this->numDroppedFrames;
				continue; // drop frame
			}
			frame->timeToDisplay = time - this->frameDuration;
			frame->iteration = this->iteration;
			frame->_setFrameNumber(frame_number);
			this->lastDecodedFrameNumber = frame_number;
			th_decode_ycbcr_out(this->info.TheoraDecoder, buff);
			TheoraPixelTransform t;
			memset(&t, 0, sizeof(TheoraPixelTransform));
			
			t.y = buff[0].data; t.yStride = buff[0].stride;
			t.u = buff[1].data; t.uStride = buff[1].stride;
			t.v = buff[2].data; t.vStride = buff[2].stride;
			frame->decode(&t);
			break;
		}
		else
		{
			if (!_readData())
			{
				frame->inUse = 0;
				should_restart = this->autoRestart;
				break;
			}
		}
	}
	
	if (this->audioInterface != NULL)
	{
		Mutex::ScopeLock lock(this->audioMutex);
		decodeAudio();
		lock.release();
	}
	if (should_restart)
	{
		++this->iteration;
		_restart();
	}
	return 1;
}

void TheoraVideoClip_Theora::_restart()
{
	bool paused = this->timer->isPaused();
	if (!paused)
	{
		this->timer->pause();
	}
	long initialGranule = 0;
	th_decode_ctl(this->info.TheoraDecoder, TH_DECCTL_SET_GRANPOS, &initialGranule, sizeof(initialGranule));
	th_decode_free(this->info.TheoraDecoder);
	this->info.TheoraDecoder = th_decode_alloc(&this->info.TheoraInfo, this->info.TheoraSetup);
	ogg_stream_reset(&this->info.TheoraStreamState);
	if (this->audioInterface)
	{
		// empty the DSP buffer
		//float **pcm;
		//int len = vorbis_synthesis_pcmout(&this->info.VorbisDSPState,&pcm);
		//if (len) vorbis_synthesis_read(&this->info.VorbisDSPState,len);
		ogg_packet opVorbis;
		this->readAudioSamples = 0;
		while (ogg_stream_packetout(&this->info.VorbisStreamState, &opVorbis) > 0)
		{
			if (vorbis_synthesis(&this->info.VorbisBlock, &opVorbis) == 0)
				vorbis_synthesis_blockin(&this->info.VorbisDSPState, &this->info.VorbisBlock);
		}
		ogg_stream_reset(&this->info.VorbisStreamState);
	}
	
	ogg_sync_reset(&this->info.OggSyncState);
	this->stream->seek(0);
	ogg_int64_t granulePos = 0;
	th_decode_ctl(this->info.TheoraDecoder, TH_DECCTL_SET_GRANPOS, &granulePos, sizeof(granulePos));
	
	this->endOfFile = false;
	
	this->restarted = 1;
	
	if (!paused)
	{
		this->timer->play();
	}
}

void TheoraVideoClip_Theora::load(TheoraDataSource* source)
{
#ifdef _DEBUG
	th_writelog("-----");
#endif
	this->stream = source;
	readTheoraVorbisHeaders();
	
	this->info.TheoraDecoder = th_decode_alloc(&this->info.TheoraInfo,this->info.TheoraSetup);
	
	this->width = this->info.TheoraInfo.frame_width;
	this->height = this->info.TheoraInfo.frame_height;
	this->subFrameWidth = this->info.TheoraInfo.pic_width;
	this->subFrameHeight = this->info.TheoraInfo.pic_height;
	this->subFrameOffsetX = this->info.TheoraInfo.pic_x;
	this->subFrameOffsetY = this->info.TheoraInfo.pic_y;
	this->stride = (this->stride == 1) ? _nextPow2(getWidth()) : getWidth();
	this->fps = this->info.TheoraInfo.fps_numerator / (float) this->info.TheoraInfo.fps_denominator;
	
#ifdef _DEBUG
	th_writelog("width: " + str(this->width) + ", height: " + str(this->height) + ", fps: " + str((int)getFps()));
#endif
	this->frameQueue = new TheoraFrameQueue(this);
	this->frameQueue->setSize(this->numPrecachedFrames);
	// find out the duration of the file by seeking to the end
	// having ogg decode pages, extract the granule pos from
	// the last theora page and seek back to beginning of the file
	uint64_t streamSize = this->stream->getSize(), seekPos;
	for (unsigned int i = 1; i <= 50; ++i)
	{
		ogg_sync_reset(&this->info.OggSyncState);
		if (4096 * i > streamSize)
		{
			seekPos = 0;
		}
		else
		{
			seekPos = streamSize - 4096 * i;
		}
		this->stream->seek(seekPos);
		
		char *buffer = ogg_sync_buffer(&this->info.OggSyncState, 4096 * i);
		int bytes_read = this->stream->read(buffer, 4096 * i);
		ogg_sync_wrote(&this->info.OggSyncState, bytes_read);
		ogg_sync_pageseek(&this->info.OggSyncState, &this->info.OggPage);
		
		for (;;)
		{
			int ret = ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage);
			if (ret == 0)
			{
				break;
			}
			// if page is not a theora page or page is unsynced(-1), skip it
			if (ret == -1 || ogg_page_serialno(&this->info.OggPage) != this->info.TheoraStreamState.serialno)
			{
				continue;
			}
			
			ogg_int64_t granule = ogg_page_granulepos(&this->info.OggPage);
			if (granule >= 0)
			{
				this->numFrames = (int) th_granule_frame(this->info.TheoraDecoder, granule) + 1;
			}
			else if (this->numFrames > 0)
			{
				++this->numFrames; // append delta frames at the end to get the exact number
			}
		}
		if (this->numFrames > 0 || streamSize < 4096 * i)
		{
			break;
		}
		
	}
	if (this->numFrames < 0)
	{
		th_writelog("unable to determine file duration!");
	}
	else
	{
		this->duration = this->numFrames / this->fps;
#ifdef _DEBUG
		th_writelog("duration: " + strf(this->duration) + " seconds");
#endif
	}
	// restore to beginning of stream.
	ogg_sync_reset(&this->info.OggSyncState);
	this->stream->seek(0);
	
	if (this->vorbisStreams) // if there is no audio interface factory defined, even though the video
		// clip might have audio, it will be ignored
	{
		vorbis_synthesis_init(&this->info.VorbisDSPState, &this->info.VorbisInfo);
		vorbis_block_init(&this->info.VorbisDSPState, &this->info.VorbisBlock);
		this->numAudioChannels = this->info.VorbisInfo.channels;
		this->audioFrequency = (int) this->info.VorbisInfo.rate;

		// create an audio interface instance if available
		TheoraAudioInterfaceFactory* audio_factory = TheoraVideoManager::getSingleton().getAudioInterfaceFactory();
		if (audio_factory)
		{
			setAudioInterface(audio_factory->createInstance(this, this->numAudioChannels, this->audioFrequency));
		}
	}
	
	this->frameDuration = 1.0f / getFps();
#ifdef _DEBUG
	th_writelog("-----");
#endif
}

void TheoraVideoClip_Theora::readTheoraVorbisHeaders()
{
	ogg_packet tempOggPacket;
	bool done = false;
	bool decode_audio = TheoraVideoManager::getSingleton().getAudioInterfaceFactory() != NULL;
	//init Vorbis/Theora Layer
	//Ensure all structures get cleared out.
	memset(&this->info.OggSyncState, 0, sizeof(ogg_sync_state));
	memset(&this->info.OggPage, 0, sizeof(ogg_page));
	memset(&this->info.VorbisStreamState, 0, sizeof(ogg_stream_state));
	memset(&this->info.TheoraStreamState, 0, sizeof(ogg_stream_state));
	memset(&this->info.TheoraInfo, 0, sizeof(th_info));
	memset(&this->info.TheoraComment, 0, sizeof(th_comment));
	memset(&this->info.VorbisInfo, 0, sizeof(vorbis_info));
	memset(&this->info.VorbisDSPState, 0, sizeof(vorbis_dsp_state));
	memset(&this->info.VorbisBlock, 0, sizeof(vorbis_block));
	memset(&this->info.VorbisComment, 0, sizeof(vorbis_comment));
	
	ogg_sync_init(&this->info.OggSyncState);
	th_comment_init(&this->info.TheoraComment);
	th_info_init(&this->info.TheoraInfo);
	vorbis_info_init(&this->info.VorbisInfo);
	vorbis_comment_init(&this->info.VorbisComment);
	
	while (!done)
	{
		char *buffer = ogg_sync_buffer(&this->info.OggSyncState, 4096);
		int bytes_read = this->stream->read(buffer, 4096);
		ogg_sync_wrote(&this->info.OggSyncState, bytes_read);
		
		if (bytes_read == 0)
			break;
		
		while (ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage) > 0)
		{
			ogg_stream_state OggStateTest;
			
			//is this an initial header? If not, stop
			if (!ogg_page_bos(&this->info.OggPage))
			{
				//This is done blindly, because stream only accept themselves
				if (this->theoraStreams)
				{
					ogg_stream_pagein(&this->info.TheoraStreamState, &this->info.OggPage);
				}
				if (this->vorbisStreams)
				{
					ogg_stream_pagein(&this->info.VorbisStreamState, &this->info.OggPage);
				}
				
				done=true;
				break;
			}
			
			ogg_stream_init(&OggStateTest, ogg_page_serialno(&this->info.OggPage));
			ogg_stream_pagein(&OggStateTest, &this->info.OggPage);
			ogg_stream_packetout(&OggStateTest, &tempOggPacket);
			
			//identify the codec
			int ret;
			if (!this->theoraStreams)
			{
				ret = th_decode_headerin(&this->info.TheoraInfo, &this->info.TheoraComment, &this->info.TheoraSetup, &tempOggPacket);
				
				if (ret > 0)
				{
					//This is the Theora Header
					memcpy(&this->info.TheoraStreamState, &OggStateTest, sizeof(OggStateTest));
					this->theoraStreams = 1;
					continue;
				}
			}
			if (decode_audio && !this->vorbisStreams &&
				vorbis_synthesis_headerin(&this->info.VorbisInfo, &this->info.VorbisComment, &tempOggPacket) >=0)
			{
				//This is vorbis header
				memcpy(&this->info.VorbisStreamState, &OggStateTest, sizeof(OggStateTest));
				this->vorbisStreams = 1;
				continue;
			}
			//Hmm. I guess it's not a header we support, so erase it
			ogg_stream_clear(&OggStateTest);
		}
	}
	
	while ((this->theoraStreams && (this->theoraStreams < 3)) ||
		(this->vorbisStreams && (this->vorbisStreams < 3)))
	{
		//Check 2nd'dary headers... Theora First
		int iSuccess;
		while (this->theoraStreams && this->theoraStreams < 3 &&
			  (iSuccess = ogg_stream_packetout(&this->info.TheoraStreamState, &tempOggPacket)))
		{
			if (iSuccess < 0)
			{
				throw TheoraGenericException("Error parsing Theora stream headers.");
			}
			if (!th_decode_headerin(&this->info.TheoraInfo, &this->info.TheoraComment, &this->info.TheoraSetup, &tempOggPacket))
			{
				throw TheoraGenericException("invalid theora stream");
			}
			
			++this->theoraStreams;
		} //end while looking for more theora headers
		
		//look 2nd vorbis header packets
		while (this->vorbisStreams < 3 && (iSuccess = ogg_stream_packetout(&this->info.VorbisStreamState, &tempOggPacket)))
		{
			if (iSuccess < 0)
			{
				throw TheoraGenericException("Error parsing vorbis stream headers");
			}
			
			if (vorbis_synthesis_headerin(&this->info.VorbisInfo, &this->info.VorbisComment,&tempOggPacket))
			{
				throw TheoraGenericException("invalid stream");
			}
			
			++this->vorbisStreams;
		} //end while looking for more vorbis headers
		
		//Not finished with Headers, get some more file data
		if (ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage) > 0)
		{
			if (this->theoraStreams)
			{
				ogg_stream_pagein(&this->info.TheoraStreamState, &this->info.OggPage);
			}
			if (this->vorbisStreams)
			{
				ogg_stream_pagein(&this->info.VorbisStreamState, &this->info.OggPage);
			}
		}
		else
		{
			char *buffer = ogg_sync_buffer(&this->info.OggSyncState, 4096);
			int bytes_read = this->stream->read(buffer, 4096);
			ogg_sync_wrote(&this->info.OggSyncState, bytes_read);
			
			if (bytes_read == 0)
			{
				throw TheoraGenericException("End of file found prematurely");
			}
		}
	} //end while looking for all headers
	//	writelog("Vorbis Headers: " + str(mVorbisHeaders) + " Theora Headers : " + str(mTheoraHeaders));
}

void TheoraVideoClip_Theora::decodedAudioCheck()
{
	if (!this->audioInterface || this->timer->isPaused())
	{
		return;
	}

	Mutex::ScopeLock lock(this->audioMutex);
	flushAudioPackets(this->audioInterface);
	lock.release();
}

float TheoraVideoClip_Theora::decodeAudio()
{
	if (this->restarted) return -1;
	
	ogg_packet opVorbis;
	float **pcm;
	int len = 0;
	float timestamp = -1;
	bool read_past_timestamp = 0;
	
	float factor = 1.0f / (this->audioFrequency);
	float videoTime = (float) this->lastDecodedFrameNumber / this->fps;
	float min = this->frameQueue->getSize() / this->fps + 1.0f;

	for (;;)
	{
		len = vorbis_synthesis_pcmout(&this->info.VorbisDSPState, &pcm);
		if (len == 0)
		{
			if (ogg_stream_packetout(&this->info.VorbisStreamState, &opVorbis) > 0)
			{
				if (vorbis_synthesis(&this->info.VorbisBlock, &opVorbis) == 0)
				{
					if (timestamp < 0 && opVorbis.granulepos >= 0)
					{
						timestamp = (float) vorbis_granule_time(&this->info.VorbisDSPState, opVorbis.granulepos);
					}
					else if (timestamp >= 0)
					{
						read_past_timestamp = 1;
					}
					vorbis_synthesis_blockin(&this->info.VorbisDSPState, &this->info.VorbisBlock);
				}
				continue;
			}
			else
			{
				float audioTime = this->readAudioSamples * factor;
				// always buffer up of audio ahead of the frames
				if (audioTime - videoTime < min)
				{
					if (!_readData())
					{
						break;
					}
				}
				else
					break;
			}
		}
		if (len > 0)
		{
			addAudioPacket(pcm, len, this->audioGain);
			this->readAudioSamples += len;
			if (read_past_timestamp)
			{
				timestamp += (float)len / this->info.VorbisInfo.rate;
			}
			vorbis_synthesis_read(&this->info.VorbisDSPState, len); // tell vorbis we read a number of samples
		}
	}
	return timestamp;
}

long TheoraVideoClip_Theora::seekPage(long targetFrame, bool return_keyframe)
{
	int i;
	uint64_t seek_min = 0, seek_max = this->stream->getSize();
	long frame;
	ogg_int64_t granule = 0;
	
	if (targetFrame == 0) this->stream->seek(0);
	for (i = (targetFrame == 0) ? 100 : 0; i < 100; ++i)
	{
		ogg_sync_reset(&this->info.OggSyncState);
		this->stream->seek(seek_min / 2 + seek_max / 2); // do a binary search
		memset(&this->info.OggPage, 0, sizeof(ogg_page));
		ogg_sync_pageseek(&this->info.OggSyncState, &this->info.OggPage);
		
		for (;i < 1000;)
		{
			int ret = ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage);
			if (ret == 1)
			{
				int serno = ogg_page_serialno(&this->info.OggPage);
				if (serno == this->info.TheoraStreamState.serialno)
				{
					granule = ogg_page_granulepos(&this->info.OggPage);
					if (granule >= 0)
					{
						frame = (long) th_granule_frame(this->info.TheoraDecoder, granule);
						if (frame < targetFrame && targetFrame - frame < 10)
						{
							// we're close enough, let's break this.
							i = 1000;
							break;
						}
						// we're not close enough, let's shorten the borders of the binary search
						if (targetFrame - 1 > frame) seek_min = seek_min / 2 + seek_max / 2;
						else				         seek_max = seek_min / 2 + seek_max / 2;
						break;
					}
				}
			}
			else
			{
				char *buffer = ogg_sync_buffer(&this->info.OggSyncState, 4096);
				int bytes_read = this->stream->read(buffer, 4096);
				if (bytes_read == 0) break;
				ogg_sync_wrote(&this->info.OggSyncState, bytes_read);
			}
		}
	}
	if (return_keyframe)
	{
		return (long)(granule >> this->info.TheoraInfo.keyframe_granule_shift);
	}
	
	ogg_sync_reset(&this->info.OggSyncState);
	memset(&this->info.OggPage, 0, sizeof(ogg_page));
	ogg_sync_pageseek(&this->info.OggSyncState, &this->info.OggPage);
	if (targetFrame == 0)
	{
		return -1;
	}
	this->stream->seek((seek_min + seek_max) / 2); // do a binary search
	return -1;
}

void TheoraVideoClip_Theora::doSeek()
{
#if _DEBUG
	th_writelog(this->name + " [seek]: seeking to frame " + str(this->seekFrame));
#endif
	int frame;
	float time = this->seekFrame / getFps();
	this->timer->seek(time);
	bool paused = this->timer->isPaused();
	if (!paused) this->timer->pause(); // pause until seeking is done
	
	this->endOfFile = false;
	this->restarted = false;
	
	resetFrameQueue();
	// reset the video decoder.
	ogg_stream_reset(&this->info.TheoraStreamState);
	th_decode_free(this->info.TheoraDecoder);
	this->info.TheoraDecoder = th_decode_alloc(&this->info.TheoraInfo, this->info.TheoraSetup);

	Mutex::ScopeLock audioMutexLock;
	if (this->audioInterface)
	{
		audioMutexLock.acquire(this->audioMutex);
		ogg_stream_reset(&this->info.VorbisStreamState);
		vorbis_synthesis_restart(&this->info.VorbisDSPState);
		destroyAllAudioPackets();
	}
	// first seek to desired frame, then figure out the location of the
	// previous keyframe and seek to it.
	// then by setting the correct time, the decoder will skip N frames untill
	// we get the frame we want.
	frame = (int)seekPage(this->seekFrame, 1); // find the keyframe nearest to the target frame
#ifdef _DEBUG
	//		th_writelog(mName + " [seek]: nearest keyframe for frame " + str(mSeekFrame) + " is frame: " + str(frame));
#endif
	seekPage(std::max(0, frame - 1), 0);
	
	ogg_packet opTheora;
	ogg_int64_t granulePos;
	bool granule_set = 0;
	if (frame <= 1)
	{
		if (this->info.TheoraInfo.version_major == 3 && this->info.TheoraInfo.version_minor == 2 && this->info.TheoraInfo.version_subminor == 0)
		{
			granulePos = 0;
		}
		else
		{
			granulePos = 1; // because of difference in granule interpretation in theora streams 3.2.0 and newer ones
		}
		th_decode_ctl(this->info.TheoraDecoder, TH_DECCTL_SET_GRANPOS, &granulePos, sizeof(granulePos));
		granule_set = 1;
	}
	
	// now that we've found the keyframe that preceeds our desired frame, lets keep on decoding frames until we
	// reach our target frame.
	
	int status, ret;
	for (; this->seekFrame != 0;)
	{
		ret = ogg_stream_packetout(&this->info.TheoraStreamState, &opTheora);
		if (ret > 0)
		{
			if (!granule_set)
			{
				// theora decoder requires to set the granule pos after seek to be able to determine the current frame
				if (opTheora.granulepos >= 0)
				{
					th_decode_ctl(this->info.TheoraDecoder, TH_DECCTL_SET_GRANPOS, &opTheora.granulepos, sizeof(opTheora.granulepos));
					granule_set = 1;
				}
				else
				{
					continue; // ignore prev delta frames until we hit a keyframe
				}
			}
			status = th_decode_packetin(this->info.TheoraDecoder, &opTheora, &granulePos);
			if (status != 0 && status != TH_DUPFRAME)
			{
				continue;
			}
			frame = (int) th_granule_frame(this->info.TheoraDecoder, granulePos);
			if (frame >= this->seekFrame - 1)
			{
				break;
			}
		}
		else
		{
			if (!_readData())
			{
				th_writelog(this->name + " [seek]: fineseeking failed, _readData failed!");
				audioMutexLock.release();
				return;
			}
		}
	}
#ifdef _DEBUG
	//	th_writelog(mName + " [seek]: fineseeked to frame " + str(frame + 1) + ", requested: " + str(mSeekFrame));
#endif
	if (this->audioInterface)
	{
		// read audio data until we reach a timestamp. this usually takes only one iteration, but just in case let's
		// wrap it in a loop
		float timestamp;
		for (;;)
		{
			timestamp = decodeAudio();
			if (timestamp >= 0)
			{
				break;
			}
			else
			{
				_readData();
			}
		}
		float rate = (float) this->audioFrequency * this->numAudioChannels;
		float queued_time = getAudioPacketQueueLength();
		// at this point there are only 2 possibilities: either we have too much packets and we have to delete
		// the first N ones, or we don't have enough, so let's fill the gap with silence.
 		if (time > timestamp - queued_time)
		{
			while (this->theoraAudioPacketQueue != NULL)
			{
				if (time > timestamp - queued_time + this->theoraAudioPacketQueue->numSamples / rate)
				{
					queued_time -= this->theoraAudioPacketQueue->numSamples / rate;
					destroyAudioPacket(popAudioPacket());
				}
				else
				{
					int n_trim = (int) ((timestamp - queued_time + this->theoraAudioPacketQueue->numSamples / rate - time) * rate);
					if (this->theoraAudioPacketQueue->numSamples - n_trim <= 0)
					{
						destroyAudioPacket(popAudioPacket()); // if there's no data to be left, just destroy it
					}
					else
					{
						for (int i = n_trim, j = 0; i < this->theoraAudioPacketQueue->numSamples; ++i, ++j)
							this->theoraAudioPacketQueue->pcm[j] = this->theoraAudioPacketQueue->pcm[i];
						this->theoraAudioPacketQueue->numSamples -= n_trim;
					}
					break;
				}
			}
		}
		else
		{
			// expand the first packet with silence.
			if (this->theoraAudioPacketQueue) // just in case!
			{
				int i, j, nmissing = (int) ((timestamp - queued_time - time) * rate);
				if (nmissing > 0)
				{
					float* samples = new float[nmissing + this->theoraAudioPacketQueue->numSamples];
					for (i = 0; i < nmissing; ++i) samples[i] = 0;
					for (j = 0; i < nmissing + this->theoraAudioPacketQueue->numSamples; ++i, ++j)
						samples[i] = this->theoraAudioPacketQueue->pcm[j];
					delete[] this->theoraAudioPacketQueue->pcm;
					this->theoraAudioPacketQueue->pcm = samples;
				}
			}
		}
		this->lastDecodedFrameNumber = this->seekFrame;
		this->readAudioSamples = (unsigned int) (timestamp * this->audioFrequency);
		
		audioMutexLock.release();
	}
	if (!paused)
	{
		this->timer->play();
	}
	this->seekFrame = -1;
}
#endif
