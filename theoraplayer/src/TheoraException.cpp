/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#include <stdio.h>

#include "TheoraException.h"
#include "TheoraUtil.h"
#include "TheoraVideoManager.h"

_TheoraGenericException::_TheoraGenericException(const std::string& errorText, std::string type, std::string file, int line)
{
	this->errText = errorText;
	int src = (int) file.find("src");
	if (src >= 0) file = file.substr(src + 4, 1000);
	this->lineNumber = line;
	this->file = file;
}


std::string _TheoraGenericException::repr()
{
	std::string text = getType();
	if (text != "")
	{
		text += ": ";
	}

	if (this->file != "") 
	{
		text += "[" + this->file + ":" + str(this->lineNumber) + "] - ";
	}

	return text + getErrorText();
}

void _TheoraGenericException::writeOutput()
{
	th_writelog("----------------\nException Error!\n\n" + repr() + "\n----------------");
}
