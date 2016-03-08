/// @file
/// @version 1.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <theoraplayer/Manager.h>

#include "clipwebm.h"

namespace theoraplayer
{
	static bool initialized = false;

	void init()
	{
		if (!initialized && theoraplayer::manager != NULL)
		{
			initialized = true;
		}
	}

	void destroy()
	{
	}

}
