/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2013 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef APRILVIDEO_EXPORT_H
#define APRILVIDEO_EXPORT_H

	#ifdef _LIB
		#define aprilVideoExport
		#define aprilVideoFnExport
	#else
		#ifdef _WIN32
			#ifdef APRILUI_EXPORTS
				#define aprilVideoExport __declspec(dllexport)
				#define aprilVideoFnExport __declspec(dllexport)
			#else
				#define aprilVideoExport __declspec(dllimport)
				#define aprilVideoFnExport __declspec(dllimport)
			#endif
		#else
			#define aprilVideoExport __attribute__ ((visibility("default")))
			#define aprilVideoFnExport __attribute__ ((visibility("default")))
		#endif
	#endif
	#ifndef DEPRECATED_ATTRIBUTE
		#ifdef _MSC_VER
			#define DEPRECATED_ATTRIBUTE __declspec(deprecated("function is deprecated"))
		#else
			#define DEPRECATED_ATTRIBUTE __attribute__((deprecated))
		#endif
	#endif

#endif
