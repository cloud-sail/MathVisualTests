#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Input/InputSystem.hpp"

// ----------------------------------------------------------------------------------------------
class Game
{
public:
	Game();
	virtual ~Game();
	virtual void Update() = 0;
	virtual void Render() const = 0;
	virtual void RandomizeSceneObjects() = 0;
	virtual CursorMode GetCursorMode();

protected:
	virtual void UpdateCameras() = 0;
	virtual void DrawUsage() const = 0;

	void UpdateDeveloperCheats();

protected:
	// Camera m_screenCamera;
	Clock* m_clock = nullptr;
	CursorMode m_cursorMode = CursorMode::POINTER;
};



// ----------------------------------------------------------------------------------------------
// 2D template
//-----------------------------------------------------------------------------------------------
//#include "Game/Game.hpp"
//#include "Engine/Renderer/Camera.hpp"
//
//
//// ----------------------------------------------------------------------------------------------
//class Game2DPachinkoMachine : public Game
//{
//public:
//	Game2DPachinkoMachine();
//	virtual ~Game2DPachinkoMachine();
//	virtual void Update() override;
//	virtual void Render() const override;
//	virtual void RandomizeSceneObjects() override;
//
//protected:
//	virtual void UpdateCameras() override;
//	virtual void DrawUsage() const override;
//
//private:
//	void HandleInput();
//
//private:
//	Camera m_camera;
//
//};

//-----------------------------------------------------------------------------------------------

//#include "Game/Game2DPachinkoMachine.hpp"
//#include "Engine/Core/Clock.hpp"
//#include "Engine/Core/VertexUtils.hpp"
//#include "Engine/Input/InputSystem.hpp"
//#include "Engine/Math/RandomNumberGenerator.hpp"
//#include "Engine/Math/MathUtils.hpp"
//#include "Engine/Renderer/BitmapFont.hpp"
//#include "Engine/Renderer/Renderer.hpp"
//
//static const char* G2PACHINKO_TEXT = "Pachinko Machine (2D):";
//
//
////-----------------------------------------------------------------------------------------------
//Game2DPachinkoMachine::Game2DPachinkoMachine()
//{
//
//}
//
//Game2DPachinkoMachine::~Game2DPachinkoMachine()
//{
//
//}
//
//void Game2DPachinkoMachine::Update()
//{
//	UpdateDeveloperCheats();
//
//	HandleInput();
//
//	UpdateCameras();
//}
//
//void Game2DPachinkoMachine::Render() const
//{
//	g_theRenderer->BeginCamera(m_camera);
//  DrawUsage();
//  g_theRenderer->EndCamera(m_camera);
//}
//
//void Game2DPachinkoMachine::RandomizeSceneObjects()
//{
//
//}
//
//void Game2DPachinkoMachine::UpdateCameras()
//{
//	m_camera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
//}
//
//void Game2DPachinkoMachine::DrawUsage() const
//{
//	std::vector<Vertex_PCU> verts;
//	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
//	Vec2 alignment(0.f, 1.f);
//	float cellAspect = 0.6f;
//	BitmapFont* testFont = nullptr;
//	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
//
//	std::string usageText = Stringf(G2PACHINKO_TEXT);
//	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
//	g_theRenderer->BindTexture(&testFont->GetTexture());
//
//	g_theRenderer->BindShader(nullptr);
//	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
//	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
//	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
//	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
//
//	g_theRenderer->DrawVertexArray(verts);
//}
//
//void Game2DPachinkoMachine::HandleInput()
//{
//
//}

//-----------------------------------------------------------------------------------------------
// 3D template
//-----------------------------------------------------------------------------------------------
//#include "Game/Game.hpp"
//#include "Engine/Renderer/Camera.hpp"
//
//
//// ----------------------------------------------------------------------------------------------
//class Game3DQuaternion : public Game
//{
//public:
//	Game3DQuaternion();
//	virtual ~Game3DQuaternion();
//	virtual void Update() override;
//	virtual void Render() const override;
//	virtual void RandomizeSceneObjects() override;
//
//protected:
//	virtual void UpdateCameras() override;
//	virtual void DrawUsage() const override;
//
//	void UpdatePlayer();
//	void UpdatePlayerPosition(float deltaSeconds);
//	void UpdatePlayerOrientation();
//	void UpdatePlayerCameraCenterAxis();
//
//
//private:
//	void HandleInput();
//
//private:
//	Camera m_screenCamera;
//
//	Camera m_camera;
//	Vec3 m_playerPosition = Vec3(2.f, 2.f, 2.f);
//	EulerAngles m_playerOrientation = EulerAngles(-135.f, 0.f, 0.f);
//
//};

