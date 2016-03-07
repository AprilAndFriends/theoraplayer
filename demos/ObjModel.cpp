/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

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

#include "ObjModel.h"

struct ObjFace
{
	int v;
	int t;
	int n;
};

ObjModel::ObjModel()
{
	this->textureId = 0;
	this->vertices = NULL;
	this->verticesCount = 0;
	this->normals = false;
}

ObjModel::~ObjModel()
{
	this->unload();
}

void ObjModel::load(std::string filename, unsigned int textureId, bool normals)
{
	this->unload();
	this->textureId = textureId;
	this->name = filename;
	this->normals = normals;
	std::vector<ObjVertex> v;
	std::vector<ObjVertex> t;
	std::vector<ObjVertex> vn;
	std::vector<ObjFace> faces;
	ObjVertex temp;
	char line[512] = { 0 };
	ObjFace faceData[4];
	memset(faceData, 0, sizeof(faceData));
	FILE* file = fopen(filename.c_str(), "r");
	int slashCount = 0;
	char* ptr = NULL;
	while (fgets(line, 512, file))
	{
		if (strncmp(line, "v  ", 3) == 0)
		{
			sscanf(line + 3, "%f %f %f", &temp.x, &temp.y, &temp.z);
			v.push_back(temp);
		}
		if (strncmp(line, "vn ", 3) == 0)
		{
			sscanf(line + 3, "%f %f %f", &temp.nx, &temp.ny, &temp.nz);
			vn.push_back(temp);
		}
		else if (strncmp(line, "vt ", 3) == 0)
		{
			sscanf(line + 3, "%f %f", &temp.u, &temp.v);
			t.push_back(temp);
		}
		else if (strncmp(line, "f ", 2) == 0)
		{
			slashCount = 0;
			ptr = line - 1;
			while ((ptr = strstr(ptr + 1, "/")))
			{
				slashCount++;
			}
			if (slashCount == 4)
			{
				sscanf(line + 2, "%d/%d %d/%d %d/%d %d/%d",
					&faceData[0].v, &faceData[0].t,
					&faceData[1].v, &faceData[1].t,
					&faceData[2].v, &faceData[2].t,
					&faceData[3].v, &faceData[3].t);
				faces.push_back(faceData[0]);
				faces.push_back(faceData[1]);
				faces.push_back(faceData[2]);
				faces.push_back(faceData[0]);
				faces.push_back(faceData[2]);
				faces.push_back(faceData[3]);
			}
			else if (slashCount == 8)
			{
				sscanf(line + 2, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
					&faceData[0].v, &faceData[0].t, &faceData[0].n,
					&faceData[1].v, &faceData[1].t, &faceData[1].n,
					&faceData[2].v, &faceData[2].t, &faceData[2].n,
					&faceData[3].v, &faceData[3].t, &faceData[3].n);
				faces.push_back(faceData[0]);
				faces.push_back(faceData[1]);
				faces.push_back(faceData[2]);
				faces.push_back(faceData[0]);
				faces.push_back(faceData[2]);
				faces.push_back(faceData[3]);
			}
			else if (slashCount == 3)
			{
				sscanf(line + 2, "%d/%d %d/%d %d/%d",
					&faceData[0].v, &faceData[0].t,
					&faceData[1].v, &faceData[1].t,
					&faceData[2].v, &faceData[2].t);
				faces.push_back(faceData[0]);
				faces.push_back(faceData[1]);
				faces.push_back(faceData[2]);
			}
			else
			{
				sscanf(line + 2, "%d/%d/%d %d/%d/%d %d/%d/%d",
					&faceData[0].v, &faceData[0].t, &faceData[0].n,
					&faceData[1].v, &faceData[1].t, &faceData[1].n,
					&faceData[2].v, &faceData[2].t, &faceData[2].n);
				faces.push_back(faceData[0]);
				faces.push_back(faceData[1]);
				faces.push_back(faceData[2]);
			}
		}
	}
	fclose(file);
	int _v = 0;
	int _t = 0;
	int _n = 0;
	this->vertices = new ObjVertex[faces.size()];
	for (std::vector<ObjFace>::iterator it = faces.begin(); it != faces.end(); it++)
	{
		_v = it->v > 0 ? it->v - 1 : (int)(v.size() + it->v);
		_t = it->t > 0 ? it->t - 1 : (int)(t.size() + it->t);
		if (normals)
		{
			_n = it->n > 0 ? it->n - 1 : (int)(vn.size() + it->n);
		}
		this->vertices[this->verticesCount].x = v[_v].x;
		this->vertices[this->verticesCount].y = v[_v].y;
		this->vertices[this->verticesCount].z = v[_v].z;
		if (normals)
		{
			this->vertices[this->verticesCount].nx = vn[_n].nx;
			this->vertices[this->verticesCount].ny = vn[_n].ny;
			this->vertices[this->verticesCount].nz = vn[_n].nz;
		}
		this->vertices[this->verticesCount].u = t[_t].u;
		this->vertices[this->verticesCount].v = 1 - t[_t].v;
		++this->verticesCount;
	}
}

void ObjModel::unload()
{
	if (this->vertices != NULL)
	{
		this->textureId = 0;
		delete[] this->vertices;
		this->vertices = NULL;
		this->verticesCount = 0;
	}
}

void ObjModel::draw(void (*textureFunction)(float, float))
{
	if (this->textureId == 0)
	{
		return;
	}
	glBindTexture(GL_TEXTURE_2D, this->textureId);
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < this->verticesCount; i++)
	{
		if (textureFunction != NULL)
		{
			(*textureFunction)(this->vertices[i].u, this->vertices[i].v);
		}
		else
		{
			glTexCoord2f(this->vertices[i].u, this->vertices[i].v);
		}
		if (this->normals)
		{
			glNormal3f(this->vertices[i].nx, this->vertices[i].ny, this->vertices[i].nz);
		}
		glVertex3f(this->vertices[i].x, this->vertices[i].y, this->vertices[i].z);
	}
	glEnd();
}
