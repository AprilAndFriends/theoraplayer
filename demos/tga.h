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
/// This file contains simple uncompressed OpenGL TGA texture file loading code.
/// The reason TGA format was choosed for libtheoraplayer's demos is because it's a simple
/// and portable format.

#ifndef THEORAYPLAYER_DEMOS_TGA_H
#define THEORAYPLAYER_DEMOS_TGA_H

unsigned int loadTexture(const char* filename, int* outWidth = NULL, int* outHeight = NULL);

#endif
