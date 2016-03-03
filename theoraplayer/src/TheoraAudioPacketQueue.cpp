/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <stdlib.h>
#include "TheoraAudioPacketQueue.h"
#include "TheoraAudioInterface.h"

TheoraAudioPacketQueue::TheoraAudioPacketQueue()
{
	this->theoraAudioPacketQueue = NULL;
}

TheoraAudioPacketQueue::~TheoraAudioPacketQueue()
{
	destroyAllAudioPackets();
}

float TheoraAudioPacketQueue::getAudioPacketQueueLength()
{
	float len = 0;
	for (TheoraAudioPacket* p = this->theoraAudioPacketQueue; p != NULL; p = p->next)
		len += p->numSamples;
	
	return len / (this->audioFrequency * this->numAudioChannels);
}

void TheoraAudioPacketQueue::_addAudioPacket(float* data, int numSamples)
{
	TheoraAudioPacket* packet = new TheoraAudioPacket;
	packet->pcm = data;
	packet->numSamples = numSamples;
	packet->next = NULL;

	if (this->theoraAudioPacketQueue == NULL) this->theoraAudioPacketQueue = packet;
	else
	{
		TheoraAudioPacket* last = this->theoraAudioPacketQueue;
		for (TheoraAudioPacket* p = last; p != NULL; p = p->next)
			last = p;
		last->next = packet;
	}
}

void TheoraAudioPacketQueue::addAudioPacket(float** buffer, int numSamples, float gain)
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

void TheoraAudioPacketQueue::addAudioPacket(float* buffer, int numSamples, float gain)
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

TheoraAudioPacket* TheoraAudioPacketQueue::popAudioPacket()
{
	if (this->theoraAudioPacketQueue == NULL) return NULL;
	TheoraAudioPacket* p = this->theoraAudioPacketQueue;
	this->theoraAudioPacketQueue = this->theoraAudioPacketQueue->next;
	return p;
}

void TheoraAudioPacketQueue::destroyAudioPacket(TheoraAudioPacket* p)
{
	if (p != NULL)
	{
		delete[] p->pcm;
		delete p;
	}
}

void TheoraAudioPacketQueue::destroyAllAudioPackets()
{
	for (TheoraAudioPacket* p = popAudioPacket(); p != NULL; p = popAudioPacket())
		destroyAudioPacket(p);
}

void TheoraAudioPacketQueue::flushAudioPackets(TheoraAudioInterface* audioInterface)
{
	
	for (TheoraAudioPacket* p = popAudioPacket(); p != NULL; p = popAudioPacket())
	{
		audioInterface->insertData(p->pcm, p->numSamples);
		destroyAudioPacket(p);
	}
}