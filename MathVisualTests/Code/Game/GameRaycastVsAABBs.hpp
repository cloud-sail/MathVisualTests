#pragma once
#include "Game/Game.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include <vector>

struct GRO_AABB
{
	Vec2 m_mins;
	Vec2 m_maxs;

	GRO_AABB(Vec2 const& sceneDimensions);
};

//-----------------------------------------------------------------------------------------------
class GameRaycastVsAABBs : public Game
{
public:
	GameRaycastVsAABBs();
	~GameRaycastVsAABBs();

	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;


private:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;
	
	void HandleInput();
	void DoRaycast();

	void DrawObjects() const;
private:
	Camera m_camera;
	Vec2 m_startPos;
	Vec2 m_endPos;

	GRO_AABB* m_hitObject = nullptr;
	RaycastResult2D m_raycastResult;
	std::vector<GRO_AABB*> m_shapeList;
};

