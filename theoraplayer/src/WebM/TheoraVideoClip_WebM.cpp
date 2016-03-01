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
#include "TheoraVideoClip_WebM.h"
#include "TheoraPixelTransform.h"
#include <stdio.h>
#include <stdlib.h>

#include "webmdec.h"

TheoraVideoClip_WebM::TheoraVideoClip_WebM(TheoraDataSource* data_source,
										TheoraOutputMode output_mode,
										int nPrecachedFrames,
										bool usePower2Stride):
	TheoraVideoClip(data_source, output_mode, nPrecachedFrames, usePower2Stride),
	TheoraAudioPacketQueue()
{	
	memset(&(webm_ctx), 0, sizeof(webm_ctx));
	input.webm_ctx = &webm_ctx;
	input.vpx_input_ctx = &vpx_input_ctx;
	mSeekFrame = 0;
	this->data_source = data_source;

	mFrameNumber = 0;
}

TheoraVideoClip_WebM::~TheoraVideoClip_WebM()
{
	TheoraWebmDec::webm_free(input.webm_ctx);
}

bool TheoraVideoClip_WebM::_readData()
{
	return 1;
}

bool TheoraVideoClip_WebM::decodeNextFrame()
{	
	TheoraVideoFrame* frame = mFrameQueue->requestEmptyFrame();
	
	if (!frame) return 0; // max number of precached frames reached
	bool should_restart = 0;

	uint8_t* buf = NULL;
	size_t bytes_in_buffer = 0, buffer_size = 0;
	
	if (!TheoraWebmDec::webm_read_frame(input.webm_ctx, &buf, &bytes_in_buffer, &buffer_size))
	{
		vpx_codec_iter_t  iter = NULL;
		vpx_image_t    *img;

		if (vpx_codec_decode(&decoder, buf, (unsigned int)bytes_in_buffer,
			NULL, 0))
		{
			const char *detail = vpx_codec_error_detail(&decoder);

			if (detail)
				warn("Additional information: %s", detail);
		}
		if ((img = vpx_codec_get_frame(&decoder, &iter)))
		{
			mFrame = img;

			frame->mTimeToDisplay = mFrameNumber / mFPS;
			frame->mIteration = mIteration;
			frame->_setFrameNumber(mFrameNumber++);

			TheoraPixelTransform t;
			memset(&t, 0, sizeof(TheoraPixelTransform));

			t.y = mFrame->planes[0]; t.yStride = mFrame->stride[0];
			t.u = mFrame->planes[1]; t.uStride = mFrame->stride[1];
			t.v = mFrame->planes[2]; t.vStride = mFrame->stride[2];

			frame->decode(&t);
		}
	}		
	return 1;
}

void TheoraVideoClip_WebM::_restart()
{
	
}

void TheoraVideoClip_WebM::load(TheoraDataSource* source)
{	
	if (!TheoraWebmDec::file_is_webm(source, input.webm_ctx, input.vpx_input_ctx))
	{
		th_writelog("Error: File is not webm.");
		return;
	}

	if (TheoraWebmDec::webm_guess_framerate(source, input.webm_ctx, input.vpx_input_ctx))
	{
		th_writelog("Error: Unable to guess webm framerate.");
		return;
	}

	mNumFrames = TheoraWebmDec::webm_guess_duration(input.webm_ctx);
	TheoraWebmDec::webm_free(input.webm_ctx); //hack, because no rewind functionality

	TheoraWebmDec::file_is_webm(source, input.webm_ctx, input.vpx_input_ctx);
	TheoraWebmDec::webm_guess_framerate(source, input.webm_ctx, input.vpx_input_ctx);

	printf("(Debug) Frameratea: %d\n", input.vpx_input_ctx->framerate.numerator / input.vpx_input_ctx->framerate.denominator);

	mWidth = input.vpx_input_ctx->width;
	mHeight = input.vpx_input_ctx->height;
	mSubFrameWidth = input.vpx_input_ctx->width;
	mSubFrameHeight = input.vpx_input_ctx->height;
	mSubFrameOffsetX = 0;
	mSubFrameOffsetY = 0;
	mStride = (mStride == 1) ? _nextPow2(getWidth()) : getWidth();

	mFPS = input.vpx_input_ctx->framerate.numerator / input.vpx_input_ctx->framerate.denominator;		
	mFrameDuration = 1.0f / mFPS;
	mDuration = mNumFrames * mFrameDuration;

	printf("Video duration: %f", mDuration);

	fourcc_interface = (VpxInterface*)get_vpx_decoder_by_fourcc(vpx_input_ctx.fourcc);
	interf = fourcc_interface;
	
	int dec_flags = 0;
	if (vpx_codec_dec_init(&decoder, interf->codec_interface(),
		&cfg, dec_flags)) 
	{
		fprintf(stderr, "Error: Failed to initialize decoder: %s\n",
			vpx_codec_error(&decoder));
		return;
	}

	if (mFrameQueue == NULL) 
	{
		mFrameQueue = new TheoraFrameQueue(this);
		mFrameQueue->setSize(mNumPrecachedFrames);
	}		
}

void TheoraVideoClip_WebM::decodedAudioCheck()
{
	if (!mAudioInterface || mTimer->isPaused()) return;

	TheoraMutex::ScopeLock lock(mAudioMutex);
	flushAudioPackets(mAudioInterface);
	lock.release();
}

float TheoraVideoClip_WebM::decodeAudio()
{
	return -1;
}

void TheoraVideoClip_WebM::doSeek()
{
	uint32_t i = 0;

	int frame;
	float time = mSeekFrame / getFPS();
	mTimer->seek(time);
	bool paused = mTimer->isPaused();
	if (!paused) mTimer->pause();
	
	resetFrameQueue();	

	printf("%d", mSeekFrame);

	/*uint8_t* buf = NULL;
	size_t bytes_in_buffer = 0, buffer_size = 0;

	while (!TheoraWebmDec::webm_read_frame(input.webm_ctx, &buf, &bytes_in_buffer, &buffer_size) && i<mSeekFrame)
	{
		i++;
	}*/

	mLastDecodedFrameNumber = mSeekFrame;

	decodeNextFrame();

	if (!paused) mTimer->play();
	mSeekFrame = -1;
	printf("ayy");
}

#endif
