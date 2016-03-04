/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _USE_THEORA
#include <algorithm>
#include <memory.h>
#include <string>

#include "AudioInterface.h"
#include "PixelTransform.h"

#include "DataSource.h"
#include "Exception.h"
#include "FrameQueue.h"
#include "Manager.h"
#include "Mutex.h"
#include "Timer.h"
#include "theoraplayer.h"
#include "Utility.h"
#include "VideoClip_Theora.h"
#include "VideoFrame.h"

#define BUFFER_SIZE 4096

namespace theoraplayer
{
	VideoClip_Theora::VideoClip_Theora(DataSource* data_source, TheoraOutputMode output_mode, int nPrecachedFrames, bool usePower2Stride) :
		VideoClip(data_source, output_mode, nPrecachedFrames, usePower2Stride),
		AudioPacketQueue()
	{
		this->info.TheoraDecoder = NULL;
		this->info.TheoraSetup = NULL;
		this->vorbisStreams = 0;
		this->theoraStreams = 0;
		this->readAudioSamples = 0;
		this->lastDecodedFrameNumber = 0;
	}

	VideoClip_Theora::~VideoClip_Theora()
	{
		if (this->info.TheoraDecoder != NULL)
		{
			th_decode_free(this->info.TheoraDecoder);
			th_setup_free(this->info.TheoraSetup);
			if (this->audioInterface != NULL)
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

	bool VideoClip_Theora::_readData()
	{
		int audioEos = 0;
		int serno = 0;
		float audioTime = 0;
		float time = this->timer->getTime();
		if (this->restarted)
		{
			time = 0;
		}
		char* buffer = NULL;
		int bytesRead = 0;
		ogg_int64_t g = 0;
		do
		{
			buffer = ogg_sync_buffer(&this->info.OggSyncState, BUFFER_SIZE);
			bytesRead = this->stream->read(buffer, BUFFER_SIZE);
			ogg_sync_wrote(&this->info.OggSyncState, bytesRead);
			if (bytesRead == 0)
			{
				if (!this->autoRestart)
				{
					this->endOfFile = true;
					log(this->name + " finished playing");
				}
				return false;
			}
			// when we fill the stream with enough pages, it'll start spitting out packets
			// which contain keyframes, delta frames or audio data
			while (ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage) > 0)
			{
				serno = ogg_page_serialno(&this->info.OggPage);
				if (serno == this->info.TheoraStreamState.serialno)
				{
					ogg_stream_pagein(&this->info.TheoraStreamState, &this->info.OggPage);
				}
				if (this->audioInterface && serno == this->info.VorbisStreamState.serialno)
				{
					g = ogg_page_granulepos(&this->info.OggPage);
					audioTime = (float)vorbis_granule_time(&this->info.VorbisDSPState, g);
					audioEos = ogg_page_eos(&this->info.OggPage);
					ogg_stream_pagein(&this->info.VorbisStreamState, &this->info.OggPage);
				}
			}
		} while (!(this->audioInterface != NULL && audioEos != 0 && audioTime < time + 1.0f));
		return true;
	}

	bool VideoClip_Theora::decodeNextFrame()
	{
		if (this->endOfFile)
		{
			return false;
		}
		VideoFrame* frame = this->frameQueue->requestEmptyFrame();
		if (frame == NULL)
		{
			return false; // max number of precached frames reached
		}
		bool shouldRestart = false;
		ogg_packet opTheora;
		ogg_int64_t granulePos;
		th_ycbcr_buffer buff;
		int result = 0;
		int attempts = 0;
		while (true)
		{
			// ogg_stream_packetout can return -1 and the official docs suggest to do subsequent calls until it succeeds
			// because the data is out of sync. still will limit the number of attempts just in case
			for (result = -1, attempts = 0; result < 0 && attempts < 100; ++attempts)
			{
				result = ogg_stream_packetout(&this->info.TheoraStreamState, &opTheora);
			}
			if (result > 0)
			{
				int status = th_decode_packetin(this->info.TheoraDecoder, &opTheora, &granulePos);
				if (status != 0 && status != TH_DUPFRAME)
				{
					continue; // 0 means success
				}
				float time = (float)th_granule_time(this->info.TheoraDecoder, granulePos);
				unsigned long frameNumber = (unsigned long)th_granule_frame(this->info.TheoraDecoder, granulePos);
				// %16 operation is here to prevent a playback halt during video playback if the decoder can't keep up with demand.
				if (time < this->timer->getTime() && !this->restarted && frameNumber % 16 != 0)
				{
#ifdef _DEBUG_FRAMEDROP
					log(mName + ": pre-dropped frame " + str((int)frameNumber));
#endif
					++this->droppedFramesCount;
					continue; // drop frame
				}
				frame->timeToDisplay = time - this->frameDuration;
				frame->iteration = this->iteration;
				frame->_setFrameNumber(frameNumber);
				this->lastDecodedFrameNumber = frameNumber;
				th_decode_ycbcr_out(this->info.TheoraDecoder, buff);
				PixelTransform pixelTransform;
				memset(&pixelTransform, 0, sizeof(PixelTransform));
				pixelTransform.y = buff[0].data; pixelTransform.yStride = buff[0].stride;
				pixelTransform.u = buff[1].data; pixelTransform.uStride = buff[1].stride;
				pixelTransform.v = buff[2].data; pixelTransform.vStride = buff[2].stride;
				frame->decode(&pixelTransform);
				break;
			}
			if (!this->_readData())
			{
				frame->inUse = false;
				shouldRestart = this->autoRestart;
				break;
			}
		}
		if (this->audioInterface != NULL)
		{
			Mutex::ScopeLock lock(this->audioMutex);
			this->decodeAudio();
		}
		if (shouldRestart)
		{
			++this->iteration;
			this->_restart();
		}
		return true;
	}

	void VideoClip_Theora::_restart()
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
		if (this->audioInterface != NULL)
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
				{
					vorbis_synthesis_blockin(&this->info.VorbisDSPState, &this->info.VorbisBlock);
				}
			}
			ogg_stream_reset(&this->info.VorbisStreamState);
		}
		ogg_sync_reset(&this->info.OggSyncState);
		this->stream->seek(0);
		ogg_int64_t granulePos = 0;
		th_decode_ctl(this->info.TheoraDecoder, TH_DECCTL_SET_GRANPOS, &granulePos, sizeof(granulePos));
		this->endOfFile = false;
		this->restarted = true;
		if (!paused)
		{
			this->timer->play();
		}
	}

	void VideoClip_Theora::_load(DataSource* source)
	{
#ifdef _DEBUG
		log("-----");
#endif
		this->stream = source;
		this->readTheoraVorbisHeaders();
		this->info.TheoraDecoder = th_decode_alloc(&this->info.TheoraInfo, this->info.TheoraSetup);
		this->width = this->info.TheoraInfo.frame_width;
		this->height = this->info.TheoraInfo.frame_height;
		this->subFrameWidth = this->info.TheoraInfo.pic_width;
		this->subFrameHeight = this->info.TheoraInfo.pic_height;
		this->subFrameOffsetX = this->info.TheoraInfo.pic_x;
		this->subFrameOffsetY = this->info.TheoraInfo.pic_y;
		this->stride = (this->stride == 1) ? potCeil(this->getWidth()) : this->getWidth();
		this->fps = this->info.TheoraInfo.fps_numerator / (float) this->info.TheoraInfo.fps_denominator;
#ifdef _DEBUG
		log("width: " + str(this->width) + ", height: " + str(this->height) + ", fps: " + str((int)this->getFps()));
#endif
		this->frameQueue = new FrameQueue(this);
		this->frameQueue->setSize(this->precachedFramesCount);
		// find out the duration of the file by seeking to the end
		// having ogg decode pages, extract the granule pos from
		// the last theora page and seek back to beginning of the file
		char* buffer = NULL;
		int bytesRead = 0;
		uint64_t streamSize = this->stream->getSize();
		uint64_t seekPos = 0;
		int result = 0;
		ogg_int64_t granule = 0;
		for (unsigned int i = 1; i <= 50; ++i)
		{
			ogg_sync_reset(&this->info.OggSyncState);
			seekPos = (BUFFER_SIZE * i > streamSize ? 0 : streamSize - BUFFER_SIZE * i);
			this->stream->seek(seekPos);
			buffer = ogg_sync_buffer(&this->info.OggSyncState, BUFFER_SIZE * i);
			bytesRead = this->stream->read(buffer, BUFFER_SIZE * i);
			ogg_sync_wrote(&this->info.OggSyncState, bytesRead);
			ogg_sync_pageseek(&this->info.OggSyncState, &this->info.OggPage);
			while (true)
			{
				result = ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage);
				if (result == 0)
				{
					break;
				}
				// if page is not a theora page or page is unsynced(-1), skip it
				if (result == -1 || ogg_page_serialno(&this->info.OggPage) != this->info.TheoraStreamState.serialno)
				{
					continue;
				}
				granule = ogg_page_granulepos(&this->info.OggPage);
				if (granule >= 0)
				{
					this->numFrames = (int)th_granule_frame(this->info.TheoraDecoder, granule) + 1;
				}
				else if (this->numFrames > 0)
				{
					++this->numFrames; // append delta frames at the end to get the exact number
				}
			}
			if (this->numFrames > 0 || streamSize < BUFFER_SIZE * i)
			{
				break;
			}
		}
		if (this->numFrames < 0)
		{
			log("unable to determine file duration!");
		}
		else
		{
			this->duration = this->numFrames / this->fps;
#ifdef _DEBUG
			log("duration: " + strf(this->duration) + " seconds");
#endif
		}
		// restore to beginning of stream.
		ogg_sync_reset(&this->info.OggSyncState);
		this->stream->seek(0);
		if (this->vorbisStreams > 0) // if there is no audio interface factory defined, even though the video
		{
			// clip might have audio, it will be ignored
			vorbis_synthesis_init(&this->info.VorbisDSPState, &this->info.VorbisInfo);
			vorbis_block_init(&this->info.VorbisDSPState, &this->info.VorbisBlock);
			this->numAudioChannels = this->info.VorbisInfo.channels;
			this->audioFrequency = (int) this->info.VorbisInfo.rate;
			// create an audio interface instance if available
			AudioInterfaceFactory* audioInterfaceFactory = theoraplayer::manager->getAudioInterfaceFactory();
			if (audioInterfaceFactory != NULL)
			{
				this->setAudioInterface(audioInterfaceFactory->createInstance(this, this->numAudioChannels, this->audioFrequency));
			}
		}
		this->frameDuration = 1.0f / this->getFps();
