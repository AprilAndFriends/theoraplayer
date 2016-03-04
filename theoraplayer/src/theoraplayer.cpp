/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "Manager.h"
#include "theoraplayer.h"
#include "Utility.h"

namespace theoraplayer
{
	static void _log(const std::string& output)
	{
		printf("%s\n", output.c_str());
	}
	static void (*logFunction)(const std::string&) = _log;

	void init(int workerThreadCount)
	{
		theoraplayer::manager = new Manager(workerThreadCount);
	}

	void destroy()
	{
		delete theoraplayer::manager;
		theoraplayer::manager = NULL;
	}

	void setLogFunction(void (*function)(const std::string&))
	{
		logFunction = function;
		if (logFunction == NULL)
		{
			logFunction = _log;
		}
	}

	void log(const std::string& message)
	{
		(*logFunction)(message);
	}

	void registerFormatLoader()
	{

	}

}
