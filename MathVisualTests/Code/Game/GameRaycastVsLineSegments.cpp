#include "Game/GameRaycastVsLineSegments.hpp"
#include "Game/App.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

//-----------------------------------------------------------------------------------------------
static constexpr int	LINE_SEGMENT_NUM = 20;
static constexpr float LINE_SEGMENT_THICKNESS = 5.f;

static const std::string GRL_TEXT = "GameRaycastVsLineSegments: Ray Start (ESDF/LMB), Ray End(IJKL/RMB), Arrow Key(entire ray)";


//-----------------------------------------------------------------------------------------------
GameRaycastVsLineSegments::GameRaycastVsLineSegments()
{
	m_startPos = Vec2(100.f, 100.f);
	m_endPos = Vec2(SCREEN_SIZE_X - 100.f, SCREEN_SIZE_Y - 100.f);

	Vec2 sceneDimensions = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);
	m_shapeList.reserve(LINE_SEGMENT_NUM);
	for (int i = 0; i < LINE_SEGMENT_NUM; ++i)
	{
		m_shapeList.push_back(new GRO_LineSegement(sceneDimensions));
	}
}

GameRaycastVsLineSegments::~GameRaycastVsLineSegments()
{
	for (GRO_LineSegement* ptr : m_shapeList) {
		delete ptr;
	}
	m_shapeList.clear();
}

void GameRaycastVsLineSegments::Update()
{
	UpdateDeveloperCheats();

	HandleInput();
	DoRaycast();

	UpdateCameras();
}

void GameRaycastVsLineSegments::Render() const
{
	g_theRenderer->BeginCamera(m_camera);

	DrawObjects();
	DrawUsage();

	g_theRenderer->EndCamera(m_camera);
}

void GameRaycastVsLineSegments::UpdateCameras()
{
	m_camera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void GameRaycastVsLineSegments::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + GRL_TEXT, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void GameRaycastVsLineSegments::RandomizeSceneObjects()
{
	for (GRO_LineSegement* ptr : m_shapeList) {
		delete ptr;
	}
	m_shapeList.clear();

	Vec2 sceneDimensions = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);
	m_shapeList.reserve(LINE_SEGMENT_NUM);
	for (int i = 0; i < LINE_SEGMENT_NUM; ++i)
	{
		m_shapeList.push_back(new GRO_LineSegement(sceneDimensions));
	}
}

void GameRaycastVsLineSegments::HandleInput()
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
	m_startPos += startPosIntention * RAYCAST_MOVESPEED * deltaSeconds;
	m_endPos += endPosIntention * RAYCAST_MOVESPEED * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		m_startPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
	{
		m_endPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
	}
}

void GameRaycastVsLineSegments::DoRaycast()
{
	m_hitObject = nullptr;
	m_raycastResult = {};
	for (int i = 0; i < (int)m_shapeList.size(); ++i)
	{
		Vec2 disp = m_endPos - m_startPos;
		RaycastResult2D result = RaycastVsLineSegment2D(m_startPos, disp.GetNormalized(), disp.GetLength(), m_shapeList[i]->m_start, m_shapeList[i]->m_end);
		if (result.m_didImpact)
		{
			if (!m_raycastResult.m_didImpact)
			{
				m_raycastResult = result;
				m_hitObject = m_shapeList[i];
			}
			else
			{
				if (m_raycastResult.m_impactDist > result.m_impactDist)
				{
					m_raycastResult = result;
					m_hitObject = m_shapeList[i];
				}
			}
		}
	}

}

void GameRaycastVsLineSegments::DrawObjects() const
{
	std::vector<Vertex_PCU> verts;
	for (GRO_LineSegement* shape : m_shapeList) {
		AddVertsForLineSegment2D(verts, shape->m_start, shape->m_end, LINE_SEGMENT_THICKNESS, DARK_BLUE);
	}

	if (m_hitObject)
	{
		AddVertsForLineSegment2D(verts, m_hitObject->m_start, m_hitObject->m_end, LINE_SEGMENT_THICKNESS, LIGHT_BLUE);
	}

	if (!m_raycastResult.m_didImpact)
	{
		AddVertsForArrow2D(verts, m_startPos, m_endPos, RAYCAST_ARROW_SIZE, RAYCAST_LINE_THICKNESS, Rgba8::OPAQUE_WHITE);
	}
	else
	{
		AddVertsForArrow2D(verts, m_startPos, m_endPos, RAYCAST_ARROW_SIZE, RAYCAST_LINE_THICKNESS, DARK_GREY);
		AddVertsForArrow2D(verts, m_startPos, m_raycastResult.m_impactPos, RAYCAST_ARROW_SIZE, RAYCAST_LINE_THICKNESS, ORANGE);
		AddVertsForArrow2D(verts, m_raycastResult.m_impactPos, m_raycastResult.m_impactPos + RAYCAST_NORMAL_ARROW_LENGTH * m_raycastResult.m_impactNormal, RAYCAST_ARROW_SIZE, RAYCAST_LINE_THICKNESS, Rgba8::CYAN);
		AddVertsForDisc2D(verts, m_raycastResult.m_impactPos, RAYCAST_WHITE_DOT_RADIUS, Rgba8::OPAQUE_WHITE);

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
GRO_LineSegement::GRO_LineSegement(Vec2 const& sceneDimensions)
{
	float discRadius = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.05f, sceneDimensions.y * 0.15f);
	Vec2 discCenter;
	discCenter.x = g_rng.RollRandomFloatInRange(discRadius, sceneDimensions.x - discRadius);
	discCenter.y = g_rng.RollRandomFloatInRange(discRadius, sceneDimensions.y - discRadius);

	float angle = g_rng.RollRandomFloatInRange(0.f, 360.f);

	Vec2 direction = Vec2::MakeFromPolarDegrees(angle);
	m_start = discCenter + discRadius * direction;
	m_end = discCenter - discRadius * direction;
}
