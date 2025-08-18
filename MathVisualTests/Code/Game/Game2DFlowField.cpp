#include "Game/Game2DFlowField.hpp"

#include "Game/Game2DFlowField.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Gradient.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

static const char* G2EXP_TEXT = "Flow Field (2D): LMB: Add/Remove Start Points, RMB: Add/Remove End Points, Space: Generate one Actor";
static constexpr float SOLID_PROBABILITY = 0.1f;
static constexpr float SOLID_VALUE = 1.f;
static constexpr float SPECIAL_VALUE = 999999.f;
static constexpr float MAX_ACTOR_SPEED = 80.f;
static constexpr float MIN_ACTOR_SPEED = 50.f;
static constexpr float EXIT_VALUE = 0.f;

//-----------------------------------------------------------------------------------------------
Game2DFlowField::Game2DFlowField()
{
	RandomizeSceneObjects();
}

Game2DFlowField::~Game2DFlowField()
{

}

void Game2DFlowField::Update()
{
	UpdateDeveloperCheats();

	HandleInput();
	UpdateActors();
	TeleportActors();

	UpdateCameras();
}

void Game2DFlowField::Render() const
{
	g_theRenderer->BeginCamera(m_camera);
	DrawDistanceMap();
	
	DrawFlowField();
	//DrawSolidMap();
	DrawTileGrid();
	DrawStartsAndEnds();
	DrawActors();
	DrawUsage();

	g_theRenderer->EndCamera(m_camera);
}

void Game2DFlowField::RandomizeSceneObjects()
{
	m_startRadius = m_cellSize.x * 0.45f;
	m_endRadius = m_cellSize.x * 0.35f;

	m_starts.clear();
	m_ends.clear();

	m_actors.clear();

	RecreateSolidMap();

	RecreateDistanceMapAndFlowField();
}

