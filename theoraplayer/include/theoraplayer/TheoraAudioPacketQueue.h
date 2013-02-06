/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _TheoraAudioPacketQueue_h
#define _TheoraAudioPacketQueue_h

#include "TheoraExport.h"

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
class TheoraPlayerExport TheoraAudioPacketQueue
{
protected:
	unsigned int mAudioFrequency, mNumAudioChannels;
	TheoraAudioPacket* mTheoraAudioPacketQueue;
	void _addAudioPacket(float* data, int numSamples);
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
};

#endif
