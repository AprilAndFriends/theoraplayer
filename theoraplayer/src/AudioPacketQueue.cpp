/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <stdlib.h>
#include "AudioPacketQueue.h"
#include "AudioInterface.h"

namespace theoraplayer
{

	AudioPacketQueue::AudioPacketQueue()
	{
		this->theoraAudioPacketQueue = NULL;
	}

	AudioPacketQueue::~AudioPacketQueue()
	{
		destroyAllAudioPackets();
	}

	float AudioPacketQueue::getAudioPacketQueueLength()
	{
		float len = 0;
		for (AudioPacket* p = this->theoraAudioPacketQueue; p != NULL; p = p->next)
			len += p->numSamples;

		return len / (this->audioFrequency * this->numAudioChannels);
	}

	void AudioPacketQueue::_addAudioPacket(float* data, int numSamples)
	{
		AudioPacket* packet = new AudioPacket;
		packet->pcm = data;
		packet->numSamples = numSamples;
		packet->next = NULL;

		if (this->theoraAudioPacketQueue == NULL) this->theoraAudioPacketQueue = packet;
		else
		{
			AudioPacket* last = this->theoraAudioPacketQueue;
			for (AudioPacket* p = last; p != NULL; p = p->next)
				last = p;
			last->next = packet;
		}
	}

	void AudioPacketQueue::addAudioPacket(float** buffer, int numSamples, float gain)
	{
		float* data = new float[numSamples * this->numAudioChannels];
		float* dataptr = data;
		int i;
		unsigned int j;

		if (gain < 1.0f)
		{
			// apply gain, let's attenuate the samples
			for (i = 0; i < numSamples; ++i)
			{
				for (j = 0; j < this->numAudioChannels; j++, ++dataptr)
				{
					*dataptr = buffer[j][i] * gain;
				}
			}
		}
		else
		{
			// do a simple copy, faster then the above method, when gain is 1.0f
			for (i = 0; i < numSamples; ++i)
			{
				for (j = 0; j < this->numAudioChannels; j++, ++dataptr)
				{
					*dataptr = buffer[j][i];
				}
			}
		}

		_addAudioPacket(data, numSamples * this->numAudioChannels);
	}

	void AudioPacketQueue::addAudioPacket(float* buffer, int numSamples, float gain)
	{
		float* data = new float[numSamples * this->numAudioChannels];
		float* dataptr = data;
		int i, numFloats = numSamples * this->numAudioChannels;

		if (gain < 1.0f)
		{
			// apply gain, let's attenuate the samples
			for (i = 0; i < numFloats; ++i, dataptr++)
				*dataptr = buffer[i] * gain;
		}
		else
		{
			// do a simple copy, faster then the above method, when gain is 1.0f
			for (i = 0; i < numFloats; ++i, dataptr++)
				*dataptr = buffer[i];
		}

		_addAudioPacket(data, numFloats);
	}

	AudioPacket* AudioPacketQueue::popAudioPacket()
	{
		if (this->theoraAudioPacketQueue == NULL) return NULL;
		AudioPacket* p = this->theoraAudioPacketQueue;
		this->theoraAudioPacketQueue = this->theoraAudioPacketQueue->next;
		return p;
	}

	void AudioPacketQueue::destroyAudioPacket(AudioPacket* p)
	{
		if (p != NULL)
		{
			delete[] p->pcm;
			delete p;
		}
	}

	void AudioPacketQueue::destroyAllAudioPackets()
	{
		for (AudioPacket* p = popAudioPacket(); p != NULL; p = popAudioPacket())
			destroyAudioPacket(p);
	}

	void AudioPacketQueue::flushAudioPackets(AudioInterface* audioInterface)
	{

		for (AudioPacket* p = popAudioPacket(); p != NULL; p = popAudioPacket())
		{
			audioInterface->insertData(p->pcm, p->numSamples);
			destroyAudioPacket(p);
		}
	}
}