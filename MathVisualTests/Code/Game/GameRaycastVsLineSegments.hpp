#pragma once
#include "Game/Game.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include <vector>

struct GRO_LineSegement
{
	Vec2 m_start;
	Vec2 m_end;

	GRO_LineSegement(Vec2 const& sceneDimensions);
};

//-----------------------------------------------------------------------------------------------
class GameRaycastVsLineSegments : public Game
{
public:
	GameRaycastVsLineSegments();
	~GameRaycastVsLineSegments();

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

	GRO_LineSegement* m_hitObject = nullptr;
	RaycastResult2D m_raycastResult;
	std::vector<GRO_LineSegement*> m_shapeList;
};

