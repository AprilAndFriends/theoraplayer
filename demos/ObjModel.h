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
/// Demonstrates how to render environments using videos.
/// Note that this is a very simple and featureless obj loader, it's made to work only
/// with the obj files used in the demos. I don't encourage using this code in your own
/// projects, there are much better and feature-full obj loaders out there. Not to mention
/// obj is an outdated format to use in real-life projects in the first place.

#ifndef THEORAPLAYER_DEMOS_OBJ_MODEL_H
#define THEORAPLAYER_DEMOS_OBJ_MODEL_H

#include <string>

struct ObjVertex
{
	float x;
	float y;
	float z;
	float u;
	float v;
	float nx;
	float ny;
	float nz;
};

class ObjModel
{
public:
	ObjModel();
	~ObjModel();

	void load(std::string filename, unsigned int textureId, bool normals = false);
	void unload();
	void draw(void (*textureFunction)(float, float) = NULL);

protected:
	std::string name;
	unsigned int textureId;
	ObjVertex* vertices;
	int verticesCount;
	bool normals;

};

#endif
