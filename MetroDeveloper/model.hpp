#ifndef __MODEL_HPP__
#define __MODEL_HPP__

#include <vector>
#include "i_pathengine.h"

struct Vec3f
{
	float x, y, z;
};

struct Face
{
	int v1, v2, v3;
	int userData;
	int surfaceType;
};

class FaceVertexMeshImpl : public iFaceVertexMesh
{
	public:
	std::vector<Vec3f> m_points;
	std::vector<Face> m_faces;

	virtual tSigned32 faces() const { return m_faces.size(); };
	virtual tSigned32 vertices() const { return m_points.size(); };
	virtual tSigned32 vertexIndex(tSigned32 face, tSigned32 vertexInFace) const
	{
		//printf("vertexIndex face=%d vertexInFace=%d\n", face, vertexInFace);
		switch(vertexInFace)
		{
			case 0:		return m_faces[face].v1;
			case 1:		return m_faces[face].v2;
			case 2:		return m_faces[face].v3;
			default:	return -1;
		}
	};
	virtual tSigned32 vertexX(tSigned32 v) const { return tSigned32(m_points[v].x * 1000); };
	virtual tSigned32 vertexY(tSigned32 v) const { return tSigned32(m_points[v].z * 1000); };
	virtual float vertexZ(tSigned32 v) const { return m_points[v].y * 1000; };
	virtual tSigned32 faceAttribute(tSigned32 face, tSigned32 attributeIndex) const
	{
		switch(attributeIndex)
		{
			case PE_FaceAttribute_SurfaceType:		return m_faces[face].surfaceType;
			case PE_FaceAttribute_UserData:			return m_faces[face].userData;
			default:								return -1;
		}
	};

	bool load_raw(const char *filename);
	bool load_4a(const char *filename);
	int addPoint(Vec3f &p);
};

#endif