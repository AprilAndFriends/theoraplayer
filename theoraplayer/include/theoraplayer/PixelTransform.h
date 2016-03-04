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
/// Defines a utility struct for pixel transformation.

#ifndef THEORA_PIXEL_TRANSFORM_H
#define THEORA_PIXEL_TRANSFORM_H

// TODOth - move to Utility as internal object?

struct PixelTransform
{
	unsigned char *raw, *y, *u, *v, *out;
	unsigned int w, h, rawStride, yStride, uStride, vStride;
};

#endif
