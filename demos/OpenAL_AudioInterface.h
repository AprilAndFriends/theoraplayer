/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef OPENAL_AUDIOINTERFACE_H
#define OPENAL_AUDIOINTERFACE_H

#include <theoraplayer/AudioInterface.h>
#include <theoraplayer/VideoClip.h>

#include <theoraplayer/Timer.h>

#ifndef __APPLE__
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif
#include <queue>

class OpenAL_AudioInterface : public theoraplayer::AudioInterface, theoraplayer::Timer
{	
public:
	OpenAL_AudioInterface(theoraplayer::VideoClip* owner,int nChannels,int freq);
	~OpenAL_AudioInterface();

	//! queued audio buffers, expressed in seconds
	float getQueuedAudioSize();

	void insertData(float* data,int nSamples);	

	void update(float time_increase);

	void pause();
	void play();
	void seek(float time);

private:
	int sourceNumChannels;
	int maxBuffSize;
	int buffSize;
	short *tempBuffer;
	float currentTimer;

	struct OpenAL_Buffer
	{
		ALuint id;
		int nSamples;
	};
	std::queue<OpenAL_Buffer> bufferQueue;

	ALuint source;
	int numProcessedSamples, numPlayedSamples;
};



class OpenAL_AudioInterfaceFactory : public theoraplayer::AudioInterfaceFactory
{
public:
	OpenAL_AudioInterfaceFactory();
	~OpenAL_AudioInterfaceFactory();
	OpenAL_AudioInterface* createInstance(theoraplayer::VideoClip* owner,int nChannels,int freq);
};

#endif
