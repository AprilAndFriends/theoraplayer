/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "TheoraException.h"
#include "TheoraUtil.h"
#include "TheoraVideoManager.h"
#include <stdio.h>

_TheoraGenericException::_TheoraGenericException(const std::string& errorText,std::string type,std::string file,int line)
{
    mErrText = errorText;
	int src=file.find("src");
	if (src >= 0) file=file.substr(src+4,1000);
	mLineNumber=line;
	mFile=file;
}


std::string _TheoraGenericException::repr()
{
	std::string text=getType();
	if (text != "") text+=": ";

	if (mFile != "") text+="["+mFile+":"+str(mLineNumber)+"] - ";

	return text + getErrorText();
}

void _TheoraGenericException::writeOutput()
{
	th_writelog("----------------\nException Error!\n\n"+repr()+"\n----------------");
}
