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
/// Implements an interface to be able to process the FFMPEG codec.

#ifdef __FFMPEG
#ifndef THEORAPLAYER_VIDEO_CLIP_FFMPEG_H
#define THEORAPLAYER_VIDEO_CLIP_FFMPEG_H

#include "AudioPacketQueue.h"

#include "VideoClip.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVIOContext;

// TODOth - refactor for new convention
namespace theoraplayer
{
	class VideoClip_FFmpeg : public VideoClip, public AudioPacketQueue
	{
	protected:
		bool mLoaded;

		AVFormatContext* mFormatContext;
		AVCodecContext* mCodecContext;
		AVIOContext* mAvioContext;
		AVCodec* mCodec;
		AVFrame* mFrame;
		unsigned char* mInputBuffer;
		int mVideoStreamIndex;
		int mFrameNumber;

		void unload();
		void doSeek();
	public:
		VideoClip_FFmpeg(DataSource* data_source, TheoraOutputMode output_mode, int nPrecachedFrames, bool usePower2Stride);
		~VideoClip_FFmpeg();

		bool _readData();
		bool decodeNextFrame();
		void _restart();
		void _load(DataSource* source);
		float decodeAudio();
		void decodedAudioCheck();
		std::string getDecoderName() { return "FFmpeg"; }
	};

}
#endif
#endif
