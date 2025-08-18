#include "Game/Game2DExposureAvoidance.hpp"

#include "Game/Game2DExposureAvoidance.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Gradient.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

static const char* G2EXP_TEXT = "Exposure Avoidance (2D): LMB Add/Remove Sentinel";
static constexpr float SOLID_PROBABILITY = 0.1f;
static constexpr float SOLID_VALUE = 1.f;
static constexpr float SPECIAL_VALUE_POS = 999999.f;
static constexpr float SPECIAL_VALUE_NEG = -1.f; // this is the reverse spread lower bound, useful
static constexpr float EXPOSED_VALUE = 10000.f;
static constexpr float UNEXPOSED_VALUE = 0.f;

//-----------------------------------------------------------------------------------------------
Game2DExposureAvoidance::Game2DExposureAvoidance()
{

	RandomizeSceneObjects();
}

Game2DExposureAvoidance::~Game2DExposureAvoidance()
{

}

void Game2DExposureAvoidance::Update()
{
	UpdateDeveloperCheats();

	HandleInput();
	UpdateExposureMap();
	UpdateCameras();
}

void Game2DExposureAvoidance::Render() const
{
	g_theRenderer->BeginCamera(m_camera);
	DrawExposureMap();
	//DrawSolidMap();
	DrawTileGrid();
	DrawSentinels();
	DrawUsage();

	g_theRenderer->EndCamera(m_camera);
}

void Game2DExposureAvoidance::RandomizeSceneObjects()
{
	m_sentinels.clear();

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

	// Update exposure map size here
	m_exposureMap = TileHeatMap(m_gridDimensions);
}

void Game2DExposureAvoidance::UpdateCameras()
{
	m_camera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game2DExposureAvoidance::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	std::string usageText = Stringf(G2EXP_TEXT);
	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game2DExposureAvoidance::HandleInput()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		Vec2 clickPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));

		int clickedSentinelIndex = -1;
		int numSentinels = (int)m_sentinels.size();
		for(int index = 0; index < numSentinels; ++index)
		{
			if (IsPointInsideDisc2D(clickPos, m_sentinels[index], m_clickRadius))
			{
				clickedSentinelIndex = index;
				break;
			}
		}

		if (clickedSentinelIndex >= 0)
		{
			m_sentinels.erase(m_sentinels.begin() + clickedSentinelIndex);
		}
		else
		{
			m_sentinels.push_back(clickPos);
		}
	}
}

void Game2DExposureAvoidance::UpdateExposureMap()
{


	GUARANTEE_OR_DIE(m_exposureMap.m_dimensions == m_gridDimensions, "TileHeatMap and Map did not match!");
	m_exposureMap.SetAllValues(UNEXPOSED_VALUE);

	// Cast rays from each sentinel to each cell, and set 
	for (Vec2 const& sentinel : m_sentinels)
	{
		for (int tileY = 0; tileY < m_gridDimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < m_gridDimensions.x; ++tileX)
			{
				if (IsTileSolid(tileX, tileY))
				{
					m_exposureMap.SetValueAtCoords(IntVec2(tileX, tileY), SPECIAL_VALUE_POS);
					continue;
				}

				Vec2 rayEnd = GetTileCenter(tileX, tileY);
				Vec2 disp = rayEnd - sentinel;
				RaycastResult2D result = FastVoxelRaycast(sentinel, disp.GetNormalized(), disp.GetLength());
				// Limited view range
				if (!result.m_didImpact && result.m_impactDist <= m_sightRange)
				{
					// Sentinel can see
					m_exposureMap.SetValueAtCoords(IntVec2(tileX, tileY), EXPOSED_VALUE);
				}
			}
		}
	}

	SpreadDistanceMapHeat(m_exposureMap, UNEXPOSED_VALUE, 1.f);

	// Reverse spread
	int numTiles = m_exposureMap.GetNumTiles();
	for (int tileIndex = 0; tileIndex < numTiles; ++tileIndex)
	{
		if (m_exposureMap.GetValueAtIndex(tileIndex) == UNEXPOSED_VALUE)
		{
			m_exposureMap.SetValueAtIndex(tileIndex, SPECIAL_VALUE_NEG);
		}
	}

	SpreadDistanceMapHeat(m_exposureMap, UNEXPOSED_VALUE + 1.f, -1.f);


	
}

