/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.googlecode.com
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
*************************************************************************************/
#ifndef APRILVIDEO_EXPORT_H
#define APRILVIDEO_EXPORT_H

	#ifdef _LIB
		#define aprilVideoExport
		#define aprilVideoFnExport
	#else
		#ifdef _WIN32
			#ifdef APRILVIDEO_EXPORTS
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
