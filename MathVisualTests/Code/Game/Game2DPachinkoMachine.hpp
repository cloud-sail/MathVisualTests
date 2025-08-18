#pragma once
#include "Game/Game.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Renderer/Camera.hpp"

namespace PachinkoMachine
{
	constexpr float EXTRA_WRAP_HEIGHT = 300.f;

	enum class BumperType : unsigned char
	{
		DISC,
		CAPSULE,
		OBB,
		COUNT,
	};

	class Ball
	{
	public:
		Vec2 m_center;
		Vec2 m_velocity;
		float m_radius = 0.f;
		Rgba8 m_color;
	};

	class Bumper
	{
	public:
		Vec2 m_center; // disc, OBB, capsule
		Vec2 m_capsuleHalfOffset; // Capsule
		Vec2 m_halfDimensions; // OBB
		Vec2 m_iBasisNormal; // OBB
		float m_radius = 0.f; // disc, capsule

		float m_boundingRadius = 0.f;
		float m_elasticity = 0.f;
		BumperType m_type = BumperType::COUNT;

	public:
		void BounceBallOffOf(Ball& ball, float ballElasticity);
	};


}

// ----------------------------------------------------------------------------------------------
class Game2DPachinkoMachine : public Game
{
public:
	Game2DPachinkoMachine();
	virtual ~Game2DPachinkoMachine();
	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;

protected:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;

private:
	void HandleInput();
	void HandleInputMoveLauncher();
	void HandleInputCameraAndGravityDirection();

	void UpdatePhysics(float deltaSeconds);
	void ApplyGravityAndMoveBalls(float deltaSeconds);
	void BounceBalls();
	void BounceBallsWithBumpers();
	void BounceBallsWithWalls(); // Also teleport

	void SpawnBall();

	void DrawObjects() const;
	void AddVertsForLauncher(std::vector<Vertex_PCU>& verts) const;
	void AddVertsForBalls(std::vector<Vertex_PCU>& verts) const;
	void AddVertsForWalls(std::vector<Vertex_PCU>& verts) const;

	void DrawPachinkoMachine() const;

	Vec2 MapMouseCursorToWorldPoint() const;

	Vec2 GetRandomUVs() const;

private:
	Camera m_camera;
	Vec2 m_cameraCenter = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);



	Vec2 m_startPos = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);
	Vec2 m_endPos = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);



	std::vector<PachinkoMachine::Bumper> m_bumpers;
	std::vector<PachinkoMachine::Ball> m_balls;

	std::vector<Vertex_PCU> m_bumperVerts;

	float m_ballElasticity = 0.9f;
	bool m_isBottomWallPresent = true;
	bool m_isFixedTimeStep = true;
	float m_fixedTimeStep = 0.005f; // it is same as timer's m_period
	float m_owedPhysicsSeconds = 0.f;

	float m_gravityAcceleration = 10.f;
	float m_gravityDirectionDegrees = -90.f;
	Vec2 m_gravityDirection = Vec2(0.f, -1.f); // also affect camera rotation

	float m_cameraSizeScale = 1.f;
	//-----------------------------------------------------------------------------------------------
	// Pachinko Box Setting
	// 
	float m_leftWallX		= 0.f;
	float m_rightWallX		= SCREEN_SIZE_X;
	float m_bottomWallY		= 0.f;
	float m_topPortalY		= SCREEN_SIZE_Y + PachinkoMachine::EXTRA_WRAP_HEIGHT;

};