#ifdef _DEBUG
		log("-----");
#endif
	}

	void VideoClip_Theora::readTheoraVorbisHeaders()
	{
		ogg_packet tempOggPacket;
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
		// init all structures
		ogg_sync_init(&this->info.OggSyncState);
		th_comment_init(&this->info.TheoraComment);
		th_info_init(&this->info.TheoraInfo);
		vorbis_info_init(&this->info.VorbisInfo);
		vorbis_comment_init(&this->info.VorbisComment);
		// start
		ogg_stream_state oggStateTest;
		bool decodeAudio = (theoraplayer::manager->getAudioInterfaceFactory() != NULL);
		char* buffer = NULL;
		int bytesRead = 0;
		bool done = false;
		while (!done)
		{
			buffer = ogg_sync_buffer(&this->info.OggSyncState, BUFFER_SIZE);
			bytesRead = this->stream->read(buffer, BUFFER_SIZE);
			ogg_sync_wrote(&this->info.OggSyncState, bytesRead);
			if (bytesRead == 0)
			{
				break;
			}
			while (ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage) > 0)
			{
				memset(&oggStateTest, 0, sizeof(oggStateTest));
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
					done = true;
					break;
				}
				ogg_stream_init(&oggStateTest, ogg_page_serialno(&this->info.OggPage));
				ogg_stream_pagein(&oggStateTest, &this->info.OggPage);
				ogg_stream_packetout(&oggStateTest, &tempOggPacket);
				//identify the codec
				if (this->theoraStreams == 0 && th_decode_headerin(&this->info.TheoraInfo, &this->info.TheoraComment, &this->info.TheoraSetup, &tempOggPacket) > 0)
				{
					//This is the Theora Header
					memcpy(&this->info.TheoraStreamState, &oggStateTest, sizeof(oggStateTest));
					this->theoraStreams = 1;
				}
				else if (decodeAudio && !this->vorbisStreams && vorbis_synthesis_headerin(&this->info.VorbisInfo, &this->info.VorbisComment, &tempOggPacket) >= 0)
				{
					//This is vorbis header
					memcpy(&this->info.VorbisStreamState, &oggStateTest, sizeof(oggStateTest));
					this->vorbisStreams = 1;
				}
				else //Hmm. I guess it's not a header we support, so erase it
				{
					ogg_stream_clear(&oggStateTest);
				}
			}
		}
		int result = 0;
		while ((this->theoraStreams && (this->theoraStreams < 3)) || (this->vorbisStreams && (this->vorbisStreams < 3)))
		{
			//Check 2nd'dary headers... Theora First
			while (this->theoraStreams && this->theoraStreams < 3 && (result = ogg_stream_packetout(&this->info.TheoraStreamState, &tempOggPacket)))
			{
				if (result  < 0)
				{
					throw TheoraplayerException("Error parsing Theora stream headers!");
				}
				if (!th_decode_headerin(&this->info.TheoraInfo, &this->info.TheoraComment, &this->info.TheoraSetup, &tempOggPacket))
				{
					throw TheoraplayerException("Invalid theora stream!");
				}
				++this->theoraStreams;
			} //end while looking for more theora headers
			//look 2nd vorbis header packets
			while (this->vorbisStreams < 3 && (result = ogg_stream_packetout(&this->info.VorbisStreamState, &tempOggPacket)))
			{
				if (result  < 0)
				{
					throw TheoraplayerException("Error parsing vorbis stream headers!");
				}
				if (vorbis_synthesis_headerin(&this->info.VorbisInfo, &this->info.VorbisComment, &tempOggPacket))
				{
					throw TheoraplayerException("Invalid stream!");
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
				buffer = ogg_sync_buffer(&this->info.OggSyncState, BUFFER_SIZE);
				bytesRead = this->stream->read(buffer, BUFFER_SIZE);
				ogg_sync_wrote(&this->info.OggSyncState, bytesRead);
				if (bytesRead == 0)
				{
					throw TheoraplayerException("End of file found prematurely!");
				}
			}
		} //end while looking for all headers
		//	log("Vorbis Headers: " + str(mVorbisHeaders) + " Theora Headers : " + str(mTheoraHeaders));
	}

	void VideoClip_Theora::decodedAudioCheck()
	{
		if (this->audioInterface != NULL && !this->timer->isPaused())
		{
			Mutex::ScopeLock lock(this->audioMutex);
			this->flushAudioPackets(this->audioInterface);
		}
	}

	float VideoClip_Theora::decodeAudio()
	{
		if (this->restarted)
		{
			return -1.0f;
		}
		ogg_packet opVorbis;
		float** pcm;
		int length = 0;
		float timestamp = -1.0f;
		bool readPastTimestamp = false;
		float factor = 1.0f / this->audioFrequency;
		float videoTime = (float)this->lastDecodedFrameNumber / this->fps;
		float min = this->frameQueue->getSize() / this->fps + 1.0f;
		float audioTime = 0.0f;
		while (true)
		{
			length = vorbis_synthesis_pcmout(&this->info.VorbisDSPState, &pcm);
			if (length == 0)
			{
				if (ogg_stream_packetout(&this->info.VorbisStreamState, &opVorbis) > 0)
				{
					if (vorbis_synthesis(&this->info.VorbisBlock, &opVorbis) == 0)
					{
						if (timestamp < 0 && opVorbis.granulepos >= 0)
						{
							timestamp = (float)vorbis_granule_time(&this->info.VorbisDSPState, opVorbis.granulepos);
						}
						else if (timestamp >= 0)
						{
							readPastTimestamp = true;
						}
						vorbis_synthesis_blockin(&this->info.VorbisDSPState, &this->info.VorbisBlock);
					}
					continue;
				}
				audioTime = this->readAudioSamples * factor;
				// always buffer up of audio ahead of the frames
				if (audioTime - videoTime >= min || !this->_readData())
				{
					break;
				}
			}
			if (length > 0)
			{
				addAudioPacket(pcm, length, this->audioGain);
				this->readAudioSamples += length;
				if (readPastTimestamp)
				{
					timestamp += (float)length / this->info.VorbisInfo.rate;
				}
				vorbis_synthesis_read(&this->info.VorbisDSPState, length); // tell vorbis we read a number of samples
			}
		}
		return timestamp;
	}

	long VideoClip_Theora::seekPage(long targetFrame, bool returnKeyframe)
	{
		int i;
		uint64_t seekMin = 0;
		uint64_t seekMax = this->stream->getSize();
		long frame;
		ogg_int64_t granule = 0;
		if (targetFrame == 0)
		{
			this->stream->seek(0);
		}
		int serno = 0;
		char* buffer = NULL;
		int bytesRead = 0;
		for (i = (targetFrame == 0) ? 100 : 0; i < 100; ++i)
		{
			ogg_sync_reset(&this->info.OggSyncState);
			this->stream->seek(seekMin / 2 + seekMax / 2); // do a binary search
			memset(&this->info.OggPage, 0, sizeof(ogg_page));
			ogg_sync_pageseek(&this->info.OggSyncState, &this->info.OggPage);
			while (i < 1000)
			{
				if (ogg_sync_pageout(&this->info.OggSyncState, &this->info.OggPage) == 1)
				{
					serno = ogg_page_serialno(&this->info.OggPage);
					if (serno == this->info.TheoraStreamState.serialno)
					{
						granule = ogg_page_granulepos(&this->info.OggPage);
						if (granule >= 0)
						{
							frame = (long)th_granule_frame(this->info.TheoraDecoder, granule);
							if (frame < targetFrame && targetFrame - frame < 10)
							{
								// we're close enough, let's break this.
								i = 1000;
								break;
							}
							// we're not close enough, let's shorten the borders of the binary search
							if (targetFrame - 1 > frame)
							{
								seekMin = seekMin / 2 + seekMax / 2;
							}
							else
							{
								seekMax = seekMin / 2 + seekMax / 2;
							}
							break;
						}
					}
				}
				else
				{
					buffer = ogg_sync_buffer(&this->info.OggSyncState, BUFFER_SIZE);
					bytesRead = this->stream->read(buffer, BUFFER_SIZE);
					if (bytesRead == 0)
					{
						break;
					}
					ogg_sync_wrote(&this->info.OggSyncState, bytesRead);
				}
			}
		}
		if (returnKeyframe)
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
		this->stream->seek((seekMin + seekMax) / 2); // do a binary search
		return -1;
	}

	void VideoClip_Theora::doSeek()
	{
#if _DEBUG
		log(this->name + " [seek]: seeking to frame " + str(this->seekFrame));
#endif
		int frame = 0;
		float time = this->seekFrame / getFps();
		this->timer->seek(time);
		bool paused = this->timer->isPaused();
		if (!paused)
		{
			this->timer->pause(); // pause until seeking is done
		}
		this->endOfFile = false;
		this->restarted = false;
		this->resetFrameQueue();
		// reset the video decoder.
		ogg_stream_reset(&this->info.TheoraStreamState);
		th_decode_free(this->info.TheoraDecoder);
		this->info.TheoraDecoder = th_decode_alloc(&this->info.TheoraInfo, this->info.TheoraSetup);
		Mutex::ScopeLock audioMutexLock;
		if (this->audioInterface != NULL)
		{
			audioMutexLock.acquire(this->audioMutex);
			ogg_stream_reset(&this->info.VorbisStreamState);
			vorbis_synthesis_restart(&this->info.VorbisDSPState);
			this->destroyAllAudioPackets();
		}
		// first seek to desired frame, then figure out the location of the
		// previous keyframe and seek to it.
		// then by setting the correct time, the decoder will skip N frames untill
		// we get the frame we want.
		frame = (int)seekPage(this->seekFrame, 1); // find the keyframe nearest to the target frame
#ifdef _DEBUG
	//		log(mName + " [seek]: nearest keyframe for frame " + str(mSeekFrame) + " is frame: " + str(frame));
#endif
		this->seekPage(std::max(0, frame - 1), 0);

		ogg_packet opTheora;
		ogg_int64_t granulePos;
		bool granuleSet = false;
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
			granuleSet = true;
		}

		// now that we've found the keyframe that preceeds our desired frame, lets keep on decoding frames until we
		// reach our target frame.

		int status = 0;
		while (this->seekFrame != 0)
		{
			if (ogg_stream_packetout(&this->info.TheoraStreamState, &opTheora) > 0)
			{
				if (!granuleSet)
				{
					// theora decoder requires to set the granule pos after seek to be able to determine the current frame
					if (opTheora.granulepos < 0)
					{
						continue; // ignore prev delta frames until we hit a keyframe
					}
					th_decode_ctl(this->info.TheoraDecoder, TH_DECCTL_SET_GRANPOS, &opTheora.granulepos, sizeof(opTheora.granulepos));
					granuleSet = 1;
				}
				status = th_decode_packetin(this->info.TheoraDecoder, &opTheora, &granulePos);
				if (status != 0 && status != TH_DUPFRAME)
				{
					continue;
				}
				frame = (int)th_granule_frame(this->info.TheoraDecoder, granulePos);
				if (frame >= this->seekFrame - 1)
				{
					break;
				}
			}
			else if (!_readData())
			{
				log(this->name + " [seek]: fineseeking failed, _readData failed!");
				audioMutexLock.release();
				return;
			}
		}
