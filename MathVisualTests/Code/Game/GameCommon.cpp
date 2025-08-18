#include "Game/GameCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB3.hpp"

Vec2 MapMouseCursorToWorldCoords2D(AABB2 const& cameraBounds, AABB2 const& viewport)
{
	Vec2 clientUV = g_theInput->GetCursorNormalizedPosition();
	Vec2 adjustUV = viewport.GetUVForPoint(clientUV);
	return cameraBounds.GetPointAtUV(adjustUV);
}

void AddVertsForSimpleLine2D(std::vector<Vertex_PCU>& verts, std::vector<Vec2> const& points, float thickness, Rgba8 const& color)
{
	int numPoints = (int)points.size();

	for (int i = 0; i < numPoints - 1; ++i)
	{
		Vec2 start = points[i];
		Vec2 end = points[i + 1];
		AddVertsForLineSegment2D(verts, start, end, thickness, color);
	}
}

void AddVertsForColoredCube3D(std::vector<Vertex_PCU>& verts)
{
	// p-positive n-negative
	Vec3 nnn(-0.5f, -0.5f, -0.5f);
	Vec3 nnp(-0.5f, -0.5f, +0.5f);
	Vec3 npn(-0.5f, +0.5f, -0.5f);
	Vec3 npp(-0.5f, +0.5f, +0.5f);
	Vec3 pnn(+0.5f, -0.5f, -0.5f);
	Vec3 pnp(+0.5f, -0.5f, +0.5f);
	Vec3 ppn(+0.5f, +0.5f, -0.5f);
	Vec3 ppp(+0.5f, +0.5f, +0.5f);

	AddVertsForQuad3D(verts, pnn, ppn, ppp, pnp, Rgba8::RED); // +x
	AddVertsForQuad3D(verts, npn, nnn, nnp, npp, Rgba8::CYAN); // -x
	AddVertsForQuad3D(verts, ppn, npn, npp, ppp, Rgba8::GREEN); // +y
	AddVertsForQuad3D(verts, nnn, pnn, pnp, nnp, Rgba8::MAGENTA); // -y
	AddVertsForQuad3D(verts, npp, nnp, pnp, ppp, Rgba8::BLUE); // +z
	AddVertsForQuad3D(verts, ppn, pnn, nnn, npn, Rgba8::YELLOW); // -z
}

void AddVertsForColoredAABB3D(std::vector<Vertex_PCU>& verts, AABB3 const& bounds)
{
	float minX = bounds.m_mins.x;
	float minY = bounds.m_mins.y;
	float minZ = bounds.m_mins.z;
	float maxX = bounds.m_maxs.x;
	float maxY = bounds.m_maxs.y;
	float maxZ = bounds.m_maxs.z;

	// p-max n-min
	Vec3 nnn(minX, minY, minZ);
	Vec3 nnp(minX, minY, maxZ);
	Vec3 npn(minX, maxY, minZ);
	Vec3 npp(minX, maxY, maxZ);
	Vec3 pnn(maxX, minY, minZ);
	Vec3 pnp(maxX, minY, maxZ);
	Vec3 ppn(maxX, maxY, minZ);
	Vec3 ppp(maxX, maxY, maxZ);

	AddVertsForQuad3D(verts, pnn, ppn, ppp, pnp, Rgba8::RED); // +x
	AddVertsForQuad3D(verts, npn, nnn, nnp, npp, Rgba8::CYAN); // -x
	AddVertsForQuad3D(verts, ppn, npn, npp, ppp, Rgba8::GREEN); // +y
	AddVertsForQuad3D(verts, nnn, pnn, pnp, nnp, Rgba8::MAGENTA); // -y
	AddVertsForQuad3D(verts, npp, nnp, pnp, ppp, Rgba8::BLUE); // +z
	AddVertsForQuad3D(verts, ppn, pnn, nnn, npn, Rgba8::YELLOW); // -z
}


void AddVertsForScratchyLines(std::vector<Vertex_PCU>& verts, std::vector<Vec3> points, float radius, Rgba8 const& color)
{
	int numPoints = (int)points.size();
	//if (numPoints < 2)
	//{
	//	return;
	//}

	for (int i = 0; i < numPoints - 1; ++i)
	{
		AddVertsForCylinder3D(verts, points[i], points[i + 1], radius, color, AABB2::ZERO_TO_ONE, 4);
	}
}
