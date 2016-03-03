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
/// Defines an audio packet queue.

#ifndef THEORA_AUDIOPACKETQUEUE_H
#define THEORA_AUDIOPACKETQUEUE_H

#include "theoraplayerExport.h"

class TheoraAudioInterface;
/**
 This is an internal structure which TheoraVideoClip_Theora uses to store audio packets
 */
struct TheoraAudioPacket
{
	float* pcm;
	int numSamples; //! size in number of float samples (stereo has twice the number of samples)
	TheoraAudioPacket* next; // pointer to the next audio packet, to implement a linked list
};

/**
	This is a Mutex object, used in thread syncronization.
 */
class theoraplayerExport TheoraAudioPacketQueue
{
public:
	TheoraAudioPacketQueue();
	~TheoraAudioPacketQueue();
	
	float getAudioPacketQueueLength();

	void addAudioPacket(float** buffer, int numSamples, float gain);
	void addAudioPacket(float* buffer, int numSamples, float gain);

	TheoraAudioPacket* popAudioPacket();

	void destroyAudioPacket(TheoraAudioPacket* p);
	void destroyAllAudioPackets();
	
	void flushAudioPackets(TheoraAudioInterface* audioInterface);

protected:
	unsigned int audioFrequency, numAudioChannels;
	TheoraAudioPacket* theoraAudioPacketQueue;

	void _addAudioPacket(float* data, int numSamples);
};

#endif
