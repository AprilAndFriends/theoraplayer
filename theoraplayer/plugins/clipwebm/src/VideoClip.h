/// @file
/// @version 1.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Implements an interface to be able to process the WebM codec.

#ifndef CLIPWEBM_VIDEO_CLIP_H
#define THEORAPLAYER_VIDEO_CLIP_WEBM_H

//#include <ogg/ogg.h>
//#include <vorbis/vorbisfile.h>
//#include <theora/theoradec.h>

#include <theoraplayer/AudioPacketQueue.h>
#include <theoraplayer/DataSource.h>
#include <theoraplayer/VideoClip.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

#include "webmdec.h"

#define FORMAT_NAME "WebM"

namespace clipwebm
{
	struct VpxDecInputContext
	{
		struct VpxInputContext* vpx_input_ctx;
		struct WebmInputContext* webm_ctx;
	};

	class VideoClip : public theoraplayer::VideoClip, public theoraplayer::AudioPacketQueue
	{
	public:
		VideoClip(theoraplayer::DataSource* dataSource, theoraplayer::TheoraOutputMode outputMode, int precachedFramesCount, bool usePotStride);
		~VideoClip();

		bool _readData();
		bool decodeNextFrame();
		void _restart();
		float decodeAudio();
		void decodedAudioCheck();
		std::string getDecoderName() { return FORMAT_NAME; }

		static theoraplayer::VideoClip* create(theoraplayer::DataSource* dataSource, theoraplayer::TheoraOutputMode outputMode, int precachedFramesCount, bool usePotStride);

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
		void _load(theoraplayer::DataSource* source);

	};

}
#endif
