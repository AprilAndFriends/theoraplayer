/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines macros for DLL exports/imports.

#ifndef APRILVIDEO_EXPORT_H
#define APRILVIDEO_EXPORT_H

	#ifdef _LIB
		#define aprilvideoExport
		#define aprilvideoFnExport
	#else
		#ifdef _WIN32
			#ifdef APRILVIDEO_EXPORTS
				#define aprilvideoExport __declspec(dllexport)
				#define aprilvideoFnExport __declspec(dllexport)
			#else
				#define aprilvideoExport __declspec(dllimport)
				#define aprilvideoFnExport __declspec(dllimport)
			#endif
		#else
			#define aprilvideoExport __attribute__ ((visibility("default")))
			#define aprilvideoFnExport __attribute__ ((visibility("default")))
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
