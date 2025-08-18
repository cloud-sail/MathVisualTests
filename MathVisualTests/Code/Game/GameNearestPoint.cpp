#include "Game/GameNearestPoint.hpp"
#include "Game/App.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

//-----------------------------------------------------------------------------------------------
static constexpr float GNP_MOVESPEED = 200.f;
static constexpr float GNP_WHITE_DOT_RADIUS = 3.f;
static constexpr float GNP_ORANGE_DOT_RADIUS = 6.f;
static constexpr float GNP_LINE_THICKNESS = 3.f;

static const std::string GNP_TEXT = "GameNearestPoint: use ESDF / Arrow Keys / LMB to move the white point";

//-----------------------------------------------------------------------------------------------
GameNearestPoint::GameNearestPoint()
{
	m_whiteDotPos = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);

	m_shapeList.reserve(7);
	Vec2 sceneDimensions = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);
	m_shapeList.push_back(new GNPO_Disc(sceneDimensions));
	m_shapeList.push_back(new GNPO_Triangle(sceneDimensions));
	m_shapeList.push_back(new GNPO_AABB2(sceneDimensions));
	m_shapeList.push_back(new GNPO_OBB2(sceneDimensions));
	m_shapeList.push_back(new GNPO_Capsule2(sceneDimensions));
	m_shapeList.push_back(new GNPO_LineSegment2(sceneDimensions));
	m_shapeList.push_back(new GNPO_InfiniteLine(sceneDimensions));
}

GameNearestPoint::~GameNearestPoint()
{
	for (GNPObject* ptr : m_shapeList) {
		delete ptr;
	}
	m_shapeList.clear();
}

void GameNearestPoint::Update()
{
	UpdateDeveloperCheats();

	Vec2 direction = Vec2();

	if (g_theInput->IsKeyDown(KEYCODE_LEFT) || g_theInput->IsKeyDown(KEYCODE_S))
	{
		direction += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_RIGHT) || g_theInput->IsKeyDown(KEYCODE_F))
	{
		direction += Vec2(1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_UP) || g_theInput->IsKeyDown(KEYCODE_E))
	{
		direction += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_DOWN) || g_theInput->IsKeyDown(KEYCODE_D))
	{
		direction += Vec2(0.f, -1.f);
	}

	float deltaSeconds = static_cast<float>(m_clock->GetDeltaSeconds());
	m_whiteDotPos += direction * GNP_MOVESPEED * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		m_whiteDotPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
	}


	UpdateCameras();
}

void GameNearestPoint::Render() const
{
	g_theRenderer->BeginCamera(m_camera);

	DrawObjects();
	DrawUsage();

	g_theRenderer->EndCamera(m_camera);
}

void GameNearestPoint::UpdateCameras()
{
	m_camera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void GameNearestPoint::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + GNP_TEXT, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::DrawObjects() const
{
	std::vector<Vertex_PCU> verts;

	for (GNPObject* shape : m_shapeList) {
		shape->AddVerts(verts, (shape->IsPointInside(m_whiteDotPos)) ? LIGHT_BLUE : DARK_BLUE);
	}

	for (GNPObject* shape : m_shapeList) {
		Vec2 nearestPoint = shape->GetNearestPoint(m_whiteDotPos);
		AddVertsForDisc2D(verts, nearestPoint, GNP_ORANGE_DOT_RADIUS, ORANGE);
	}

	// White dot
	AddVertsForDisc2D(verts, m_whiteDotPos, GNP_WHITE_DOT_RADIUS, Rgba8::OPAQUE_WHITE);

	for (GNPObject* shape : m_shapeList) {
		Vec2 nearestPoint = shape->GetNearestPoint(m_whiteDotPos);
		AddVertsForLineSegment2D(verts, nearestPoint, m_whiteDotPos, GNP_LINE_THICKNESS, TRANSLUCENT_WHITE);
	}

	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::RandomizeSceneObjects()
{
	for (GNPObject* ptr : m_shapeList) {
		delete ptr;
	}
	m_shapeList.clear();

	m_shapeList.reserve(7);
	Vec2 sceneDimensions = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);
	m_shapeList.push_back(new GNPO_Disc(sceneDimensions));
	m_shapeList.push_back(new GNPO_Triangle(sceneDimensions));
	m_shapeList.push_back(new GNPO_AABB2(sceneDimensions));
	m_shapeList.push_back(new GNPO_OBB2(sceneDimensions));
	m_shapeList.push_back(new GNPO_Capsule2(sceneDimensions));
	m_shapeList.push_back(new GNPO_LineSegment2(sceneDimensions));
	m_shapeList.push_back(new GNPO_InfiniteLine(sceneDimensions));
}

//-----------------------------------------------------------------------------------------------
GNPO_Disc::GNPO_Disc(Vec2 const& sceneDimensions)
{
	m_discRadius = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.05f, sceneDimensions.y * 0.1f);
	m_discCenter.x = g_rng.RollRandomFloatInRange(m_discRadius, sceneDimensions.x - m_discRadius);
	m_discCenter.y = g_rng.RollRandomFloatInRange(m_discRadius, sceneDimensions.y - m_discRadius);
}

