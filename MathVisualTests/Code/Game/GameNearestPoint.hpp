#pragma once
#include "Game/Game.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>


//-----------------------------------------------------------------------------------------------
class GNPObject
{
public:
	virtual ~GNPObject() {};

	virtual bool IsPointInside(Vec2 const& point) const = 0;
	virtual Vec2 GetNearestPoint(Vec2 const& ref) const = 0;
	virtual void AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const = 0;
};

class GNPO_Disc : public GNPObject
{
public:
	GNPO_Disc(Vec2 const& sceneDimensions);

	virtual bool IsPointInside(Vec2 const& point) const override;
	virtual Vec2 GetNearestPoint(Vec2 const& ref) const override;
	virtual void AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const override;

	Vec2 m_discCenter;
	float m_discRadius;
};

class GNPO_Triangle : public GNPObject
{
public:
	GNPO_Triangle(Vec2 const& sceneDimensions);

	virtual bool IsPointInside(Vec2 const& point) const override;
	virtual Vec2 GetNearestPoint(Vec2 const& ref) const override;
	virtual void AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const override;

	Triangle2 m_triangle;
};

class GNPO_AABB2 : public GNPObject
{
public:
	GNPO_AABB2(Vec2 const& sceneDimensions);

	virtual bool IsPointInside(Vec2 const& point) const override;
	virtual Vec2 GetNearestPoint(Vec2 const& ref) const override;
	virtual void AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const override;

	AABB2 m_box;
};

class GNPO_OBB2 : public GNPObject
{
public:
	GNPO_OBB2(Vec2 const& sceneDimensions);

	virtual bool IsPointInside(Vec2 const& point) const override;
	virtual Vec2 GetNearestPoint(Vec2 const& ref) const override;
	virtual void AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const override;

	OBB2 m_obb;
};

class GNPO_Capsule2 : public GNPObject
{
public:
	GNPO_Capsule2(Vec2 const& sceneDimensions);

	virtual bool IsPointInside(Vec2 const& point) const override;
	virtual Vec2 GetNearestPoint(Vec2 const& ref) const override;
	virtual void AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const override;

	Capsule2 m_capsule;
};

class GNPO_LineSegment2 : public GNPObject
{
public:
	GNPO_LineSegment2(Vec2 const& sceneDimensions);

	virtual bool IsPointInside(Vec2 const& point) const override;
	virtual Vec2 GetNearestPoint(Vec2 const& ref) const override;
	virtual void AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const override;

	LineSegment2 m_lineSegment;
};

class GNPO_InfiniteLine : public GNPObject
{
public:
	GNPO_InfiniteLine(Vec2 const& sceneDimensions);

	virtual bool IsPointInside(Vec2 const& point) const override;
	virtual Vec2 GetNearestPoint(Vec2 const& ref) const override;
	virtual void AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const override;

	LineSegment2 m_infiniteLine;
};

//-----------------------------------------------------------------------------------------------

class GameNearestPoint : public Game
{
public:
	GameNearestPoint();
	virtual ~GameNearestPoint();

	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;

private:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;

	void DrawObjects() const;

private:
	Camera m_camera;

	std::vector<GNPObject*> m_shapeList;
	Vec2 m_whiteDotPos;
};

