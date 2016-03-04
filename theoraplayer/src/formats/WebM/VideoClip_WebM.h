/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Implements an interface to be able to process the WebM codec.

#if 0
//#ifndef THEORAPLAYER_VIDEO_CLIP_WEBM_H
#define THEORAPLAYER_VIDEO_CLIP_WEBM_H

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <theora/theoradec.h>

#include "AudioPacketQueue.h"

#include "DataSource.h"
#include "VideoClip.h"

#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#include "webmdec.h"

namespace theoraplayer
{
	struct VpxDecInputContext
	{
		struct VpxInputContext *vpx_input_ctx;
		struct WebmInputContext *webm_ctx;
	};

	class VideoClip_WebM : public VideoClip, public AudioPacketQueue
	{
	public:
		VideoClip_WebM(DataSource* data_source, TheoraOutputMode output_mode, int nPrecachedFrames, bool usePower2Stride);
		~VideoClip_WebM();

		bool _readData();
		bool decodeNextFrame();
		void _restart();
		float decodeAudio();
		void decodedAudioCheck();
		std::string getDecoderName() { return "WebM"; }

	protected:
		vpx_codec_ctx_t decoder;
		vpx_codec_dec_cfg_t cfg;
		VpxDecInputContext input;
		VpxInputContext vpx_input_ctx;
		WebmInputContext webm_ctx;
		VpxInterface* fourcc_interface;
		VpxInterface* interf;
		vpx_image* frame;
		int frameNumber;

		void doSeek();
		unsigned long lastDecodedFrameNumber;
		void _load(DataSource* source);

	};

}
#endif
