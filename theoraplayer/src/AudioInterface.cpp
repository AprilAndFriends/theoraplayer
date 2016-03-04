/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "AudioInterface.h"
#include "theoraplayer.h"

namespace theoraplayer
{
	AudioInterface::AudioInterface(theoraplayer::VideoClip* owner, int nChannels, int freq)
	{
		this->freq = freq;
		this->numChannels = nChannels;
		this->clip = owner;
	}

	AudioInterface::~AudioInterface()
	{

	}
}
