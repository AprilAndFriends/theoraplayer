/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _Obj_Model_h
#define _Obj_Model_h

#include <string>

struct ObjVertex
{
	float x,y,z,u,v;
};

class ObjModel
{
	std::string mName;
	unsigned int mTexture;
	ObjVertex *mVertices;
	int mNumVertices;
public:
	ObjModel();
	~ObjModel();

	void load(std::string filename,unsigned int texture_id);
	void draw();
};


#endif