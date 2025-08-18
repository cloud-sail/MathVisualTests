#pragma once
#include "Game/Game.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Spline.hpp"


struct Game3DCurvesTransform
{
	Quat m_rotation = Quat::IDENTITY;
	Vec3 m_position;
	Vec3 m_scale = Vec3(1.f, 1.f, 1.f);
};

// ----------------------------------------------------------------------------------------------
class Game3DCurves : public Game
{
public:
	Game3DCurves();
	virtual ~Game3DCurves();
	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;

protected:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;

	void UpdatePlayer();
	void UpdatePlayerPosition(float deltaSeconds);
	void UpdatePlayerOrientation();
	void UpdatePlayerCameraCenterAxis();

private:
	void HandleInput();

	void InitializeModel();

	float GetCurrentFraction(int numSegment = 1) const;

	Vec3 GetRandomPosInsideBox(AABB3 const& box) const;
	Vec3 GetPositionInBoxCoords(AABB3 const& box, Vec3 const& point) const;
	Vec3 GetRandomVec3FromZeroToOne() const;
	void BubbleSort(std::vector<Vec3>& points);

	void DrawSplines() const;
	void DrawObjects() const;

private:
	Camera m_screenCamera;

	Camera m_camera;
	Vec3 m_playerPosition = Vec3(2.f, 2.f, 2.f);
	EulerAngles m_playerOrientation = EulerAngles(-135.f, 0.f, 0.f);

	AABB3 m_splineBoxs[3];
	Spline3D m_spline1;

	std::vector<Vertex_PCU> m_modelVerts;
	Texture* m_modelTexture = nullptr;
};