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
			#define DEPRECATED_ATTRIBUTE(message) __declspec(deprecated(message))
		#elif !defined(__APPLE__) || __has_extension(attribute_deprecated_with_message) || (defined(__GNUC) && ((GNUC >= 5) || ((GNUC == 4) && (GNUC_MINOR__ >= 5))))
			#define DEPRECATED_ATTRIBUTE(message) __attribute__((deprecated(message)))
		#else
			#define DEPRECATED_ATTRIBUTE(message) __attribute__((deprecated))
		#endif
	#endif

#endif
