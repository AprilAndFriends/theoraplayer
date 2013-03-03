/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
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
	TheoraVideoClip_AVFoundation(TheoraDataSource* data_source,
								 TheoraOutputMode output_mode,
								 int nPrecachedFrames,
								 bool usePower2Stride);
	~TheoraVideoClip_AVFoundation();
	
	bool _readData();
	bool decodeNextFrame();
	void _restart();
	void load(TheoraDataSource* source);
	float decodeAudio();
	void decodedAudioCheck();
	std::string getDecoderName() { return "AVFoundation"; }
};

#endif