//-----------------------------------------------------------------------------------------------
//#include "Game/Game3DQuaternion.hpp"
//#include "Engine/Core/DebugRender.hpp"
//
//#include "Engine/Core/Clock.hpp"
//#include "Engine/Core/VertexUtils.hpp"
//#include "Engine/Input/InputSystem.hpp"
//#include "Engine/Math/RandomNumberGenerator.hpp"
//#include "Engine/Math/MathUtils.hpp"
//#include "Engine/Renderer/BitmapFont.hpp"
//#include "Engine/Renderer/Renderer.hpp"
//
//static const char* G2PACHINKO_TEXT = "Quaternion (3D):";
//
//
////-----------------------------------------------------------------------------------------------
//Game3DQuaternion::Game3DQuaternion()
//{
//	m_cursorMode = CursorMode::FPS;
//
//	m_camera.SetPerspectiveView(WINDOW_ASPECT, 60.f, 0.1f, 100.0f);
//	m_camera.SetCameraToRenderTransform(Mat44::DIRECTX_C2R);
//	DebugAddBasis(Mat44(), -1.f, 1.f, 0.05f);
//
//	RandomizeSceneObjects();
//}
//
//Game3DQuaternion::~Game3DQuaternion()
//{
//
//}
//
//void Game3DQuaternion::Update()
//{
//	UpdateDeveloperCheats();
//
//	HandleInput();
//	UpdatePlayer();
//
//	UpdateCameras();
//}
//
//void Game3DQuaternion::Render() const
//{
//	g_theRenderer->BeginCamera(m_camera);
//	//DrawObjects();
//	//DrawRaycastResult();
//	g_theRenderer->EndCamera(m_camera);
//	DebugRenderWorld(m_camera);
//
//	g_theRenderer->BeginCamera(m_screenCamera);
//	DrawUsage();
//	g_theRenderer->EndCamera(m_screenCamera);
//	DebugRenderScreen(m_screenCamera);
//}
//
//void Game3DQuaternion::RandomizeSceneObjects()
//{
//
//}
//
//void Game3DQuaternion::UpdateCameras()
//{
//	m_camera.SetPositionAndOrientation(m_playerPosition, m_playerOrientation);
//	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
//}
//
//void Game3DQuaternion::DrawUsage() const
//{
//	std::vector<Vertex_PCU> verts;
//	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
//	Vec2 alignment(0.f, 1.f);
//	float cellAspect = 0.6f;
//	BitmapFont* testFont = nullptr;
//	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
//
//	std::string usageText = Stringf(G2PACHINKO_TEXT);
//	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
//	g_theRenderer->BindTexture(&testFont->GetTexture());
//
//	g_theRenderer->BindShader(nullptr);
//	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
//	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
//	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
//	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
//
//	g_theRenderer->DrawVertexArray(verts);
//}
//
//void Game3DQuaternion::UpdatePlayer()
//{
//	float deltaSeconds = static_cast<float>(m_clock->GetDeltaSeconds());
//	UpdatePlayerPosition(deltaSeconds);
//	UpdatePlayerOrientation();
//
//	UpdatePlayerCameraCenterAxis();
//
//}
//
//void Game3DQuaternion::UpdatePlayerPosition(float deltaSeconds)
//{
//	Vec3 moveIntention;
//	if (g_theInput->IsKeyDown(KEYCODE_W))
//	{
//		moveIntention += Vec3(1.f, 0.f, 0.f);
//	}
//	if (g_theInput->IsKeyDown(KEYCODE_S))
//	{
//		moveIntention += Vec3(-1.f, 0.f, 0.f);
//	}
//	if (g_theInput->IsKeyDown(KEYCODE_A))
//	{
//		moveIntention += Vec3(0.f, 1.f, 0.f);
//	}
//	if (g_theInput->IsKeyDown(KEYCODE_D))
//	{
//		moveIntention += Vec3(0.f, -1.f, 0.f);
//	}
//	moveIntention.ClampLength(1.f);
//
//	Vec3 forwardIBasis, leftJBasis, upKBasis;
//	m_playerOrientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);
//	Vec3 forwardNormal = Vec3(forwardIBasis.x, forwardIBasis.y, 0.f).GetNormalized();
//	Vec3 leftNormal = Vec3(-forwardNormal.y, forwardNormal.x, 0.f);
//	m_playerPosition += (forwardNormal * moveIntention.x + leftNormal * moveIntention.y) * CAMERA_MOVE_SPEED * deltaSeconds;
//
//
//	Vec3 elevateIntention;
//	if (g_theInput->IsKeyDown(KEYCODE_Q))
//	{
//		elevateIntention += Vec3(0.f, 0.f, -1.f);
//	}
//	if (g_theInput->IsKeyDown(KEYCODE_E))
//	{
//		elevateIntention += Vec3(0.f, 0.f, 1.f);
//	}
//	elevateIntention.ClampLength(1.f);
//
//	m_playerPosition += elevateIntention * CAMERA_MOVE_SPEED * deltaSeconds;
//
//}
//
//void Game3DQuaternion::UpdatePlayerOrientation()
//{
//	Vec2 cursorPositionDelta = g_theInput->GetCursorClientDelta();
//	float deltaYaw = -cursorPositionDelta.x * 0.125f;
//	float deltaPitch = cursorPositionDelta.y * 0.125f;
//
//	m_playerOrientation.m_yawDegrees += deltaYaw;
//	m_playerOrientation.m_pitchDegrees += deltaPitch;
//	m_playerOrientation.m_pitchDegrees = GetClamped(m_playerOrientation.m_pitchDegrees, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);
//}
//
//void Game3DQuaternion::UpdatePlayerCameraCenterAxis()
//{
//	// Add camera center axis
//	Vec3 forwardIBasis, leftJBasis, upKBasis;
//	m_playerOrientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);
//	Mat44 cameraCenterAxisTransform;
//	cameraCenterAxisTransform.SetTranslation3D(m_playerPosition + 0.2f * forwardIBasis);
//	DebugAddBasis(cameraCenterAxisTransform, 0.f, 0.006f, 0.0003f);
//}
//
//void Game3DQuaternion::HandleInput()
//{
//
//}



//-----------------------------------------------------------------------------------------------