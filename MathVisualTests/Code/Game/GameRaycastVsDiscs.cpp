#include "Game/GameRaycastVsDiscs.hpp"
#include "Game/App.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

//-----------------------------------------------------------------------------------------------
static constexpr float GRD_MOVESPEED = 200.f;
static constexpr float GRD_WHITE_DOT_RADIUS = 6.f;
static constexpr float GRD_LINE_THICKNESS = 3.f;
static constexpr float GRD_ARROW_SIZE = 25.f;
static constexpr float GRD_NORMAL_ARROW_LENGTH = 100.f;
static constexpr int	DISC_NUM = 12;

static const std::string GRV_TEXT = "GameRaycastVsDiscs: Ray Start (ESDF/LMB), Ray End(IJKL/RMB), Arrow Key(entire ray)";


//-----------------------------------------------------------------------------------------------
GameRaycastVsDiscs::GameRaycastVsDiscs()
{
	m_startPos = Vec2(100.f, 100.f);
	m_endPos = Vec2(SCREEN_SIZE_X - 100.f, SCREEN_SIZE_Y - 100.f);


	Vec2 sceneDimensions = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);
	m_shapeList.reserve(DISC_NUM);
	for (int i = 0; i < DISC_NUM; ++i)
	{
		m_shapeList.push_back(new GRDO_Disc(sceneDimensions));
	}
}

GameRaycastVsDiscs::~GameRaycastVsDiscs()
{
	for (GRDO_Disc* ptr : m_shapeList) {
		delete ptr;
	}
	m_shapeList.clear();
}

void GameRaycastVsDiscs::Update()
{
	UpdateDeveloperCheats();

	HandleInput();
	DoRaycast();

	UpdateCameras();
}

void GameRaycastVsDiscs::Render() const
{
	g_theRenderer->BeginCamera(m_camera);

	DrawObjects();
	DrawUsage();

	g_theRenderer->EndCamera(m_camera);
}

void GameRaycastVsDiscs::UpdateCameras()
{
	m_camera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void GameRaycastVsDiscs::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + GRV_TEXT, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void GameRaycastVsDiscs::RandomizeSceneObjects()
{
	for (GRDO_Disc* ptr : m_shapeList) {
		delete ptr;
	}
	m_shapeList.clear();

	Vec2 sceneDimensions = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);
	m_shapeList.reserve(DISC_NUM);
	for (int i = 0; i < DISC_NUM; ++i)
	{
		m_shapeList.push_back(new GRDO_Disc(sceneDimensions));
	}
}

void GameRaycastVsDiscs::HandleInput()
{
	Vec2 startPosIntention = Vec2::ZERO;

	if (g_theInput->IsKeyDown(KEYCODE_E))
	{
		startPosIntention += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_D))
	{
		startPosIntention += Vec2(0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_S))
	{
		startPosIntention += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_F))
	{
		startPosIntention += Vec2(1.f, 0.f);
	}

	Vec2 endPosIntention = Vec2::ZERO;

	if (g_theInput->IsKeyDown(KEYCODE_I))
	{
		endPosIntention += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_K))
	{
		endPosIntention += Vec2(0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_J))
	{
		endPosIntention += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_L))
	{
		endPosIntention += Vec2(1.f, 0.f);
	}

	if (g_theInput->IsKeyDown(KEYCODE_UP))
	{
		startPosIntention += Vec2(0.f, 1.f);
		endPosIntention += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_DOWN))
	{
		startPosIntention += Vec2(0.f, -1.f);
		endPosIntention += Vec2(0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_LEFT))
	{
		startPosIntention += Vec2(-1.f, 0.f);
		endPosIntention += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_RIGHT))
	{
		startPosIntention += Vec2(1.f, 0.f);
		endPosIntention += Vec2(1.f, 0.f);
	}

	startPosIntention.ClampLength(1.f);
	endPosIntention.ClampLength(1.f);

	float deltaSeconds = static_cast<float>(m_clock->GetDeltaSeconds());
	m_startPos += startPosIntention * GRD_MOVESPEED * deltaSeconds;
	m_endPos += endPosIntention * GRD_MOVESPEED * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		m_startPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
	{
		m_endPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
	}
}

void GameRaycastVsDiscs::DoRaycast()
{
	m_hitDisc = nullptr;
	m_raycastResult = {};
	for (int i = 0; i < (int)m_shapeList.size(); ++i)
	{
		Vec2 disp = m_endPos - m_startPos;
		RaycastResult2D result = RaycastVsDisc2D(m_startPos, disp.GetNormalized(), disp.GetLength(), m_shapeList[i]->m_center, m_shapeList[i]->m_radius);
		if (result.m_didImpact)
		{
			if (!m_raycastResult.m_didImpact)
			{
				m_raycastResult = result;
				m_hitDisc = m_shapeList[i];
			}
			else
			{
				if (m_raycastResult.m_impactDist > result.m_impactDist)
				{
					m_raycastResult = result;
					m_hitDisc = m_shapeList[i];
				}
			}
		}
	}

}

void GameRaycastVsDiscs::DrawObjects() const
{
	std::vector<Vertex_PCU> verts;
	for (GRDO_Disc* shape : m_shapeList) {
		AddVertsForDisc2D(verts, shape->m_center, shape->m_radius, DARK_BLUE);
	}

	if (m_hitDisc)
	{
		AddVertsForDisc2D(verts, m_hitDisc->m_center, m_hitDisc->m_radius, LIGHT_BLUE);
	}

	if (!m_raycastResult.m_didImpact)
	{
		AddVertsForArrow2D(verts, m_startPos, m_endPos, GRD_ARROW_SIZE, GRD_LINE_THICKNESS, Rgba8::OPAQUE_WHITE);
	}
	else
	{
		AddVertsForArrow2D(verts, m_startPos, m_endPos, GRD_ARROW_SIZE, GRD_LINE_THICKNESS, DARK_GREY);
		AddVertsForArrow2D(verts, m_startPos, m_raycastResult.m_impactPos, GRD_ARROW_SIZE, GRD_LINE_THICKNESS, ORANGE);
		AddVertsForArrow2D(verts, m_raycastResult.m_impactPos, m_raycastResult.m_impactPos + GRD_NORMAL_ARROW_LENGTH * m_raycastResult.m_impactNormal, GRD_ARROW_SIZE, GRD_LINE_THICKNESS, Rgba8::CYAN);
		AddVertsForDisc2D(verts, m_raycastResult.m_impactPos, GRD_WHITE_DOT_RADIUS, Rgba8::OPAQUE_WHITE);

	}

	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

//-----------------------------------------------------------------------------------------------
GRDO_Disc::GRDO_Disc(Vec2 const& sceneDimensions)
{
	m_radius = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.01f, sceneDimensions.y * 0.25f);
	m_center.x = g_rng.RollRandomFloatInRange(m_radius, sceneDimensions.x - m_radius);
	m_center.y = g_rng.RollRandomFloatInRange(m_radius, sceneDimensions.y - m_radius);
}
