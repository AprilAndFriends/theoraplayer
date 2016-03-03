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

#ifndef THEORA_AUDIOINTERFACE_H
#define THEORA_AUDIOINTERFACE_H

#include "theoraplayerExport.h"

class TheoraVideoClip;


/**
 This is the class that serves as an interface between the library's audio
 output and the audio playback library of your choice.
 The class gets mono or stereo PCM data in in floating point data
 */
class theoraplayerExport TheoraAudioInterface
{
public:
	//! PCM frequency, usualy 44100 Hz
	int freq;
	//! Mono or stereo
	int numChannels;
	//! Pointer to the parent TheoraVideoClip object
	TheoraVideoClip* clip;
	
	TheoraAudioInterface(TheoraVideoClip* owner, int nChannels, int freq);
	virtual ~TheoraAudioInterface();
	//! A function that the TheoraVideoClip object calls once more audio packets are decoded
	/*!
	 \param data contains one or two channels of float PCM data in the range [-1,1]
	 \param nSamples contains the number of samples that the data parameter contains in each channel
	 */
	virtual void insertData(float* data, int nSamples)=0;	
};

class theoraplayerExport TheoraAudioInterfaceFactory
{
public:
	//! VideoManager calls this when creating a new TheoraVideoClip object
	virtual TheoraAudioInterface* createInstance(TheoraVideoClip* owner, int nChannels, int freq) = 0;
};


#endif

