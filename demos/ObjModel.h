/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************
Note that this is a very simple and featureless obj loader, it's made to work only
with the obj files used in the demos. I don't encourage using this code in your own
projects, there are much better and feature-full obj loaders out there. Not to mention
obj is an outdated format to use in real-life projects in the first place ;)
*************************************************************************************/
#ifndef _Obj_Model_h
#define _Obj_Model_h

#include <string>

struct ObjVertex
{
	float x,y,z,u,v,nx,ny,nz;
};

class ObjModel
{
	std::string mName;
	unsigned int mTexture;
	ObjVertex *mVertices;
	int mNumVertices;
	bool mNormals;
public:
	ObjModel();
	~ObjModel();

	void load(std::string filename,unsigned int texture_id, bool normals = false);
	void draw(void (*texfunc)(float, float) = NULL);
};

#endif