bool GNPO_Disc::IsPointInside(Vec2 const& point) const
{
	return IsPointInsideDisc2D(point, m_discCenter, m_discRadius);
}

Vec2 GNPO_Disc::GetNearestPoint(Vec2 const& ref) const
{
	return GetNearestPointOnDisc2D(ref, m_discCenter, m_discRadius);
}

void GNPO_Disc::AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	AddVertsForDisc2D(verts, m_discCenter, m_discRadius, color);
}

//-----------------------------------------------------------------------------------------------
GNPO_Triangle::GNPO_Triangle(Vec2 const& sceneDimensions)
	: m_triangle(Vec2(), Vec2(), Vec2())
{
	float discRadius = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.1f, sceneDimensions.y * 0.15f);
	Vec2 discCenter;
	discCenter.x = g_rng.RollRandomFloatInRange(discRadius, sceneDimensions.x - discRadius);
	discCenter.y = g_rng.RollRandomFloatInRange(discRadius, sceneDimensions.y - discRadius);

	float angle0 = g_rng.RollRandomFloatInRange(0.f, 60.f);
	float angle2 = g_rng.RollRandomFloatInRange(300.f, 360.f);

	float angle1 = g_rng.RollRandomFloatInRange(angle0, angle2);

	m_triangle = Triangle2(	discCenter + Vec2(discRadius * CosDegrees(angle0), discRadius * SinDegrees(angle0)),
							discCenter + Vec2(discRadius * CosDegrees(angle1), discRadius * SinDegrees(angle1)), 
							discCenter + Vec2(discRadius * CosDegrees(angle2), discRadius * SinDegrees(angle2)));
}

bool GNPO_Triangle::IsPointInside(Vec2 const& point) const
{
	return IsPointInsideTriangle2D(point, m_triangle);
}

Vec2 GNPO_Triangle::GetNearestPoint(Vec2 const& ref) const
{
	return GetNearestPointOnTriangle2D(ref, m_triangle);
}

void GNPO_Triangle::AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	AddVertsForTriangle2D(verts, m_triangle, color);
}

//-----------------------------------------------------------------------------------------------
GNPO_AABB2::GNPO_AABB2(Vec2 const& sceneDimensions)
{
	float ratio = 0.1f;
	Vec2 center;
	center.x = g_rng.RollRandomFloatInRange(sceneDimensions.x * ratio, sceneDimensions.x * (1.f - ratio));
	center.y = g_rng.RollRandomFloatInRange(sceneDimensions.y * ratio, sceneDimensions.y * (1.f - ratio));
	m_box.SetCenter(center);

	Vec2 dimension;
	dimension.x = g_rng.RollRandomFloatInRange(sceneDimensions.x * ratio * 0.3f, sceneDimensions.x * ratio);
	dimension.y = g_rng.RollRandomFloatInRange(sceneDimensions.y * ratio * 0.3f, sceneDimensions.y * ratio);
	m_box.SetDimensions(dimension);
}

bool GNPO_AABB2::IsPointInside(Vec2 const& point) const
{
	return IsPointInsideAABB2D(point, m_box);
}

Vec2 GNPO_AABB2::GetNearestPoint(Vec2 const& ref) const
{
	return m_box.GetNearestPoint(ref);
}

void GNPO_AABB2::AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	AddVertsForAABB2D(verts, m_box, color);
}

//-----------------------------------------------------------------------------------------------
GNPO_OBB2::GNPO_OBB2(Vec2 const& sceneDimensions)
{
	float ratio = 0.1f;

	m_obb.m_center.x = g_rng.RollRandomFloatInRange(sceneDimensions.x * ratio, sceneDimensions.x * (1.f - ratio));
	m_obb.m_center.y = g_rng.RollRandomFloatInRange(sceneDimensions.y * ratio, sceneDimensions.y * (1.f - ratio));

	m_obb.m_halfDimensions.x = g_rng.RollRandomFloatInRange(sceneDimensions.x * ratio * 0.3f, sceneDimensions.x * ratio);
	m_obb.m_halfDimensions.y = g_rng.RollRandomFloatInRange(sceneDimensions.y * ratio * 0.3f, sceneDimensions.y * ratio);

	float angle = g_rng.RollRandomFloatInRange(0.f, 360.f);
	m_obb.m_iBasisNormal = Vec2::MakeFromPolarDegrees(angle);
}

