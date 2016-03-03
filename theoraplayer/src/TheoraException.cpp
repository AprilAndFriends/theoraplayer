/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

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


std::string _TheoraGenericException::toString()
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
	th_writelog("----------------\nException Error!\n\n" + toString() + "\n----------------");
}
