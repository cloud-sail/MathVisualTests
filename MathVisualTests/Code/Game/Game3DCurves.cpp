#include "Game/Game3DCurves.hpp"
#include "Engine/Core/DebugRender.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

static const char* G3C_TEXT = "Splines (3D):";


//-----------------------------------------------------------------------------------------------
Game3DCurves::Game3DCurves()
{
	m_cursorMode = CursorMode::FPS;

	m_camera.SetPerspectiveView(WINDOW_ASPECT, 60.f, 0.1f, 100.0f);
	m_camera.SetCameraToRenderTransform(Mat44::DIRECTX_C2R);
	DebugAddBasis(Mat44(), -1.f, 1.f, 0.05f);


	InitializeModel();

	constexpr float BOX_SIZE = 5.f;

	int boxNum = (int)(sizeof(m_splineBoxs) / sizeof(m_splineBoxs[0]));
	for (int i = 0; i < boxNum; ++i)
	{
		float index = (float)i;
		Vec3 mins = Vec3(index * BOX_SIZE * 4.5f, 0.f, 0.f);
		m_splineBoxs[i] = AABB3(mins, mins + Vec3(BOX_SIZE * 4.f, BOX_SIZE, BOX_SIZE * 2.f));
	}



	RandomizeSceneObjects();
}

Game3DCurves::~Game3DCurves()
{
	DebugRenderClear();
}

void Game3DCurves::Update()
{
	UpdateDeveloperCheats();

	HandleInput();
	UpdatePlayer();

	UpdateCameras();
}

void Game3DCurves::Render() const
{
	g_theRenderer->BeginCamera(m_camera);
	DrawSplines();
	DrawObjects();
	g_theRenderer->EndCamera(m_camera);
	DebugRenderWorld(m_camera);

	g_theRenderer->BeginCamera(m_screenCamera);
	DrawUsage();
	g_theRenderer->EndCamera(m_screenCamera);
	DebugRenderScreen(m_screenCamera);
}

void Game3DCurves::RandomizeSceneObjects()
{
	// First Spline
	int numPoints = g_rng.RollRandomIntInRange(5, 10);
	std::vector<Vec3> spline1Points;
	spline1Points.reserve(numPoints);
	for (int i = 0; i < numPoints; ++i)
	{
		spline1Points.push_back(GetRandomPosInsideBox(m_splineBoxs[0]));
	}
	BubbleSort(spline1Points);

	m_spline1.ClearAllSplinePoints();
	m_spline1.SetFromCatmullRomAlgorithm(spline1Points, true);

	for (int i = 0; i < numPoints; ++i)
	{
		m_spline1.SetRotationAtSplinePoint(i, EulerAngles(g_rng.RollRandomFloatInRange(0.f, 360.f), g_rng.RollRandomFloatInRange(-85.f, 85.f), g_rng.RollRandomFloatInRange(0.f, 360.f)), false);
		m_spline1.SetScaleAtSplinePoint(i, Vec3(g_rng.RollRandomFloatInRange(0.5f, 2.5f), g_rng.RollRandomFloatInRange(0.5f, 2.5f), g_rng.RollRandomFloatInRange(0.5f, 2.5f)), false);
	}


	m_spline1.UpdateSpline();
}

void Game3DCurves::UpdateCameras()
{
	m_camera.SetPositionAndOrientation(m_playerPosition, m_playerOrientation);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game3DCurves::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	std::string usageText = Stringf(G3C_TEXT);
	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game3DCurves::UpdatePlayer()
{
	float deltaSeconds = static_cast<float>(m_clock->GetDeltaSeconds());
	UpdatePlayerPosition(deltaSeconds);
	UpdatePlayerOrientation();

	UpdatePlayerCameraCenterAxis();

}

void Game3DCurves::UpdatePlayerPosition(float deltaSeconds)
{
	Vec3 moveIntention;
	if (g_theInput->IsKeyDown(KEYCODE_W))
	{
		moveIntention += Vec3(1.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_S))
	{
		moveIntention += Vec3(-1.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_A))
	{
		moveIntention += Vec3(0.f, 1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_D))
	{
		moveIntention += Vec3(0.f, -1.f, 0.f);
	}
	moveIntention.ClampLength(1.f);

	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_playerOrientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);
	Vec3 forwardNormal = Vec3(forwardIBasis.x, forwardIBasis.y, 0.f).GetNormalized();
	Vec3 leftNormal = Vec3(-forwardNormal.y, forwardNormal.x, 0.f);
	m_playerPosition += (forwardNormal * moveIntention.x + leftNormal * moveIntention.y) * CAMERA_MOVE_SPEED * deltaSeconds;


	Vec3 elevateIntention;
	if (g_theInput->IsKeyDown(KEYCODE_Q))
	{
		elevateIntention += Vec3(0.f, 0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_E))
	{
		elevateIntention += Vec3(0.f, 0.f, 1.f);
	}
	elevateIntention.ClampLength(1.f);

	m_playerPosition += elevateIntention * CAMERA_MOVE_SPEED * deltaSeconds;

}

