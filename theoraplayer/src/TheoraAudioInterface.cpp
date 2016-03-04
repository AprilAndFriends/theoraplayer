/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "TheoraAudioInterface.h"

TheoraAudioInterface::TheoraAudioInterface(VideoClip* owner, int nChannels, int freq)
{
	this->freq = freq;
	this->numChannels = nChannels;
	this->clip = owner;
}

TheoraAudioInterface::~TheoraAudioInterface()
{
	
}
