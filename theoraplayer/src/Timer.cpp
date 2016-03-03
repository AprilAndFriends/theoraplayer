/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "Timer.h"

namespace theoraplayer
{
	Timer::Timer()
	{
		this->time = 0;
		this->paused = 0;
		this->speed = 1.0f;
	}

	Timer::~Timer()
	{
	}

	void Timer::update(float timeDelta)
	{
		if (!this->isPaused())
		{
			this->time += timeDelta * this->speed;
		}
	}

	float Timer::getTime()
	{
		return this->time;
	}

	void Timer::pause()
	{
		this->paused = true;
	}

	void Timer::play()
	{
		this->paused = false;
	}

	bool Timer::isPaused()
	{
		return this->paused;
	}

	void Timer::stop()
	{
	}

	void Timer::seek(float time)
	{
		this->time = time;
	}

	void Timer::setSpeed(float speed)
	{
		this->speed = speed;
	}

	float Timer::getSpeed()
	{
		return this->speed;
	}

}
