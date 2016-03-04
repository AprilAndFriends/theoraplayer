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
/// Provides an interface for connecting videos with audio.

#ifndef THEORAPLAYER_AUDIO_INTERFACE_H
#define THEORAPLAYER_AUDIO_INTERFACE_H

#include "theoraplayerExport.h"
#include "AudioInterfaceFactory.h"

namespace theoraplayer
{
	class VideoClip;

	/**
	 This is the class that serves as an interface between the library's audio
	 output and the audio playback library of your choice.
	 The class gets mono or stereo PCM data in in floating point data
	 */
	class theoraplayerExport AudioInterface
	{
	public:
		//! PCM frequency, usualy 44100 Hz
		int freq;
		//! Mono or stereo
		int numChannels;
		//! Pointer to the parent VideoClip object
		theoraplayer::VideoClip* clip;

		AudioInterface(theoraplayer::VideoClip* owner, int nChannels, int freq);
		virtual ~AudioInterface();
		//! A function that the VideoClip object calls once more audio packets are decoded
		/*!
		 \param data contains one or two channels of float PCM data in the range [-1,1]
		 \param nSamples contains the number of samples that the data parameter contains in each channel
		 */
		virtual void insertData(float* data, int nSamples) = 0;

	};
}


#endif

