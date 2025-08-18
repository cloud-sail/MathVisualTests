#include "Game/Game2DFastVoxelRaycast.hpp"

#include "Game/Game2DFastVoxelRaycast.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

static const char* G2VOXEL_TEXT = "Fast Voxel Raycast (2D): Ray Start (ESDF/LMB), Ray End(IJKL/RMB), Arrow Key(entire ray)";
static constexpr float SOLID_PROBABILITY = 0.3f;
static constexpr float SOLID_VALUE = 1.f;

//-----------------------------------------------------------------------------------------------
Game2DFastVoxelRaycast::Game2DFastVoxelRaycast()
{
	m_rayStartPos = Vec2(400.f, 300.f);
	m_rayEndPos = Vec2(1200.f, 500.f);

	RandomizeSceneObjects();
}

Game2DFastVoxelRaycast::~Game2DFastVoxelRaycast()
{

}

void Game2DFastVoxelRaycast::Update()
{
	UpdateDeveloperCheats();

	HandleInput();

	UpdateCameras();
}

void Game2DFastVoxelRaycast::Render() const
{
	g_theRenderer->BeginCamera(m_camera);
	DrawSolidMap();
	DrawRaycastResult();
	DrawUsage();

	g_theRenderer->EndCamera(m_camera);
}

void Game2DFastVoxelRaycast::RandomizeSceneObjects()
{
	delete m_solidMap;
	m_solidMap = new TileHeatMap(m_gridDimensions);
	int numTiles = m_solidMap->GetNumTiles();
	for (int tileIndex = 0; tileIndex < numTiles; ++tileIndex)
	{
		if (g_rng.RollRandomWithProbability(SOLID_PROBABILITY))
		{
			m_solidMap->SetValueAtIndex(tileIndex, SOLID_VALUE);
		}
	}
}

void Game2DFastVoxelRaycast::UpdateCameras()
{
	m_camera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game2DFastVoxelRaycast::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	std::string usageText = Stringf(G2VOXEL_TEXT);
	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game2DFastVoxelRaycast::HandleInput()
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
	m_rayStartPos += startPosIntention * RAYCAST_MOVESPEED * deltaSeconds;
	m_rayEndPos += endPosIntention * RAYCAST_MOVESPEED * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		m_rayStartPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
	{
		m_rayEndPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
	}
}

