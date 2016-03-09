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

#ifndef THEORAPLAYER_AUDIO_PACKET_QUEUE_H
#define THEORAPLAYER_AUDIO_PACKET_QUEUE_H

#include "theoraplayerExport.h"
#include "theoraplayer.h"

namespace theoraplayer
{
	class AudioInterface;
	/**
	 This is an internal structure which VideoClip_Theora uses to store audio packets
	 */
	struct AudioPacket
	{
		float* pcm;
		int numSamples; //! size in number of float samples (stereo has twice the number of samples)
		AudioPacket* next; // pointer to the next audio packet, to implement a linked list
	};

	/**
		This is a Mutex object, used in thread syncronization.
	 */
	class theoraplayerExport AudioPacketQueue
	{
	public:
		AudioPacketQueue();
		~AudioPacketQueue();

		float getAudioPacketQueueLength();

		void addAudioPacket(float** buffer, int numSamples, float gain);
		void addAudioPacket(float* buffer, int numSamples, float gain);

		AudioPacket* popAudioPacket();

		void destroyAudioPacket(AudioPacket* p);
		void destroyAllAudioPackets();

		void flushAudioPackets(AudioInterface* audioInterface);

	protected:
		unsigned int audioFrequency;
		unsigned int audioChannelsCount;
		AudioPacket* theoraAudioPacketQueue;

		void _addAudioPacket(float* data, int numSamples);
		void _flushSynchronizedAudioPackets(AudioInterface* audioInterface, Mutex* mutex);

	};
}

#endif
