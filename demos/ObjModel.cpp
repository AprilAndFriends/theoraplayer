/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "ObjModel.h"
#include <stdio.h>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
#include <string.h>
#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

struct ObjFace
{
	int v,t;
};

ObjModel::ObjModel()
{
	mNumVertices=0; mVertices=0;
}

void ObjModel::load(std::string filename,unsigned int texture_id)
{
	mName=filename;
	mTexture=texture_id;

	std::vector<ObjVertex> v,t;
	std::vector<ObjFace> faces;
	ObjVertex temp;

	char line[512];

	FILE* f=fopen(filename.c_str(),"r");

	while (fgets(line,512,f))
	{
		if (strncmp(line,"v  ",3) == 0)
		{
			sscanf(line+3,"%f %f %f",&temp.x,&temp.y,&temp.z);
			v.push_back(temp);
		}
		else if (strncmp(line,"vt ",3) == 0)
		{
			sscanf(line+3,"%f %f",&temp.u,&temp.v);
			t.push_back(temp);
		}
		else if (strncmp(line,"f ",2) == 0)
		{
			int n[4];
			ObjFace f[4];
			sscanf(line+2,"%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
				&f[0].v,&f[0].t,&n[0],
				&f[1].v,&f[1].t,&n[1],
				&f[2].v,&f[2].t,&n[2],
				&f[3].v,&f[3].t,&n[3]);
			faces.push_back(f[0]);
			faces.push_back(f[1]);
			faces.push_back(f[2]);
			faces.push_back(f[3]);
		}
	}

	fclose(f);

	mVertices=new ObjVertex[faces.size()];
	for (std::vector<ObjFace>::iterator it=faces.begin();it!=faces.end();it++)
	{
		mVertices[mNumVertices].x=v[it->v-1].x;
		mVertices[mNumVertices].y=v[it->v-1].y;
		mVertices[mNumVertices].z=v[it->v-1].z;
		mVertices[mNumVertices].u=t[it->t-1].u;
		mVertices[mNumVertices].v=1-t[it->t-1].v;
		mNumVertices++;
	}
}

ObjModel::~ObjModel()
{
	if (!mVertices) delete [] mVertices;
}

void ObjModel::draw()
{
	glBindTexture(GL_TEXTURE_2D,mTexture);
	glBegin(GL_QUADS);

	for (int i=0;i<mNumVertices;i++)
	{
		glTexCoord2f(mVertices[i].u,mVertices[i].v);
		glVertex3f(mVertices[i].x,mVertices[i].y,mVertices[i].z);
	}
	glEnd();
}