void Game2DExposureAvoidance::DrawExposureMap() const
{
	std::vector<Vertex_PCU> verts;

	Vec2 dimensions = Vec2(static_cast<float>(m_gridDimensions.x) * m_cellSize.x, static_cast<float>(m_gridDimensions.y) * m_cellSize.y);

	//m_exposureMap.AddVertsForDebugDraw(verts, AABB2(m_gridOrigin, m_gridOrigin + dimensions), Gradient::MakeHeatGradient(), m_exposureMap.GetRangeOffValuesExcludingSpecial(SPECIAL_VALUE_POS), SPECIAL_VALUE_POS, Rgba8::OPAQUE_WHITE);
	m_exposureMap.AddVertsForDebugDraw(verts, AABB2(m_gridOrigin, m_gridOrigin + dimensions), m_exposureMap.GetRangeOffValuesExcludingSpecial(SPECIAL_VALUE_POS), UNEXPOSED_VALUE,
		Rgba8(0,233,233), Rgba8(0,0,255), Rgba8(70,0,0), Rgba8(255,0,0), SPECIAL_VALUE_POS, Rgba8::YELLOW);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);


	// Draw Text
	std::vector<Vertex_PCU> textVerts;
	BitmapFont* testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	AABB2 totalBounds = AABB2(m_gridOrigin, m_gridOrigin + dimensions);

	for (int tileY = 0; tileY < m_gridDimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_gridDimensions.x; ++tileX)
		{
			int tileIndex = tileX + m_gridDimensions.x * tileY;
			float value = m_exposureMap.GetValueAtIndex(tileIndex);

			float outMinX = RangeMap(static_cast<float>(tileX), 0.f, static_cast<float>(m_gridDimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMaxX = RangeMap(static_cast<float>(tileX + 1), 0.f, static_cast<float>(m_gridDimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMinY = RangeMap(static_cast<float>(tileY), 0.f, static_cast<float>(m_gridDimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);
			float outMaxY = RangeMap(static_cast<float>(tileY + 1), 0.f, static_cast<float>(m_gridDimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);

			AABB2 tileBounds = AABB2(outMinX, outMinY, outMaxX, outMaxY);

			std::string valueText = Stringf("%.0f", value);
			testFont->AddVertsForTextInBox2D(textVerts, valueText, tileBounds, 14.f);

		}
	}

	g_theRenderer->BindTexture(&testFont->GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(textVerts);

}

void Game2DExposureAvoidance::DrawSolidMap() const
{
	std::vector<Vertex_PCU> verts;

	Vec2 dimensions = Vec2(static_cast<float>(m_gridDimensions.x) * m_cellSize.x, static_cast<float>(m_gridDimensions.y) * m_cellSize.y);

	m_solidMap->AddVertsForDebugDraw(verts, AABB2(m_gridOrigin, m_gridOrigin + dimensions), FloatRange(0.f, SOLID_VALUE), Rgba8::TRANSPARENT_BLACK, DARK_BLUE);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}


void Game2DExposureAvoidance::DrawTileGrid() const
{
	std::vector<Vertex_PCU> verts;

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

void Game2DExposureAvoidance::DrawSentinels() const
{
	std::vector<Vertex_PCU> verts;

	for (Vec2 const& sentinel : m_sentinels)
	{
		AddVertsForDisc2D(verts, sentinel, m_clickRadius, Rgba8(0, 200, 0), 16);
	}

	

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

RaycastResult2D Game2DExposureAvoidance::FastVoxelRaycast(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength) const
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

bool Game2DExposureAvoidance::IsTileSolid(int tileX, int tileY) const
{
	IntVec2 const tileCoords = IntVec2(tileX, tileY);
	if (!m_solidMap->IsInBounds(tileCoords))
	{
		return false;
	}

	return m_solidMap->GetValueAtCoords(tileCoords) == SOLID_VALUE;
}

IntVec2 Game2DExposureAvoidance::GetTileCoordsForWorldPos(Vec2 const& worldPos) const
{
	Vec2 disp = worldPos - m_gridOrigin;

	int tileX = RoundDownToInt(disp.x / m_cellSize.x);
	int tileY = RoundDownToInt(disp.y / m_cellSize.y);

	return IntVec2(tileX, tileY);
}

Vec2 Game2DExposureAvoidance::GetTileCenter(int tileX, int tileY) const
{
	return Vec2(static_cast<float>(tileX) + 0.5f, static_cast<float>(tileY) + 0.5f)* m_cellSize + m_gridOrigin;
}

void Game2DExposureAvoidance::SpreadDistanceMapHeat(TileHeatMap& distanceMap, float startSearchValue, float heatSpreadStep /*= 1.f*/)
{
	// Normally the spread step is +1, and Increasing Heat
	// Already set values, just do heat spreading
	IntVec2 const dimensions = distanceMap.m_dimensions;
	IntVec2 directions[4] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0) };

	bool isHeatIncreasing = heatSpreadStep > 0.f;

	float currentSearchValue = startSearchValue;

	bool isHeatSpreading = true;

	while (isHeatSpreading)
	{
		isHeatSpreading = false;

		for (int tileY = 0; tileY < dimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < dimensions.x; ++tileX)
			{
				IntVec2 const currentTileCoords = IntVec2(tileX, tileY);
				float currentTileValue = distanceMap.GetValueAtCoords(currentTileCoords);
				if (currentTileValue == currentSearchValue)
				{
					isHeatSpreading = true; // or on neighbor value changed, one more round of search
					for (int i = 0; i < 4; ++i) // four directions
					{
						IntVec2 neighborTileCoords = currentTileCoords + directions[i];
						if (!distanceMap.IsInBounds(neighborTileCoords))
						{
							continue;
						}
						if (IsTileSolid(neighborTileCoords.x, neighborTileCoords.y))
						{
							continue;
						}
						float neighborTileValue = distanceMap.GetValueAtCoords(neighborTileCoords);
						if (isHeatIncreasing && neighborTileValue <= (currentSearchValue + heatSpreadStep))
						{
							continue;
						}
						if (!isHeatIncreasing && neighborTileValue >= (currentSearchValue + heatSpreadStep))
						{
							continue;
						}
						distanceMap.SetValueAtCoords(neighborTileCoords, currentSearchValue + heatSpreadStep);
					}
				}
			}
		}
		currentSearchValue += heatSpreadStep;
	}
}
