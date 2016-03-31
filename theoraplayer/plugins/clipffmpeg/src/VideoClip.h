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
/// Implements an interface to be able to process the FFmpeg codec.

#ifndef CLIPFFMPEG_VIDEO_CLIP_H
#define CLIPFFMPEG_VIDEO_CLIP_H

#include <theoraplayer/AudioPacketQueue.h>
#include <theoraplayer/DataSource.h>
#include <theoraplayer/VideoClip.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVIOContext;

#define FORMAT_NAME "FFmpeg"

namespace clipffmpeg
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
		void _load(theoraplayer::DataSource* source);
		float decodeAudio();
		void decodedAudioCheck();
		std::string getDecoderName() { return FORMAT_NAME; }

		static theoraplayer::VideoClip* create(theoraplayer::DataSource* dataSource, theoraplayer::TheoraOutputMode outputMode, int precachedFramesCount, bool usePotStride);

	protected:
		bool loaded;
		AVFormatContext* formatContext;
		AVCodecContext* codecContext;
		AVIOContext* avioContext;
		AVCodec* codec;
		AVFrame* frame;
		unsigned char* inputBuffer;
		int videoStreamIndex;
		int frameNumber;

		void unload();
		void _doSeek();

	};

}
#endif
