/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "demo_basecode.h"

// tga header info taken from http://www.organicbit.com/closecombat/formats/tga.html
typedef unsigned char byte;
struct TGAHEADER
{
    byte  identsize;          // size of ID field that follows 18 byte header (0 usually)
    byte  colourmaptype;      // type of colour map 0=none, 1=has palette
    byte  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed
	
    short colourmapstart;     // first colour map entry in palette
    short colourmaplength;    // number of colours in palette
    byte  colourmapbits;      // number of bits per palette entry 15,16,24,32
	
    short xstart;             // image x origin
    short ystart;             // image y origin
    short width;              // image width in pixels
    short height;             // image height in pixels
    byte  bits;               // image bits per pixel 8,16,24,32
    byte  descriptor;         // image descriptor bits (vh flip bits)
    // pixel data follows header
};

unsigned int loadTexture(const char* filename, int* outWidth, int* outHeight)
{
	FILE* file = fopen(filename, "rb");
	if (file == NULL)
	{
		return 0;
	}
	TGAHEADER header;
	fread(&header.identsize, 1, 1, file);
	fread(&header.colourmaptype, 1, 1, file);
	fread(&header.imagetype, 1, 1, file);
	fseek(file, 9, SEEK_CUR);
	fread(&header.width, 1, 2, file);
	fread(&header.height, 1, 2, file);
	fread(&header.bits, 1, 1, file);
	fread(&header.descriptor, 1, 1, file);
	if (header.identsize > 0)
	{
		fseek(file, header.identsize, SEEK_CUR);
	}
	if (outWidth != NULL)
	{
		*outWidth = header.width;
	}
	if (outHeight != NULL)
	{
		*outHeight = header.height;
	}
	int bpp = (header.bits == 24 ? 3 : 4);
	int size = header.width * header.height * bpp;
	byte* data = new byte[size];
	byte* ptr = data;
	byte temp;
	int i = 0;
	int j = 0;
	int stride = 0;
	if ((header.descriptor & 0x20) == 0) // upside down img
	{
		ptr += size - header.width * bpp;
		stride = -header.width * bpp * 2;
	}
	for (i = 0; i < header.height; ++i)
	{
		for (j = 0; j < header.width; ++j)
		{
			fread(ptr, 1, bpp, file);
			// switching pixels around so they are in RGBA format
			temp = ptr[0];
			ptr[0] = ptr[2];
			ptr[2] = temp;
			ptr += bpp;
		}
		ptr += stride;
	}
	fclose(file);
	// upload to GPU
	unsigned int textureId = 0;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, bpp == 3 ? GL_RGB : GL_RGBA, header.width, header.height, 0, bpp == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
	delete [] data;
	return textureId;
}
