#pragma once
#include "Game/Game.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Plane3.hpp"
#include <vector>

//struct GRO_AABB
//{
//	Vec2 m_mins;
//	Vec2 m_maxs;
//
//	GRO_AABB(Vec2 const& sceneDimensions);
//};
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
class TestShape
{
public:
	enum Type
	{
		eType_Sphere,
		eType_AABB3,
		eType_ZCylinder,
		eType_OBB3,
		eType_Plane3,
		eType_Count
	};

	TestShape(Vec3 const& sceneDimensions, Type shapeType, bool isWireFrame);
	void SetSphereType(float radius);
	void SetAABB3Type(Vec3 halfDimensions);
	void SetZCylinderType(float radius, float halfHeight);
	void SetOBB3Type(Vec3 halfDimensions, EulerAngles orientation);
	void SetPlane3Type(Vec3 normal, float d);

	Mat44 GetModelToWorldTransform() const;

	void Render() const;

	//-----------------------------------------------------------------------------------------------
	Vec3 GetNearestPoint(Vec3 const& point);
	bool IsOverlappingWithOtherShape(TestShape const& other);
	RaycastResult3D GetRaycastResult(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength);

public:
	Type m_type = eType_Count;
	bool m_isWireFrame = false;
	Vec3 m_position;
	//Rgba8 m_color = Rgba8::OPAQUE_WHITE;
	Texture* m_texture = nullptr;
	std::vector<Vertex_PCU> m_vertexes;

	float m_sphereRadius = 0.f; // Sphere
	Vec3 m_boxHalfDimensions; // AABB OBB
	float m_cylinderRadius = 0.f; // Cylinder
	float m_cylinderHalfHeight = 0.f; // Cylinder
	EulerAngles m_obbOrientation; // OBB
	Plane3 m_plane;

public:
	bool m_isGrabbed = false;
	bool m_isSelected = false;
	bool m_isOverlapping = false;
};



//-----------------------------------------------------------------------------------------------
class Game3DTestShapes : public Game
{
public:
	Game3DTestShapes();
	~Game3DTestShapes();

	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;


private:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;
	
	void UpdatePlayer();
	void UpdatePlayerPosition(float deltaSeconds);
	void UpdatePlayerOrientation();
	void UpdatePlayerCameraCenterAxis();

	void UpdateRay();
	void DoRaycast();

	void UpdateGrabAndLock();
	void TryToGrabObject();
	void ReleaseGrabbedObject();
	void UpdateGrabbedOBB();

	void CheckIfOverlapping();

	void DrawObjects() const;
	void DrawRaycastResult() const;
private:
	Camera m_screenCamera;

	Camera m_camera;
	Vec3 m_playerPosition = Vec3(2.f, 2.f, 2.f);
	EulerAngles m_playerOrientation = EulerAngles(-135.f, 0.f, 0.f);

	std::vector<TestShape*> m_shapeList;
	TestShape* m_grabbedObject = nullptr; // Grabbed or Released
	Vec3 m_grabbedObjectCameraSpacePosition;
	bool m_isRaycastLocked = false;


	TestShape* m_hitObject = nullptr; // highlighted
	RaycastResult3D m_raycastResult;
	Vec3	m_rayStart; // the start position of the raycast & nearest point ref point
	Vec3	m_rayFwdNormal;
	float	m_rayLength = 5.f;


	//Vec2 m_startPos;
	//Vec2 m_endPos;

	//GRO_AABB* m_hitObject = nullptr;
	//RaycastResult2D m_raycastResult;
	//std::vector<GRO_AABB*> m_shapeList;
};