bool GNPO_OBB2::IsPointInside(Vec2 const& point) const
{
	return IsPointInsideOBB2D(point, m_obb);
}

Vec2 GNPO_OBB2::GetNearestPoint(Vec2 const& ref) const
{
	return GetNearestPointOnOBB2D(ref, m_obb);
}

void GNPO_OBB2::AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	AddVertsForOBB2D(verts, m_obb, color);
}

//-----------------------------------------------------------------------------------------------
GNPO_Capsule2::GNPO_Capsule2(Vec2 const& sceneDimensions)
{
	float discRadius = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.05f, sceneDimensions.y * 0.1f);
	Vec2 discCenter;
	discCenter.x = g_rng.RollRandomFloatInRange(discRadius, sceneDimensions.x - discRadius);
	discCenter.y = g_rng.RollRandomFloatInRange(discRadius, sceneDimensions.y - discRadius);

	float angle = g_rng.RollRandomFloatInRange(0.f, 360.f);

	Vec2 direction = Vec2::MakeFromPolarDegrees(angle);
	m_capsule.m_bone.m_start = discCenter + discRadius * direction;
	m_capsule.m_bone.m_end = discCenter - discRadius * direction;
	m_capsule.m_radius = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.02f, sceneDimensions.y * 0.06f);
}

bool GNPO_Capsule2::IsPointInside(Vec2 const& point) const
{
	return IsPointInsideCapsule2D(point, m_capsule);
}

Vec2 GNPO_Capsule2::GetNearestPoint(Vec2 const& ref) const
{
	return GetNearestPointOnCapsule2D(ref, m_capsule);
}

void GNPO_Capsule2::AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	AddVertsForCapsule2D(verts, m_capsule, color);
}

//-----------------------------------------------------------------------------------------------
GNPO_LineSegment2::GNPO_LineSegment2(Vec2 const& sceneDimensions)
{
	float discRadius = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.05f, sceneDimensions.y * 0.15f);
	Vec2 discCenter;
	discCenter.x = g_rng.RollRandomFloatInRange(discRadius, sceneDimensions.x - discRadius);
	discCenter.y = g_rng.RollRandomFloatInRange(discRadius, sceneDimensions.y - discRadius);

	float angle = g_rng.RollRandomFloatInRange(0.f, 360.f);

	Vec2 direction = Vec2::MakeFromPolarDegrees(angle);
	m_lineSegment.m_start = discCenter + discRadius * direction;
	m_lineSegment.m_end = discCenter - discRadius * direction;
}

bool GNPO_LineSegment2::IsPointInside(Vec2 const& point) const
{
	UNUSED(point);
	return false;
}

Vec2 GNPO_LineSegment2::GetNearestPoint(Vec2 const& ref) const
{
	return GetNearestPointOnLineSegment2D(ref, m_lineSegment);
}

void GNPO_LineSegment2::AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	AddVertsForLineSegment2D(verts, m_lineSegment, GNP_LINE_THICKNESS, color);
}

//-----------------------------------------------------------------------------------------------
GNPO_InfiniteLine::GNPO_InfiniteLine(Vec2 const& sceneDimensions)
{
	Vec2 point;
	point.x = g_rng.RollRandomFloatInRange(sceneDimensions.x * 0.3f, sceneDimensions.x * 0.7f);
	point.y = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.3f, sceneDimensions.y * 0.7f);

	float angle = g_rng.RollRandomFloatInRange(0.f, 360.f);
	Vec2 direction = Vec2::MakeFromPolarDegrees(angle);

	m_infiniteLine.m_start = point + 3000.f * direction;
	m_infiniteLine.m_end = point - 3000.f * direction;
}

bool GNPO_InfiniteLine::IsPointInside(Vec2 const& point) const
{
	UNUSED(point);
	return false;
}

Vec2 GNPO_InfiniteLine::GetNearestPoint(Vec2 const& ref) const
{
	return GetNearestPointOnInfiniteLine2D(ref, m_infiniteLine);
}

void GNPO_InfiniteLine::AddVerts(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	AddVertsForLineSegment2D(verts, m_infiniteLine, GNP_LINE_THICKNESS, color);
}
