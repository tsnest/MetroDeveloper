#include <stdio.h>
#include "model.hpp"

extern "C" {
	#include "mfile.h"
};

struct T4AHeader
{
	char version;
	char modeltype;
	short shaderid;
	float bbox[6];
	float bsphere[4];
	int checksum;
	char extra[16];
};

struct T4AVertStatic
{
	Vec3f point;
	int normal, tangent, binormal;
	float u, v;
};

bool FaceVertexMeshImpl::load_raw(const char *filename)
{
	int version, count;
	FILE *f;

	f = fopen(filename, "rb");
	if(!f)
		return false;

	fread(&version, 1, 4, f);
	if(version != 1)
	{
		fclose(f);
		return false;
	}

	fread(&count, 1, 4, f);
	m_points.resize(count);
	fread(&m_points.front(), sizeof(Vec3f), count, f);

	fread(&count, 1, 4, f);
	m_faces.resize(count);
	for(int i = 0; i < count; i++)
	{
		fread(&m_faces[i].v1, 1, 4, f);
		fread(&m_faces[i].v2, 1, 4, f);
		fread(&m_faces[i].v3, 1, 4, f);
		m_faces[i].surfaceType = 1;
		m_faces[i].userData = -1;
	}

	fclose(f);
	return true;
}

bool FaceVertexMeshImpl::load_4a(const char *filename)
{
  FILE *f = fopen(filename, "rb");
  if(!f)
    return false;
    
  fseek(f, 0, SEEK_END);
  size_t length = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  void *buffer = malloc(length);
  if(!buffer)
  {
  	fclose(f);
  	return false;
  }
  
  fread(buffer, 1, length, f);
  fclose(f);
  
	MFILE *mf = mfopen(buffer, length);
	
		MFILE *child = mfopenchunk(mf, 9);
		
		if(!child)
		{
			mfclose(mf);
			free(buffer);
			return false;
		}
		
		int i = 0;
		while(true)
		{
			MFILE *mesh = mfopenchunk(child, i++);
			if(!mesh)
				break;
				
			// read points
			MFILE *vertices = mfopenchunk(mesh, 3);
			if(!vertices)
			{
				mfclose(mesh);
				break;
			}
			
			int vertexformat = mfread_int(vertices);
			int vertexcount = mfread_int(vertices);
			T4AVertStatic *pVerts = (T4AVertStatic*)((char*)vertices->buffer + vertices->cursor);
			
			mfclose(vertices);
			
			// read trinagles
			int trianglebase = m_faces.size();
			MFILE *indices = mfopenchunk(mesh, 4);
			if(!indices)
			{
				mfclose(mesh);
				break;
			}
			
			int trianglecount = mfread_int(indices) / 3;
			m_faces.resize(m_faces.size() + trianglecount);
			
			for(int k = 0; k < trianglecount; k++)
			{
				m_faces[trianglebase+k].v1 = addPoint( pVerts[mfread_short(indices)].point );
				m_faces[trianglebase+k].v2 = addPoint( pVerts[mfread_short(indices)].point );
				m_faces[trianglebase+k].v3 = addPoint( pVerts[mfread_short(indices)].point );
				m_faces[trianglebase+k].surfaceType = 1;
				m_faces[trianglebase+k].userData = -1;
			}
			
			mfclose(indices);
			
			mfclose(mesh);
		}
		
		mfclose(child);
	
	mfclose(mf);
	free(buffer);
  
	return m_points.size() > 0 && m_faces.size() > 0;
}

int FaceVertexMeshImpl::addPoint(Vec3f &p)
{
	for(int i = 0; i < m_points.size(); i++)
		if(m_points[i].x == p.x && m_points[i].y == p.y && m_points[i].z == p.z)
			return i;
			
	m_points.push_back(p);
	return m_points.size()-1;
}
