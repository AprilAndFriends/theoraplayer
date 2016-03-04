/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <algorithm>
#include <memory.h>
#include <string>

#include "Manager.h"
#include "AudioInterface.h"
#include "PixelTransform.h"

//#include <stdio.h>
//#include <stdlib.h>

#include "DataSource.h"
#include "Exception.h"
#include "FrameQueue.h"
#include "Mutex.h"
#include "Timer.h"
#include "theoraplayer.h"
#include "Utility.h"
#include "VideoClip_WebM.h"
#include "VideoFrame.h"

// TODOth - rename
#include "WebM/webmdec.h"

namespace theoraplayer
{
	VideoClip_WebM::VideoClip_WebM(DataSource* dataSource,
		TheoraOutputMode output_mode,
		int nPrecachedFrames,
		bool usePower2Stride) :
		VideoClip(dataSource, output_mode, nPrecachedFrames, usePower2Stride),
		AudioPacketQueue()
	{
		memset(&(webm_ctx), 0, sizeof(webm_ctx));
		this->input.webm_ctx = &webm_ctx;
		this->input.vpx_input_ctx = &vpx_input_ctx;
		this->seekFrame = 0;
		this->frameNumber = 0;
	}

	VideoClip_WebM::~VideoClip_WebM()
	{
		TheoraWebmDec::webm_free(this->input.webm_ctx);
	}

	bool VideoClip_WebM::_readData()
	{
		return 1;
	}

	bool VideoClip_WebM::decodeNextFrame()
	{
		VideoFrame* frame = this->frameQueue->requestEmptyFrame();
		if (frame == NULL)
		{
			return 0; // max number of precached frames reached
		}
		bool should_restart = 0;

		uint8_t* buf = NULL;
		size_t bytes_in_buffer = 0, buffer_size = 0;

		if (!TheoraWebmDec::webm_read_frame(this->input.webm_ctx, &buf, &bytes_in_buffer, &buffer_size))
		{
			vpx_codec_iter_t  iter = NULL;
			vpx_image_t    *img;

			if (vpx_codec_decode(&decoder, buf, (unsigned int)bytes_in_buffer,
				NULL, 0))
			{
				const char *detail = vpx_codec_error_detail(&decoder);
				if (detail != NULL)
				{
					log("Additional information: " + std::string(detail));
				}
			}
			if ((img = vpx_codec_get_frame(&decoder, &iter)))
			{
				this->frame = img;

				frame->timeToDisplay = this->frameNumber / this->fps;
				frame->iteration = this->iteration;
				frame->_setFrameNumber(this->frameNumber++);
				this->lastDecodedFrameNumber = this->frameNumber;

				if (this->lastDecodedFrameNumber >= (unsigned long)this->numFrames)
					should_restart = true;

				PixelTransform t;
				memset(&t, 0, sizeof(PixelTransform));

				t.y = this->frame->planes[0]; t.yStride = this->frame->stride[0];
				t.u = this->frame->planes[1]; t.uStride = this->frame->stride[1];
				t.v = this->frame->planes[2]; t.vStride = this->frame->stride[2];

				frame->decode(&t);
			}
		}

		return 1;
	}

	void VideoClip_WebM::_restart()
	{
		bool paused = this->timer->isPaused();
		if (!paused) this->timer->pause();

		TheoraWebmDec::webm_rewind(input.webm_ctx);
		this->frameNumber = 0;
		this->lastDecodedFrameNumber = -1;
		this->seekFrame = 0;

		this->endOfFile = false;

		this->restarted = 1;

		if (!paused)
		{
			this->timer->play();
		}
	}

	void VideoClip_WebM::_load(DataSource* source)
	{
		if (!TheoraWebmDec::file_is_webm(source, input.webm_ctx, input.vpx_input_ctx))
		{
			log("Error: File is not webm.");
			return;
		}

		if (TheoraWebmDec::webm_guess_framerate(source, input.webm_ctx, input.vpx_input_ctx))
		{
			log("Error: Unable to guess webm framerate.");
			return;
		}

		this->numFrames = TheoraWebmDec::webm_guess_duration(input.webm_ctx);

		TheoraWebmDec::webm_rewind(input.webm_ctx);

#ifdef _DEBUG
		float fps = (float)input.vpx_input_ctx->framerate.numerator / (float)input.vpx_input_ctx->framerate.denominator;
		log("Framerate: " + strf(fps));
#endif

		this->width = input.vpx_input_ctx->width;
		this->height = input.vpx_input_ctx->height;
		this->subFrameWidth = input.vpx_input_ctx->width;
		this->subFrameHeight = input.vpx_input_ctx->height;
		this->subFrameOffsetX = 0;
		this->subFrameOffsetY = 0;
		this->stride = (this->stride == 1) ? potCeil(getWidth()) : getWidth();

		this->fps = (float)input.vpx_input_ctx->framerate.numerator / (float)input.vpx_input_ctx->framerate.denominator;
		this->frameDuration = 1.0f / this->fps;
		this->duration = this->numFrames * this->frameDuration;

#ifdef _DEBUG
		log("Video duration: " + strf(this->duration));
#endif

		fourcc_interface = (VpxInterface*)get_vpx_decoder_by_fourcc(vpx_input_ctx.fourcc);
		interf = fourcc_interface;

		int dec_flags = 0;
		if (vpx_codec_dec_init(&decoder, interf->codec_interface(),
			&cfg, dec_flags))
		{
			log("Error: Failed to initialize decoder: " + std::string(vpx_codec_error(&decoder)));
			return;
		}

		if (this->frameQueue == NULL)
		{
			this->frameQueue = new FrameQueue(this);
			this->frameQueue->setSize(this->precachedFramesCount);
		}
	}
	
	void VideoClip_WebM::decodedAudioCheck()
	{
		if (!this->audioInterface || this->timer->isPaused()) return;

		Mutex::ScopeLock lock(this->audioMutex);
		flushAudioPackets(this->audioInterface);
		lock.release();
	}

	float VideoClip_WebM::decodeAudio()
	{
		return -1;
	}

	void VideoClip_WebM::doSeek()
	{
		float time = this->seekFrame / getFps();
		this->timer->seek(time);
		bool paused = this->timer->isPaused();
		if (!paused)
		{
			this->timer->pause();
		}
		this->resetFrameQueue();
#ifdef _DEBUG
		log("Seek frame: " + str(this->seekFrame));
#endif
		this->lastDecodedFrameNumber = this->seekFrame;
		if (!paused)
		{
			this->timer->play();
		}
		this->seekFrame = -1;
	}

}
