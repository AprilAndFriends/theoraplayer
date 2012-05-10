/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2012 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _theoraVideoExport_H
#define _theoraVideoExport_H

#ifdef _WIN32
	#ifdef THEORAVIDEO_STATIC
		#define TheoraPlayerExport
	#else
		#ifdef THEORAVIDEO_EXPORTS
			#define TheoraPlayerExport __declspec(dllexport)
		#else
			#define TheoraPlayerExport __declspec(dllimport)
		#endif
    #endif
#else
	#define TheoraPlayerExport __attribute__ ((visibility("default")))
#endif

#endif

