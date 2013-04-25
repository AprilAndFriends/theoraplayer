/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef APRILVIDEO_EXPORT_H
/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#define APRILVIDEO_EXPORT_H

#ifdef _WIN32
	#ifdef APRILVIDEO_STATIC
		#define AprilVideoExport
	#else
		#ifdef APRILVIDEO_EXPORTS
			#define AprilVideoExport __declspec(dllexport)
		#else
			#define AprilVideoExport __declspec(dllimport)
		#endif
    #endif
#else
	#define AprilVideoExport __attribute__ ((visibility("default")))
#endif

#endif
