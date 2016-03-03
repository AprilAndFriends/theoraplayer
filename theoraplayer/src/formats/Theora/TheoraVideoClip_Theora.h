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

#if defined(__THEORA) && !defined(THEORA_VIDEOCLIP_THEORA_H)
#define THEORA_VIDEOCLIP_THEORA_H

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <theora/theoradec.h>

#include "TheoraAudioPacketQueue.h"
#include "TheoraVideoClip.h"

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

class TheoraVideoClip_Theora : public TheoraVideoClip, public TheoraAudioPacketQueue
{
public:
	TheoraVideoClip_Theora(TheoraDataSource* data_source,
						   TheoraOutputMode output_mode,
						   int nPrecachedFrames,
						   bool usePower2Stride);
	~TheoraVideoClip_Theora();

	bool _readData();
	bool decodeNextFrame();
	void _restart();
	void load(TheoraDataSource* source);
	float decodeAudio();
	void decodedAudioCheck();
	std::string getDecoderName() { return "Theora"; }

protected:
	TheoraInfoStruct info; // a pointer is used to avoid having to include theora & vorbis headers
	int theoraStreams, vorbisStreams;	// Keeps track of Theora and Vorbis Streams

	long seekPage(long targetFrame, bool return_keyframe);
	void doSeek();
	void readTheoraVorbisHeaders();

	unsigned int readAudioSamples;
	unsigned long lastDecodedFrameNumber;
};

#endif
