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
/// Implements an interface to be able to process MP4 codes through AVFoundation.

#if defined(__AVFOUNDATION) && !defined(_TheoraVideoClip_AVFoundation_h)
#define _TheoraVideoClip_AVFoundation_h

#include "TheoraAudioPacketQueue.h"
#include "TheoraVideoClip.h"

#ifndef AVFOUNDATION_CLASSES_DEFINED
class AVAssetReader;
class AVAssetReaderTrackOutput;
#endif

class TheoraVideoClip_AVFoundation : public TheoraVideoClip, public TheoraAudioPacketQueue
{
protected:
	bool mLoaded;
	int mFrameNumber;
	AVAssetReader* mReader;
	AVAssetReaderTrackOutput *mOutput, *mAudioOutput;
	unsigned int mReadAudioSamples;
	
	void unload();
	void doSeek();
public:
	TheoraVideoClip_AVFoundation(DataSource* data_source,
								 TheoraOutputMode output_mode,
								 int nPrecachedFrames,
								 bool usePower2Stride);
	~TheoraVideoClip_AVFoundation();
	
	bool _readData();
	bool decodeNextFrame();
	void _restart();
	void load(DataSource* source);
	float decodeAudio();
	void decodedAudioCheck();
	std::string getDecoderName() { return "AVFoundation"; }
};

#endif
