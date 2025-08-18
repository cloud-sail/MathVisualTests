#pragma once

#include "Game/Game.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/RaycastUtils.hpp"

class TileHeatMap;

// TODO: After have job or multithread system, put raycast things on different threads
// ----------------------------------------------------------------------------------------------
class Game2DExposureAvoidance : public Game
{
public:
	Game2DExposureAvoidance();
	virtual ~Game2DExposureAvoidance();
	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;

protected:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;

private:
	void HandleInput();

	void UpdateExposureMap();


	void DrawExposureMap() const;
	void DrawSolidMap() const;
	void DrawTileGrid() const;
	void DrawSentinels() const;

	RaycastResult2D FastVoxelRaycast(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength) const;

	bool IsTileSolid(int tileX, int tileY) const;
	IntVec2 GetTileCoordsForWorldPos(Vec2 const& worldPos) const;
	Vec2 GetTileCenter(int tileX, int tileY) const;

	void SpreadDistanceMapHeat(TileHeatMap& distanceMap, float startSearchValue, float heatSpreadStep = 1.f);

private:
	Camera m_camera;

	TileHeatMap* m_solidMap = nullptr;
	TileHeatMap m_exposureMap = TileHeatMap(IntVec2(), 0.f);

	IntVec2		m_gridDimensions	= IntVec2(50, 25);
	Vec2		m_cellSize			= Vec2(28.f, 28.f);
	Vec2		m_gridOrigin		= Vec2(100.f, 50.f);


	//Vec2 m_playerPos = Vec2(800.f, 400.f);
	// click to add a sentinel
	// click to remove a sentinel

	std::vector<Vec2> m_sentinels;
	float m_clickRadius = 8.f;
	float m_sightRange = 25.f * 10.f;
};
