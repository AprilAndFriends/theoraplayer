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

#ifdef __AVFOUNDATION
#ifndef THEORAPLAYER_VIDEO_CLIP_AV_FOUNDATION_H
#define THEORAPLAYER_VIDEO_CLIP_AV_FOUNDATION_H

#include "AudioPacketQueue.h"
#include "VideoClip.h"

#ifndef AVFOUNDATION_CLASSES_DEFINED
class AVAssetReader;
class AVAssetReaderTrackOutput;
#endif

namespace theoraplayer
{
	class VideoClip_AVFoundation : public VideoClip, public AudioPacketQueue
	{
	protected:
		bool loaded;
		int frameNumber;
		AVAssetReader* reader;
		AVAssetReaderTrackOutput *output, *audioOutput;
		unsigned int readAudioSamples;
	
		void unload();
		void doSeek();
	public:
		VideoClip_AVFoundation(DataSource* data_source, TheoraOutputMode output_mode, int nPrecachedFrames, bool usePower2Stride);
		~VideoClip_AVFoundation();
	
		bool _readData();
		bool decodeNextFrame();
		void _restart();
		void load(TheoraDataSource* source);
		float decodeAudio();
		void decodedAudioCheck();
		std::string getDecoderName() { return "AVFoundation"; }

	};

}
#endif
#endif
