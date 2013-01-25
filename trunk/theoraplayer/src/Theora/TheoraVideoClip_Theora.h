/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#if defined(__THEORA) && !defined(_TheoraVideoClip_Theora_h)
#define _TheoraVideoClip_Theora_h

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <theora/theoradec.h>
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

/**
 This is an internal structure which TheoraVideoClip_Theora uses to store audio packets
 */
struct TheoraAudioPacket
{
	float* pcm;
	int num_samples; //! size in number of float samples (stereo has twice the number of samples)
	TheoraAudioPacket* next; // pointer to the next audio packet, to implement a linked list
};

class TheoraVideoClip_Theora : public TheoraVideoClip
{
protected:
	TheoraAudioPacket* mTheoraAudioPacketQueue;
	TheoraInfoStruct mInfo; // a pointer is used to avoid having to include theora & vorbis headers

	float getAudioPacketQueueLength();
	void addAudioPacket(float** buffer, int num_samples);
	TheoraAudioPacket* popAudioPacket();
	void destroyAudioPacket(TheoraAudioPacket* p);
	void destroyAllAudioPackets();
	long seekPage(long targetFrame, bool return_keyframe);
	void doSeek();
	void readTheoraVorbisHeaders();
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
};

#endif
