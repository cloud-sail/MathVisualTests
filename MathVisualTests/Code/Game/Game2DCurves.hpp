#pragma once
#include "Game/Game.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/Spline.hpp"
#include <vector>

//-----------------------------------------------------------------------------------------------
typedef float (EasingFunction1D)(float t);

struct EasingFunctionEntry
{
	EasingFunction1D* m_func = nullptr;
	std::string m_name;
};

constexpr int DEFAULT_SUBDIVISION = 64;
constexpr float DEFAULT_STEP = 1.f / static_cast<float>(DEFAULT_SUBDIVISION);
//-----------------------------------------------------------------------------------------------
class Game2DCurves : public Game
{
public:
	Game2DCurves();
	~Game2DCurves();

	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;


private:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;
	
	void HandleInput();


	void DrawLayout() const;
	void DrawEasingFunction() const;
	void DrawCubicCurve() const;
	void DrawCubicSpline() const;
	void DrawObjects() const;

	float GetCurrentFraction(int numSegment = 1) const;

	Vec2 GetRandomUVs();

	void RefreshCurves();

	void BubbleSortVec2X(std::vector<Vec2>& points);
private:
	Camera m_camera;

	int m_numSubdivisions = DEFAULT_SUBDIVISION;

	bool m_isLayoutHighlighted = false;
	AABB2 m_topLeftQuarterPane;
	AABB2 m_topRightQuarterPane;
	AABB2 m_bottomHalfPane;


	std::vector<EasingFunctionEntry> m_easingFunctions;
	int m_currentEasingIndex = 0;
	// t game clock time %n

	Spline2D m_cubicCurve;
	Spline2D m_linearCubicCurve;
	Vec2 m_cubicCurvePoints[4];

	Spline2D m_cubicSpline;
	Spline2D m_linearCubicSpline;
	std::vector<Vec2> m_cubicSplinePoints;
};