#ifdef _DEBUG
		//	log(mName + " [seek]: fineseeked to frame " + str(frame + 1) + ", requested: " + str(mSeekFrame));
#endif
		if (this->audioInterface != NULL)
		{
			// read audio data until we reach a timestamp. this usually takes only one iteration, but just in case let's
			// wrap it in a loop
			float timestamp = 0.0f;
			while (true)
			{
				timestamp = this->decodeAudio();
				if (timestamp >= 0)
				{
					break;
				}
				this->_readData();
			}
			float rate = (float)this->audioFrequency * this->numAudioChannels;
			float queuedTime = this->getAudioPacketQueueLength();
			int trimmedCount = 0;
			// at this point there are only 2 possibilities: either we have too much packets and we have to delete
			// the first N ones, or we don't have enough, so let's fill the gap with silence.
			if (time > timestamp - queuedTime)
			{
				while (this->theoraAudioPacketQueue != NULL)
				{
					if (time > timestamp - queuedTime + this->theoraAudioPacketQueue->numSamples / rate)
					{
						queuedTime -= this->theoraAudioPacketQueue->numSamples / rate;
						destroyAudioPacket(popAudioPacket());
					}
					else
					{
						trimmedCount = (int)((timestamp - queuedTime + this->theoraAudioPacketQueue->numSamples / rate - time) * rate);
						if (this->theoraAudioPacketQueue->numSamples - trimmedCount <= 0)
						{
							destroyAudioPacket(popAudioPacket()); // if there's no data to be left, just destroy it
						}
						else
						{
							for (int i = trimmedCount, j = 0; i < this->theoraAudioPacketQueue->numSamples; ++i, ++j)
							{
								this->theoraAudioPacketQueue->pcm[j] = this->theoraAudioPacketQueue->pcm[i];
							}
							this->theoraAudioPacketQueue->numSamples -= trimmedCount;
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
					int i = 0;
					int j = 0;
					int missingCount = (int)((timestamp - queuedTime - time) * rate);
					if (missingCount > 0)
					{
						float* samples = new float[missingCount + this->theoraAudioPacketQueue->numSamples];
						for (i = 0; i < missingCount; ++i)
						{
							samples[i] = 0;
						}
						for (j = 0; i < missingCount + this->theoraAudioPacketQueue->numSamples; ++i, ++j)
						{
							samples[i] = this->theoraAudioPacketQueue->pcm[j];
						}
						delete[] this->theoraAudioPacketQueue->pcm;
						this->theoraAudioPacketQueue->pcm = samples;
					}
				}
			}
			this->lastDecodedFrameNumber = this->seekFrame;
			this->readAudioSamples = (unsigned int)(timestamp * this->audioFrequency);
			audioMutexLock.release();
		}
		if (!paused)
		{
			this->timer->play();
		}
		this->seekFrame = -1;
	}

}
#endif