void Game2DFlowField::UpdateCameras()
{
	m_camera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game2DFlowField::DrawUsage() const
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

void Game2DFlowField::HandleInput()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		Vec2 clickPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
		IntVec2 const clickedTileCoords = GetTileCoordsForWorldPos(clickPos);
		if (m_solidMap->IsInBounds(clickedTileCoords))
		{
			int clickedIndex = -1;
			int numPoints = (int)m_starts.size();
			for (int index = 0; index < numPoints; ++index)
			{
				if (m_starts[index] == clickedTileCoords)
				{
					clickedIndex = index;
					break;
				}
			}

			if (clickedIndex >= 0)
			{
				m_starts.erase(m_starts.begin() + clickedIndex);
			}
			else
			{
				m_starts.push_back(clickedTileCoords);
			}
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		Vec2 clickPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
		IntVec2 const clickedTileCoords = GetTileCoordsForWorldPos(clickPos);
		if (m_solidMap->IsInBounds(clickedTileCoords))
		{
			int clickedIndex = -1;
			int numPoints = (int)m_ends.size();
			for (int index = 0; index < numPoints; ++index)
			{
				if (m_ends[index] == clickedTileCoords)
				{
					clickedIndex = index;
					break;
				}
			}

			if (clickedIndex >= 0)
			{
				m_ends.erase(m_ends.begin() + clickedIndex);
			}
			else
			{
				m_ends.push_back(clickedTileCoords);
			}

			RecreateDistanceMapAndFlowField();
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		SpawnActor();
	}
}

void Game2DFlowField::RecreateSolidMap()
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

void Game2DFlowField::RecreateDistanceMapAndFlowField()
{
	// Create Distance Map from m_ends and solid Map
	delete m_distanceMap;
	m_distanceMap = new TileHeatMap(m_gridDimensions, SPECIAL_VALUE);
	
	for (IntVec2 const& endPoint : m_ends)
	{
		m_distanceMap->SetValueAtCoords(endPoint, EXIT_VALUE);
	}

	SpreadDistanceMapHeat(*m_distanceMap, EXIT_VALUE);

	// Create Flow Field by Select Down hill from 8 direction, If not found, set it zero
	delete m_flowField;
	m_flowField = new TileVectorField(m_gridDimensions, Vec2::ZERO);

	IntVec2 directions[8] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0), IntVec2(1,1), IntVec2(-1,1), IntVec2(-1,-1), IntVec2(1,-1) };
	for (int tileY = 0; tileY < m_gridDimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_gridDimensions.x; ++tileX)
		{
			IntVec2 flowDirection = IntVec2(0, 0);
			float minDelta = 0; // delta = neighborValue - currentValue

			IntVec2 currentTileCoords = IntVec2(tileX, tileY);
			float currentTileValue = m_distanceMap->GetValueAtCoords(currentTileCoords);
			bool isNeighborImpassable[4] = {};
			for (int i = 0; i < 4; ++i)
			{
				IntVec2 neighborTileCoords = currentTileCoords + directions[i];
				if (!m_flowField->IsInBounds(neighborTileCoords) || m_solidMap->GetValueAtCoords(neighborTileCoords) == SOLID_VALUE) // maybe using IsTileSolid is better
				{
					isNeighborImpassable[i] = true;
					continue;
				}
				float neighborTileValue = m_distanceMap->GetValueAtCoords(neighborTileCoords);
				float delta = neighborTileValue - currentTileValue;
				if (delta < minDelta)
				{
					flowDirection = directions[i];
					minDelta = delta;
				}
			}

			for (int i = 4; i < 8; ++i)
			{
				// The Corner is not accessible
				if (i == 4 && isNeighborImpassable[0] && isNeighborImpassable[2]) continue;
				if (i == 5 && isNeighborImpassable[0] && isNeighborImpassable[3]) continue;
				if (i == 6 && isNeighborImpassable[1] && isNeighborImpassable[3]) continue;
				if (i == 7 && isNeighborImpassable[1] && isNeighborImpassable[2]) continue;

				IntVec2 neighborTileCoords = currentTileCoords + directions[i];
				if (!m_flowField->IsInBounds(neighborTileCoords))
				{
					continue;
				}
				float neighborTileValue = m_distanceMap->GetValueAtCoords(neighborTileCoords);
				float delta = neighborTileValue - currentTileValue;
				if (delta < minDelta)
				{
					flowDirection = directions[i];
					minDelta = delta;
				}
			}
			m_flowField->SetValueAtCoords(currentTileCoords, Vec2(flowDirection).GetNormalized());
		}
	}
}

void Game2DFlowField::SpawnActor()
{
	int numStarts = (int)m_starts.size();
	if (numStarts == 0)
	{
		return;
	}
	int spawnIndex = g_rng.RollRandomIntInRange(0, numStarts - 1);

	FlowFieldActor2D actor;
	actor.m_position = GetTileCenter(m_starts[spawnIndex].x, m_starts[spawnIndex].y);
	actor.m_speed = g_rng.RollRandomFloatInRange(MIN_ACTOR_SPEED, MAX_ACTOR_SPEED);

	m_actors.push_back(actor);
}

void Game2DFlowField::UpdateActors()
{
	float deltaSeconds = (float)m_clock->GetDeltaSeconds();
	for (FlowFieldActor2D& actor : m_actors)
	{
		Vec2 direction = GetBilinearInterpResultFromWorldPos(actor.m_position);
		actor.m_position += direction * actor.m_speed * deltaSeconds;
		actor.m_orientationDegrees = direction.GetOrientationDegrees();
	}
}

void Game2DFlowField::TeleportActors()
{
	int numStarts = (int)m_starts.size();
	if (numStarts == 0)
	{
		return;
	}

	for (FlowFieldActor2D& actor : m_actors)
	{
		for (IntVec2 const& coords : m_ends)
		{
			if (GetDistanceSquared2D(GetTileCenter(coords.x, coords.y), actor.m_position) <= m_endRadius * m_endRadius)
			{
				int teleportIndex = g_rng.RollRandomIntInRange(0, numStarts - 1);
				actor.m_position = GetTileCenter(m_starts[teleportIndex].x, m_starts[teleportIndex].y);
				break;
			}
		}
	}
}

