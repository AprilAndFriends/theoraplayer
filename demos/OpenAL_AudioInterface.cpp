/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include <stdio.h>
#include "OpenAL_AudioInterface.h"

ALCdevice* gDevice=0;
ALCcontext* gContext=0;

short float2short(float f)
{
	if (f > 1)
	{
		f = 1;
	}
	else if (f < -1)
	{
		f = -1;
	}
	return (short) (f*32767);
}

OpenAL_AudioInterface::OpenAL_AudioInterface(TheoraVideoClip* owner,int nChannels,int freq) :
	TheoraAudioInterface(owner,nChannels,freq), TheoraTimer()
{
	this->sourceNumChannels = this->numChannels;
	if (this->numChannels > 2)
	{
		// ignore audio with more than 2 channels, use only the stereo channels
		this->numChannels = 2;
	}
	this->maxBuffSize = freq * this->numChannels * 2;
	this->buffSize = 0;
	this->numProcessedSamples = 0;
	this->currentTimer = 0;

	this->tempBuffer = new short[this->maxBuffSize];
	alGenSources(1, &this->source);
	owner->setTimer(this);
	this->numPlayedSamples = 0;
}

OpenAL_AudioInterface::~OpenAL_AudioInterface()
{
	if (this->tempBuffer)
	{
		delete[] this->tempBuffer;
	}
	
	if (this->source)
	{
		alSourcei(this->source, AL_BUFFER, NULL);
		alDeleteSources(1, &this->source);
	}
	while (this->bufferQueue.size() > 0)
	{
		alDeleteBuffers(1, &this->bufferQueue.front().id);
		this->bufferQueue.pop();
	}
}

float OpenAL_AudioInterface::getQueuedAudioSize()
{
	return ((float) (this->numProcessedSamples - this->numPlayedSamples)) / this->freq;
}

void OpenAL_AudioInterface::insertData(float* data, int nSamples)
{
	float* tempData = NULL;
	if (this->sourceNumChannels > 2)
	{
		tempData = new float[nSamples * 2 / this->sourceNumChannels + 16]; // 16 padding just in case
		int i, n;
		for (n = 0, i = 0; i < nSamples; i += this->sourceNumChannels, n += 2)
		{
			tempData[n] = data[i];
			tempData[n + 1] = data[i + 1];
		}
		data = tempData;
		nSamples = n;
	}
	//printf("got %d bytes, %d buffers queued\n",nSamples,(int)this->bufferQueue.size());
	for (int i = 0; i < nSamples; i++)
	{
		if (this->buffSize < this->maxBuffSize)
		{
			this->tempBuffer[this->buffSize++]=float2short(data[i]);
		}
		if (this->buffSize == this->freq * this->numChannels / 10)
		{
			OpenAL_Buffer buff;
			alGenBuffers(1,&buff.id);

			ALuint format = (this->numChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
			alBufferData(buff.id,format,this->tempBuffer,this->buffSize*2, this->freq);
			alSourceQueueBuffers(this->source, 1, &buff.id);
			buff.nSamples=this->buffSize/this->numChannels;
			this->numProcessedSamples+=this->buffSize/this->numChannels;
			this->bufferQueue.push(buff);

			this->buffSize=0;

			int state;
			alGetSourcei(this->source,AL_SOURCE_STATE,&state);
			if (state != AL_PLAYING)
			{
				//alSourcef(this->source,AL_PITCH,0.5); // debug
				//alSourcef(this->source,AL_SAMPLE_OFFSET,(float) this->numProcessedSamples-mFreq/4);
				alSourcePlay(this->source);
			}

		}
	}
	if (tempData)
	{
		delete [] tempData;
	}
}

void OpenAL_AudioInterface::update(float time_increase)
{
	int i,/*state,*/nProcessed;
	OpenAL_Buffer buff;

	// process played buffers

	alGetSourcei(this->source,AL_BUFFERS_PROCESSED,&nProcessed);
	
	for (i=0;i<nProcessed;i++)
	{
		buff=this->bufferQueue.front();
		this->bufferQueue.pop();
		this->numPlayedSamples+=buff.nSamples;
		alSourceUnqueueBuffers(this->source,1,&buff.id);
		alDeleteBuffers(1,&buff.id);
	}
	if (nProcessed != 0)
	{
		// update offset
		alGetSourcef(this->source,AL_SEC_OFFSET,&this->currentTimer);
	}

	// control playback and return time position
	//alGetSourcei(this->source,AL_SOURCE_STATE,&state);
	//if (state == AL_PLAYING)
		this->currentTimer += time_increase;

	this->time = this->currentTimer + (float) this->numPlayedSamples/this->freq;

	float duration=this->clip->getDuration();
	if (this->time > duration)
	{
		this->time = duration;
	}
}

void OpenAL_AudioInterface::pause()
{
	alSourcePause(this->source);
	TheoraTimer::pause();
}

void OpenAL_AudioInterface::play()
{
	alSourcePlay(this->source);
	TheoraTimer::play();
}

void OpenAL_AudioInterface::seek(float time)
{
	OpenAL_Buffer buff;

	alSourceStop(this->source);
	while (!this->bufferQueue.empty())
	{
		buff=this->bufferQueue.front();
		this->bufferQueue.pop();
		alSourceUnqueueBuffers(this->source,1,&buff.id);
		alDeleteBuffers(1,&buff.id);
	}
//		int nProcessed;
//		alGetSourcei(this->source,AL_BUFFERS_PROCESSED,&nProcessed);
//		if (nProcessed != 0)
//			nProcessed=nProcessed;
	this->buffSize=0;

	this->currentTimer = 0;
	this->numPlayedSamples=this->numProcessedSamples=(int) (time*this->freq);
	this->time = time;
}

OpenAL_AudioInterfaceFactory::OpenAL_AudioInterfaceFactory()
{
	// openal init is here used only to simplify samples for this plugin
	// if you want to use this interface in your own program, you'll
	// probably want to remove the openal init/destory lines
	gDevice = alcOpenDevice("");
	if (alcGetError(gDevice) != ALC_NO_ERROR)
	{
		goto Fail;
	}
	gContext = alcCreateContext(gDevice, NULL);
	if (alcGetError(gDevice) != ALC_NO_ERROR)
	{
		goto Fail;
	}
	alcMakeContextCurrent(gContext);
	if (alcGetError(gDevice) != ALC_NO_ERROR)
	{
		goto Fail;
	}

	return;
Fail:
	gDevice=NULL;
	gContext=NULL;
}

OpenAL_AudioInterfaceFactory::~OpenAL_AudioInterfaceFactory()
{
	if (gDevice)
	{
		alcMakeContextCurrent(NULL);
		alcDestroyContext(gContext);
		alcCloseDevice(gDevice);
	}
}

OpenAL_AudioInterface* OpenAL_AudioInterfaceFactory::createInstance(TheoraVideoClip* owner,int nChannels,int freq)
{
	return new OpenAL_AudioInterface(owner,nChannels,freq);
}