void Game3DCurves::UpdatePlayerOrientation()
{
	Vec2 cursorPositionDelta = g_theInput->GetCursorClientDelta();
	float deltaYaw = -cursorPositionDelta.x * 0.125f;
	float deltaPitch = cursorPositionDelta.y * 0.125f;

	m_playerOrientation.m_yawDegrees += deltaYaw;
	m_playerOrientation.m_pitchDegrees += deltaPitch;
	m_playerOrientation.m_pitchDegrees = GetClamped(m_playerOrientation.m_pitchDegrees, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);
}

void Game3DCurves::UpdatePlayerCameraCenterAxis()
{
	// Add camera center axis
	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_playerOrientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);
	Mat44 cameraCenterAxisTransform;
	cameraCenterAxisTransform.SetTranslation3D(m_playerPosition + 0.2f * forwardIBasis);
	DebugAddBasis(cameraCenterAxisTransform, 0.f, 0.006f, 0.0003f);
}

void Game3DCurves::HandleInput()
{

}

void Game3DCurves::InitializeModel()
{
	m_modelTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");

	m_modelVerts.reserve(36);
	//Vec3 halfDimensions(0.5f, 0.5f, 0.5f);
	//AddVertsForAABB3D(m_modelVerts, AABB3(-halfDimensions, halfDimensions));
	//AddVertsForColoredCube3D(m_modelVerts);
	AddVertsForColoredAABB3D(m_modelVerts, AABB3(Vec3(-0.3f, -0.3f, -0.3f), Vec3(0.3f, 0.3f, 0.3f)));
}

float Game3DCurves::GetCurrentFraction(int numSegment /*= 1*/) const
{
	constexpr float FREQUENCY = 0.5f;
	float currentTime = static_cast<float>(m_clock->GetTotalSeconds());
	return  fmodf((currentTime * FREQUENCY), (float)numSegment);
}

Vec3 Game3DCurves::GetRandomPosInsideBox(AABB3 const& box) const
{
	return GetPositionInBoxCoords(box, GetRandomVec3FromZeroToOne());
}

Vec3 Game3DCurves::GetPositionInBoxCoords(AABB3 const& box, Vec3 const& point) const
{
	return Vec3(Interpolate(box.m_mins.x, box.m_maxs.x, point.x),
				Interpolate(box.m_mins.y, box.m_maxs.y, point.y), 
				Interpolate(box.m_mins.z, box.m_maxs.z, point.z));
}

Vec3 Game3DCurves::GetRandomVec3FromZeroToOne() const
{
	return Vec3(g_rng.RollRandomFloatZeroToOne(), g_rng.RollRandomFloatZeroToOne(), g_rng.RollRandomFloatZeroToOne());
}

void Game3DCurves::BubbleSort(std::vector<Vec3>& points)
{
	int n = (int)points.size();
	bool swapped;

	for (int i = 0; i < n - 1; ++i)
	{
		swapped = false;
		for (int j = 0; j < n - i - 1; ++j) {
			if (points[j].x > points[j + 1].x) {
				Vec3 temp = points[j];
				points[j] = points[j + 1];
				points[j + 1] = temp;

				swapped = true;
			}
		}

		if (!swapped) {
			break;
		}
	}
}

void Game3DCurves::DrawSplines() const
{
	std::vector<Vertex_PCU> splineVerts;

	std::vector<Vec3> splinePoints;
	m_spline1.GetPositionListWithSubdivisions(splinePoints, 10);
	AddVertsForScratchyLines(splineVerts, splinePoints, 0.1f, DARK_BLUE);
	m_spline1.GetPositionListWithSubdivisions(splinePoints, 1);
	for (int i = 0; i < (int)splinePoints.size(); ++i)
	{
		AddVertsForSphere3D(splineVerts, splinePoints[i], 0.15f, ORANGE, AABB2::ZERO_TO_ONE, 8, 4);
	}

	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->DrawVertexArray(splineVerts);
}

void Game3DCurves::DrawObjects() const
{
	g_theRenderer->BindTexture(m_modelTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	Mat44 modelTransform;

	float inputKey1 = GetCurrentFraction(m_spline1.GetNumberOfSplineSegments());
	Game3DCurvesTransform transform1;
	transform1.m_position = m_spline1.GetPositionAtInputKey(inputKey1);
	transform1.m_rotation = m_spline1.GetQuaternionAtInputKey(inputKey1);
	transform1.m_scale = m_spline1.GetScaleAtInputKey(inputKey1);

	// Translation Rotation Scale
	modelTransform = Mat44::MakeFromUnitQuat(transform1.m_rotation);
	modelTransform.AppendScaleNonUniform3D(transform1.m_scale);
	modelTransform.SetTranslation3D(transform1.m_position);

	g_theRenderer->SetModelConstants(modelTransform);
	g_theRenderer->DrawVertexArray(m_modelVerts);
}