void Game2DFlowField::DrawDistanceMap() const
{
	std::vector<Vertex_PCU> verts;

	Vec2 dimensions = Vec2(static_cast<float>(m_gridDimensions.x) * m_cellSize.x, static_cast<float>(m_gridDimensions.y) * m_cellSize.y);

	m_distanceMap->AddVertsForDebugDraw(verts, AABB2(m_gridOrigin, m_gridOrigin + dimensions), 
		m_distanceMap->GetRangeOffValuesExcludingSpecial(SPECIAL_VALUE), Rgba8(0, 0, 0), Rgba8(255, 255, 255),
		SPECIAL_VALUE);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game2DFlowField::DrawFlowField() const
{
	std::vector<Vertex_PCU> verts;

	Vec2 dimensions = Vec2(static_cast<float>(m_gridDimensions.x) * m_cellSize.x, static_cast<float>(m_gridDimensions.y) * m_cellSize.y);

	m_flowField->AddVertsForDebugDraw(verts, AABB2(m_gridOrigin, m_gridOrigin + dimensions));

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game2DFlowField::DrawSolidMap() const
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


void Game2DFlowField::DrawTileGrid() const
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

void Game2DFlowField::DrawStartsAndEnds() const
{
	std::vector<Vertex_PCU> verts;

	for (IntVec2 const& coords : m_starts)
	{
		AddVertsForDisc2D(verts, GetTileCenter(coords.x, coords.y), m_startRadius, Rgba8(255, 127, 39), 16);
	}

	for (IntVec2 const& coords : m_ends)
	{
		AddVertsForDisc2D(verts, GetTileCenter(coords.x, coords.y), m_endRadius, Rgba8(181, 230, 29), 16);
	}

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game2DFlowField::DrawActors() const
{
	std::vector<Vertex_PCU> verts;

	float actorRadius = m_cellSize.x * 0.5f;
	float actorArrowSize = m_cellSize.x * 0.3f;
	float actorArrowWidth = m_cellSize.x * 0.1f;
	for (FlowFieldActor2D const& actor : m_actors)
	{
		AddVertsForDisc2D(verts, actor.m_position, actorRadius, Rgba8::CYAN, 16);
		Vec2 halfArrow = Vec2::MakeFromPolarDegrees(actor.m_orientationDegrees, actorRadius * 0.8f);
		AddVertsForArrow2D(verts, actor.m_position - halfArrow, actor.m_position + halfArrow, actorArrowSize, actorArrowWidth, Rgba8(34, 126, 255));
	}


	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

bool Game2DFlowField::IsTileSolid(int tileX, int tileY) const
{
	IntVec2 const tileCoords = IntVec2(tileX, tileY);
	if (!m_solidMap->IsInBounds(tileCoords))
	{
		return false;
	}

	return m_solidMap->GetValueAtCoords(tileCoords) == SOLID_VALUE;
}

bool Game2DFlowField::IsWorldPosInBounds(Vec2 const& worldPos) const
{
	IntVec2 const tileCoords = GetTileCoordsForWorldPos(worldPos);
	return m_solidMap->IsInBounds(tileCoords);
}

IntVec2 Game2DFlowField::GetTileCoordsForWorldPos(Vec2 const& worldPos) const
{
	Vec2 disp = worldPos - m_gridOrigin;

	int tileX = RoundDownToInt(disp.x / m_cellSize.x);
	int tileY = RoundDownToInt(disp.y / m_cellSize.y);

	return IntVec2(tileX, tileY);
}

Vec2 Game2DFlowField::GetTileCenter(int tileX, int tileY) const
{
	return Vec2(static_cast<float>(tileX) + 0.5f, static_cast<float>(tileY) + 0.5f)* m_cellSize + m_gridOrigin;
}

void Game2DFlowField::SpreadDistanceMapHeat(TileHeatMap& distanceMap, float startSearchValue, float heatSpreadStep /*= 1.f*/)
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

Vec2 Game2DFlowField::GetBilinearInterpResultFromWorldPos(Vec2 const& worldPos) const
{
	Vec2 disp = worldPos - m_gridOrigin;
	Vec2 normalizedPos = Vec2(disp.x / m_cellSize.x, disp.y / m_cellSize.y);
	
	Vec2 posAtBottomLeftTileCoords = (normalizedPos - Vec2(0.5f, 0.5f));
	IntVec2 bottomLeftTileCoords = IntVec2(RoundDownToInt(posAtBottomLeftTileCoords.x), RoundDownToInt(posAtBottomLeftTileCoords.y));
	IntVec2 topLeftTileCoords = bottomLeftTileCoords	+ IntVec2(0, 1);
	IntVec2 bottmRightTileCoords = bottomLeftTileCoords + IntVec2(1, 0);
	IntVec2 topRightTileCoords = bottomLeftTileCoords	+ IntVec2(1, 1);

	Vec2 value00 = GetSafeValueFromFlowField(bottomLeftTileCoords);
	Vec2 value01 = GetSafeValueFromFlowField(topLeftTileCoords);
	Vec2 value10 = GetSafeValueFromFlowField(bottmRightTileCoords);
	Vec2 value11 = GetSafeValueFromFlowField(topRightTileCoords);

	float xWeight = posAtBottomLeftTileCoords.x - floorf(posAtBottomLeftTileCoords.x); // 0~1
	float yWeight = posAtBottomLeftTileCoords.y - floorf(posAtBottomLeftTileCoords.y);

	Vec2 topDirection = Interpolate(value01, value11, xWeight);
	Vec2 bottomDirection = Interpolate(value00, value10, xWeight);

	Vec2 resultDirection = Interpolate(bottomDirection, topDirection, yWeight);

	return resultDirection.GetNormalized();
}

Vec2 Game2DFlowField::GetSafeValueFromFlowField(IntVec2 tileCoords) const
{
	// It can not become engine code, need to be written every time

	if (m_flowField->IsInBounds(tileCoords))
	{
		return m_flowField->GetValueAtCoords(tileCoords);
	}

	// Out of Bounds: Has default value
	IntVec2 dimensions = m_flowField->m_dimensions;

	if (tileCoords.x == -1 && tileCoords.y == -1)
	{
		return Vec2(1.f, 1.f).GetNormalized();
	}
	if (tileCoords.x == -1 && tileCoords.y == dimensions.y)
	{
		return Vec2(1.f, -1.f).GetNormalized();
	}
	if (tileCoords.x == dimensions.x && tileCoords.y == -1)
	{
		return Vec2(-1.f, 1.f).GetNormalized();
	}
	if (tileCoords.x == dimensions.x && tileCoords.y == dimensions.y)
	{
		return Vec2(-1.f, -1.f).GetNormalized();
	}

	if (tileCoords.x == -1 && (tileCoords.y >= 0) && (tileCoords.y < dimensions.y))
	{
		return Vec2(1.f, 0.f);
	}
	if (tileCoords.x == dimensions.x && (tileCoords.y >= 0) && (tileCoords.y < dimensions.y))
	{
		return Vec2(-1.f, 0.f);
	}
	if (tileCoords.y == -1 && (tileCoords.x >= 0) && (tileCoords.x < dimensions.x))
	{
		return Vec2(0.f, 1.f);
	}
	if (tileCoords.y == dimensions.y && (tileCoords.x >= 0) && (tileCoords.x < dimensions.x))
	{
		return Vec2(0.f, -1.f);
	}

	return Vec2::ZERO;
}
