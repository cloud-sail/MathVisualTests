#pragma once
#include "Game/Game.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include <vector>

struct GRDO_Disc
{
	float m_radius;
	Vec2 m_center;

	GRDO_Disc(Vec2 const& sceneDimensions);
};

//-----------------------------------------------------------------------------------------------
class GameRaycastVsDiscs : public Game
{
public:
	GameRaycastVsDiscs();
	~GameRaycastVsDiscs();

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

	GRDO_Disc* m_hitDisc = nullptr;
	RaycastResult2D m_raycastResult;
	std::vector<GRDO_Disc*> m_shapeList;
};

