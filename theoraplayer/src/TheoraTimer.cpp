/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "TheoraTimer.h"

TheoraTimer::TheoraTimer()
{
	this->time = 0;
	this->paused = 0;
	this->speed = 1.0f;
}

TheoraTimer::~TheoraTimer()
{

}

void TheoraTimer::update(float timeDelta)
{
	if (!isPaused())
	{
		this->time += timeDelta * this->speed;
	}
}

float TheoraTimer::getTime()
{
	return this->time;
}

void TheoraTimer::pause()
{
	this->paused = true;
}

void TheoraTimer::play()
{
	this->paused = false;
}


bool TheoraTimer::isPaused()
{
	return this->paused;
}

void TheoraTimer::stop()
{

}

void TheoraTimer::seek(float time)
{
	this->time = time;
}

void TheoraTimer::setSpeed(float speed)
{
	this->speed = speed;
}

float TheoraTimer::getSpeed()
{
	return this->speed;
}
