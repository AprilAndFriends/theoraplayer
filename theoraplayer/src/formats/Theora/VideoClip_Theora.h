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
/// Implements an interface to be able to process the Theora codec.

#ifdef _USE_THEORA
#ifndef THEORAPLAYER_VIDEO_CLIP_THEORA_H
#define THEORAPLAYER_VIDEO_CLIP_THEORA_H

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <vorbis/vorbisfile.h>

#include "AudioPacketQueue.h"

#include "DataSource.h"
#include "VideoClip.h"

namespace theoraplayer
{
	struct TheoraInfoStruct
	{
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
	};

	class VideoClip_Theora : public VideoClip, public AudioPacketQueue
	{
	public:
		VideoClip_Theora(DataSource* data_source, TheoraOutputMode output_mode, int nPrecachedFrames, bool usePower2Stride);
		~VideoClip_Theora();

		bool _readData();
		bool decodeNextFrame();
		void _restart();
		void _load(DataSource* source);
		float decodeAudio();
		void decodedAudioCheck();
		std::string getDecoderName() { return "Theora"; }

		static VideoClip* create(DataSource* dataSource, TheoraOutputMode outputMode, int precachedFramesCount, bool usePotStride);

	protected:
		// TODOth - the comment below seems incorrect
		TheoraInfoStruct info; // a pointer is used to avoid having to include theora & vorbis headers
		int theoraStreams;
		int vorbisStreams;	// Keeps track of Theora and Vorbis Streams

		long seekPage(long targetFrame, bool returnKeyFrame);
		void _doSeek();
		void readTheoraVorbisHeaders();

		unsigned int readAudioSamples;
		unsigned long lastDecodedFrameNumber;

	};

}
#endif
#endif
