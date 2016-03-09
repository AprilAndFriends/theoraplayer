/// @file
/// @version 1.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <algorithm>
#include <memory.h>
#include <string>

//#include "Manager.h"
//#include "AudioInterface.h"

//#include "DataSource.h"
//#include "Exception.h"
//#include "Mutex.h"
//#include "theoraplayer.h"
//#include "Utility.h"
#include <theoraplayer/PixelTransform.h>
#include <theoraplayer/FrameQueue.h>
#include <theoraplayer/Timer.h>
#include <theoraplayer/VideoClip.h>
#include <theoraplayer/VideoFrame.h>
#include <tools_common.h>

#include "Utility.h"
#include "VideoClip.h"
#include "webmdec.h"

namespace clipwebm
{
	VideoClip::VideoClip(theoraplayer::DataSource* dataSource, theoraplayer::TheoraOutputMode outputMode, int precachedFramesCount, bool usePotStride) :
		theoraplayer::VideoClip(dataSource, outputMode, precachedFramesCount, usePotStride),
		AudioPacketQueue()
	{
		memset(&(webm_ctx), 0, sizeof(webm_ctx));
		this->input.webm_ctx = &webm_ctx;
		this->input.vpx_input_ctx = &vpx_input_ctx;
		this->seekFrame = 0;
		this->frameNumber = 0;
	}

	theoraplayer::VideoClip* VideoClip::create(theoraplayer::DataSource* dataSource, theoraplayer::TheoraOutputMode outputMode, int precachedFramesCount, bool usePotStride)
	{
		return new VideoClip(dataSource, outputMode, precachedFramesCount, usePotStride);
	}

	VideoClip::~VideoClip()
	{
		webm_free(this->input.webm_ctx);
	}

	bool VideoClip::_readData()
	{
		return 1;
	}

	bool VideoClip::decodeNextFrame()
	{
		theoraplayer::VideoFrame* frame = this->frameQueue->requestEmptyFrame();
		if (frame == NULL)
		{
			return 0; // max number of precached frames reached
		}
		bool should_restart = 0;

		uint8_t* buf = NULL;
		size_t bytes_in_buffer = 0, buffer_size = 0;

		if (!webm_read_frame(this->input.webm_ctx, &buf, &bytes_in_buffer, &buffer_size))
		{
			vpx_codec_iter_t  iter = NULL;
			vpx_image_t    *img;

			if (vpx_codec_decode(&decoder, buf, (unsigned int)bytes_in_buffer,
				NULL, 0))
			{
				const char *detail = vpx_codec_error_detail(&decoder);
				if (detail != NULL)
				{
					theoraplayer::log("Additional information: " + std::string(detail));
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

	void VideoClip::_restart()
	{
		bool paused = this->timer->isPaused();
		if (!paused) this->timer->pause();

		webm_rewind(input.webm_ctx);
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

	void VideoClip::_load(theoraplayer::DataSource* source)
	{
		if (!file_is_webm(source, input.webm_ctx, input.vpx_input_ctx))
		{
			theoraplayer::log("Error: File is not webm.");
			return;
		}

		if (webm_guess_framerate(source, input.webm_ctx, input.vpx_input_ctx))
		{
			theoraplayer::log("Error: Unable to guess webm framerate.");
			return;
		}

		this->numFrames = webm_guess_duration(input.webm_ctx);

		webm_rewind(input.webm_ctx);

#ifdef _DEBUG
		float fps = (float)input.vpx_input_ctx->framerate.numerator / (float)input.vpx_input_ctx->framerate.denominator;
		theoraplayer::log("Framerate: " + strf(fps));
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
		theoraplayer::log("Video duration: " + strf(this->duration));
#endif

		fourcc_interface = (VpxInterface*)get_vpx_decoder_by_fourcc(vpx_input_ctx.fourcc);
		interf = fourcc_interface;

		int dec_flags = 0;
		if (vpx_codec_dec_init(&decoder, interf->codec_interface(),
			&cfg, dec_flags))
		{
			theoraplayer::log("Error: Failed to initialize decoder: " + std::string(vpx_codec_error(&decoder)));
			return;
		}

		if (this->frameQueue == NULL)
		{
			this->frameQueue = new theoraplayer::FrameQueue(this);
			this->frameQueue->setSize(this->precachedFramesCount);
		}
	}
	
	void VideoClip::decodedAudioCheck()
	{
		if (this->audioInterface != NULL && !this->timer->isPaused())
		{
			this->_flushSynchronizedAudioPackets(this->audioInterface, this->audioMutex);
		}
	}

	float VideoClip::decodeAudio()
	{
		return -1;
	}

	void VideoClip::doSeek()
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
		theoraplayer::log("Seek frame: " + str(this->seekFrame));
#endif
		this->lastDecodedFrameNumber = this->seekFrame;
		if (!paused)
		{
			this->timer->play();
		}
		this->seekFrame = -1;
	}

}
