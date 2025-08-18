#pragma once

#include "Game/Game.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/RaycastUtils.hpp"

class TileHeatMap;

// ----------------------------------------------------------------------------------------------
class Game2DFastVoxelRaycast : public Game
{
public:
	Game2DFastVoxelRaycast();
	virtual ~Game2DFastVoxelRaycast();
	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;

protected:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;

private:
	void HandleInput();

	void DrawSolidMap() const;
	void DrawRaycastResult() const;

	RaycastResult2D FastVoxelRaycast(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength) const;

	bool IsTileSolid(int tileX, int tileY) const;
	IntVec2 GetTileCoordsForWorldPos(Vec2 const& worldPos) const;

private:
	Camera m_camera;

	TileHeatMap* m_solidMap = nullptr;

	IntVec2		m_gridDimensions	= IntVec2(30, 20);
	Vec2		m_cellSize			= Vec2(40.f, 30.f);
	Vec2		m_gridOrigin		= Vec2(100.f, 100.f);

	Vec2 m_rayStartPos;
	Vec2 m_rayEndPos;

};
