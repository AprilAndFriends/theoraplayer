/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "TheoraTimer.h"

TheoraTimer::TheoraTimer()
{
	mTime = 0;
	mPaused = 0;
    mSpeed = 1.0f;
}

TheoraTimer::~TheoraTimer()
{

}

void TheoraTimer::update(float time_increase)
{
	if (!mPaused)
		mTime += time_increase * mSpeed;
}

float TheoraTimer::getTime()
{
	return mTime;
}

void TheoraTimer::pause()
{
	mPaused = true;
}

void TheoraTimer::play()
{
	mPaused = false;
}


bool TheoraTimer::isPaused()
{
	return mPaused;
}

void TheoraTimer::stop()
{

}

void TheoraTimer::seek(float time)
{
	mTime = time;
}

void TheoraTimer::setSpeed(float speed)
{
    mSpeed = speed;
}

float TheoraTimer::getSpeed()
{
    return mSpeed;
}
