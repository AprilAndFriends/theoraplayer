/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2014 Kresimir Spes (kspes@cateia.com)
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
	int v,t,n;
};

ObjModel::ObjModel()
{
	mNumVertices=0; mVertices=0;
}

void ObjModel::load(std::string filename,unsigned int texture_id, bool normals)
{
	mNormals = normals;
	mName=filename;
	mTexture=texture_id;

	std::vector<ObjVertex> v,t,vn;
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
		if (strncmp(line,"vn ",3) == 0)
		{
			sscanf(line+3,"%f %f %f",&temp.nx,&temp.ny,&temp.nz);
			vn.push_back(temp);
		}
		else if (strncmp(line,"vt ",3) == 0)
		{
			sscanf(line+3,"%f %f",&temp.u,&temp.v);
			t.push_back(temp);
		}
		else if (strncmp(line,"f ",2) == 0)
		{
			ObjFace f[4];
			int nslash = 0;
			char* ptr = line - 1;
			while ((ptr = strstr(ptr + 1, "/"))) nslash++;
			if (nslash == 4)
			{
				sscanf(line+2,"%d/%d %d/%d %d/%d %d/%d",
					   &f[0].v,&f[0].t,
					   &f[1].v,&f[1].t,
					   &f[2].v,&f[2].t,
					   &f[3].v,&f[3].t);
				faces.push_back(f[0]);
				faces.push_back(f[1]);
				faces.push_back(f[2]);
				faces.push_back(f[0]);
				faces.push_back(f[2]);
				faces.push_back(f[3]);
			}
			else if (nslash == 8)
			{
				sscanf(line+2,"%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
					   &f[0].v,&f[0].t,&f[0].n,
					   &f[1].v,&f[1].t,&f[1].n,
					   &f[2].v,&f[2].t,&f[2].n,
					   &f[3].v,&f[3].t,&f[3].n);
				faces.push_back(f[0]);
				faces.push_back(f[1]);
				faces.push_back(f[2]);
				faces.push_back(f[0]);
				faces.push_back(f[2]);
				faces.push_back(f[3]);
			}
			else if (nslash == 3)
			{
				sscanf(line+2,"%d/%d %d/%d %d/%d",
					   &f[0].v,&f[0].t,
					   &f[1].v,&f[1].t,
					   &f[2].v,&f[2].t);
				
				faces.push_back(f[0]);
				faces.push_back(f[1]);
				faces.push_back(f[2]);
			}
			else
			{
				sscanf(line+2,"%d/%d/%d %d/%d/%d %d/%d/%d",
					   &f[0].v,&f[0].t,&f[0].n,
					   &f[1].v,&f[1].t,&f[1].n,
					   &f[2].v,&f[2].t,&f[2].n);

				faces.push_back(f[0]);
				faces.push_back(f[1]);
				faces.push_back(f[2]);
			}
		}
	}

	fclose(f);

	int _v, _t, _n;
	mVertices=new ObjVertex[faces.size()];
	for (std::vector<ObjFace>::iterator it=faces.begin();it!=faces.end();it++)
	{
		_v = it->v > 0 ? it->v - 1 : (int)(v.size() + it->v);
		_t = it->t > 0 ? it->t - 1 : (int)(t.size() + it->t);
		if (normals) _n = it->n > 0 ? it->n - 1 : (int)(vn.size() + it->n);
		mVertices[mNumVertices].x=v[_v].x;
		mVertices[mNumVertices].y=v[_v].y;
		mVertices[mNumVertices].z=v[_v].z;
		if (normals)
		{
			mVertices[mNumVertices].nx=vn[_n].nx;
			mVertices[mNumVertices].ny=vn[_n].ny;
			mVertices[mNumVertices].nz=vn[_n].nz;		
		}
		mVertices[mNumVertices].u=t[_t].u;
		mVertices[mNumVertices].v=1-t[_t].v;
		mNumVertices++;
	}
}

ObjModel::~ObjModel()
{
	if (!mVertices) delete [] mVertices;
}

void ObjModel::draw(void (*texfunc)(float, float))
{
	glBindTexture(GL_TEXTURE_2D,mTexture);
	glBegin(GL_TRIANGLES);

	for (int i=0;i<mNumVertices;i++)
	{
		if (texfunc) texfunc(mVertices[i].u,mVertices[i].v);
		else glTexCoord2f(mVertices[i].u,mVertices[i].v);

		if (mNormals) glNormal3f(mVertices[i].nx,mVertices[i].ny,mVertices[i].nz);
		glVertex3f(mVertices[i].x,mVertices[i].y,mVertices[i].z);
	}
	glEnd();
}
