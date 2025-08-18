#pragma once

#include "Game/Game.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/RaycastUtils.hpp"

class TileHeatMap;


class FlowFieldActor2D
{
public:
	Vec2 m_position; // look up velocity from flow field and apply += speed * direction * deltaSeconds
	float m_speed = 0.f; // need to randomize the speed for each actor
	float m_orientationDegrees = 0.f; // Only For Rendering, need to update by flow field
};




class Game2DFlowField : public Game
{
public:
	Game2DFlowField();
	virtual ~Game2DFlowField();
	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;

protected:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;

private:
	void HandleInput();

	void RecreateSolidMap();
	void RecreateDistanceMapAndFlowField();

	void SpawnActor();
	void UpdateActors();
	void TeleportActors();

	void DrawDistanceMap() const;
	void DrawFlowField() const;
	void DrawSolidMap() const;
	void DrawTileGrid() const;
	void DrawStartsAndEnds() const;
	void DrawActors() const;

	bool IsTileSolid(int tileX, int tileY) const;
	bool IsWorldPosInBounds(Vec2 const& worldPos) const;
	IntVec2 GetTileCoordsForWorldPos(Vec2 const& worldPos) const;
	Vec2 GetTileCenter(int tileX, int tileY) const;

	void SpreadDistanceMapHeat(TileHeatMap& distanceMap, float startSearchValue, float heatSpreadStep = 1.f);
	Vec2 GetBilinearInterpResultFromWorldPos(Vec2 const& worldPos) const;
	Vec2 GetSafeValueFromFlowField(IntVec2 tileCoords) const;


private:
	Camera m_camera;

	TileHeatMap* m_solidMap = nullptr;
	TileHeatMap* m_distanceMap = nullptr;
	TileVectorField* m_flowField = nullptr;

	IntVec2		m_gridDimensions	= IntVec2(50, 25);
	Vec2		m_cellSize			= Vec2(28.f, 28.f);
	Vec2		m_gridOrigin		= Vec2(100.f, 50.f);


	std::vector<IntVec2> m_starts; // LMB
	std::vector<IntVec2> m_ends; // RMB
	float m_startRadius = 100.f;
	float m_endRadius = 100.f;

	std::vector<FlowFieldActor2D> m_actors;


	// RMB click will regenerate the heat map and flow field
};
