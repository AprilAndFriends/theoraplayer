/// @file
/// @version 1.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines macros for DLL exports/imports.

#ifndef CLIPWEBM_EXPORT_H
#define CLIPWEBM_EXPORT_H

	#ifdef _LIB
		#define clipwebmExport
		#define clipwebmFnExport
	#else
		#ifdef _WIN32
			#ifdef CLIPWEBM_EXPORTS
				#define clipwebmExport __declspec(dllexport)
				#define clipwebmFnExport __declspec(dllexport)
			#else
				#define clipwebmExport __declspec(dllimport)
				#define clipwebmFnExport __declspec(dllimport)
			#endif
		#else
			#define clipwebmExport __attribute__ ((visibility("default")))
			#define clipwebmFnExport __attribute__ ((visibility("default")))
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