void Game2DFastVoxelRaycast::DrawSolidMap() const
{
	std::vector<Vertex_PCU> verts;

	Vec2 dimensions = Vec2(static_cast<float>(m_gridDimensions.x) * m_cellSize.x, static_cast<float>(m_gridDimensions.y) * m_cellSize.y);

	m_solidMap->AddVertsForDebugDraw(verts, AABB2(m_gridOrigin, m_gridOrigin + dimensions), FloatRange(0.f, SOLID_VALUE), Rgba8::TRANSPARENT_BLACK, DARK_BLUE);

	// Draw Grid
	float thickness = m_cellSize.x * 0.05f;
	for (int i = 0; i <= m_gridDimensions.x; ++i)
	{
		Vec2 start = Vec2(static_cast<float>(i) * m_cellSize.x, 0.f) + m_gridOrigin;
		Vec2 end = Vec2(static_cast<float>(i) * m_cellSize.x, static_cast<float>(m_gridDimensions.y) * m_cellSize.y) + m_gridOrigin;
		AddVertsForLineSegment2D(verts, start, end, thickness, DARK_GREY);
	}
	for (int i = 0; i <= m_gridDimensions.y; ++i)
	{
		Vec2 start = Vec2(0.f, static_cast<float>(i) * m_cellSize.y) + m_gridOrigin;
		Vec2 end = Vec2(static_cast<float>(m_gridDimensions.x) * m_cellSize.x, static_cast<float>(i) * m_cellSize.y) + m_gridOrigin;
		AddVertsForLineSegment2D(verts, start, end, thickness, DARK_GREY);
	}



	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game2DFastVoxelRaycast::DrawRaycastResult() const
{
	Vec2 disp = m_rayEndPos - m_rayStartPos;
	RaycastResult2D raycastResult = FastVoxelRaycast(m_rayStartPos, disp.GetNormalized(), disp.GetLength());
	
	std::vector<Vertex_PCU> verts;





	if (!raycastResult.m_didImpact)
	{
		AddVertsForArrow2D(verts, m_rayStartPos, m_rayEndPos, RAYCAST_ARROW_SIZE, RAYCAST_LINE_THICKNESS, Rgba8::OPAQUE_WHITE);
	}
	else
	{
		AddVertsForArrow2D(verts, m_rayStartPos, m_rayEndPos, RAYCAST_ARROW_SIZE, RAYCAST_LINE_THICKNESS, DARK_GREY);
		AddVertsForArrow2D(verts, m_rayStartPos, raycastResult.m_impactPos, RAYCAST_ARROW_SIZE, RAYCAST_LINE_THICKNESS, ORANGE);
		AddVertsForArrow2D(verts, raycastResult.m_impactPos, raycastResult.m_impactPos + RAYCAST_NORMAL_ARROW_LENGTH * raycastResult.m_impactNormal, RAYCAST_ARROW_SIZE, RAYCAST_LINE_THICKNESS, Rgba8::CYAN);
		AddVertsForDisc2D(verts, raycastResult.m_impactPos, RAYCAST_WHITE_DOT_RADIUS, Rgba8::OPAQUE_WHITE);

	}

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

RaycastResult2D Game2DFastVoxelRaycast::FastVoxelRaycast(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength) const
{
	RaycastResult2D raycastResult;
	raycastResult.m_ray.m_startPos = rayStart;
	raycastResult.m_ray.m_fwdNormal = rayForwardNormal;
	raycastResult.m_ray.m_maxLength = rayLength;

	Vec2 disp = rayStart - m_gridOrigin;
	int tileX = RoundDownToInt(disp.x / m_cellSize.x);
	int tileY = RoundDownToInt(disp.y / m_cellSize.y);

	if (IsTileSolid(tileX, tileY))
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactNormal = -rayForwardNormal;
		return raycastResult;
	}

	float fwdDistPerXCrossing = 1.0f / fabsf(rayForwardNormal.x); // 1.f / cos theta
	int	tileStepDirectionX = (rayForwardNormal.x < 0.f) ? -1 : 1;
	float xAtFirstXCrossing = (static_cast<float>(tileX) + static_cast<float>(tileStepDirectionX + 1) * 0.5f) * m_cellSize.x + m_gridOrigin.x;
	float xDistToFirstXCrossing = xAtFirstXCrossing - rayStart.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing; // forward direction distance

	float fwdDistPerYCrossing = 1.0f / fabsf(rayForwardNormal.y); // 1 / sin theta
	int tileStepDirectionY = (rayForwardNormal.y < 0.f) ? -1 : 1;
	float yAtFirstYCrossing = (static_cast<float>(tileY) + static_cast<float>(tileStepDirectionY + 1) * 0.5f) * m_cellSize.y + m_gridOrigin.y;
	float yDistToFirstYCrossing = yAtFirstYCrossing - rayStart.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing; // forward direction distance

	while (true)
	{
		if (fwdDistAtNextXCrossing <= fwdDistAtNextYCrossing)
		{
			if (fwdDistAtNextXCrossing > rayLength)
			{
				raycastResult.m_impactDist = rayLength;
				return raycastResult;
			}
			tileX += tileStepDirectionX;
			if (IsTileSolid(tileX, tileY))
			{
				raycastResult.m_didImpact = true;
				raycastResult.m_impactDist = fwdDistAtNextXCrossing;
				raycastResult.m_impactPos = rayStart + raycastResult.m_impactDist * rayForwardNormal;
				raycastResult.m_impactNormal = Vec2(-static_cast<float>(tileStepDirectionX), 0.f);
				return raycastResult;
			}
			fwdDistAtNextXCrossing += m_cellSize.x * fwdDistPerXCrossing;
		}
		else
		{
			if (fwdDistAtNextYCrossing > rayLength)
			{
				raycastResult.m_impactDist = rayLength;
				return raycastResult;
			}
			tileY += tileStepDirectionY;
			if (IsTileSolid(tileX, tileY))
			{
				raycastResult.m_didImpact = true;
				raycastResult.m_impactDist = fwdDistAtNextYCrossing;
				raycastResult.m_impactPos = rayStart + raycastResult.m_impactDist * rayForwardNormal;
				raycastResult.m_impactNormal = Vec2(0.f, -static_cast<float>(tileStepDirectionY));
				return raycastResult;
			}
			fwdDistAtNextYCrossing += m_cellSize.y * fwdDistPerYCrossing;
		}
	}

}

bool Game2DFastVoxelRaycast::IsTileSolid(int tileX, int tileY) const
{
	IntVec2 const tileCoords = IntVec2(tileX, tileY);
	if (!m_solidMap->IsInBounds(tileCoords))
	{
		return false;
	}

	return m_solidMap->GetValueAtCoords(tileCoords) == SOLID_VALUE;
}

IntVec2 Game2DFastVoxelRaycast::GetTileCoordsForWorldPos(Vec2 const& worldPos) const
{
	Vec2 disp = worldPos - m_gridOrigin;

	int tileX = RoundDownToInt(disp.x / m_cellSize.x);
	int tileY = RoundDownToInt(disp.y / m_cellSize.y);

	return IntVec2(tileX, tileY);
}
