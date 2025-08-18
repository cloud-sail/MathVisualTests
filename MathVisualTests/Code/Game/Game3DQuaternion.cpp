#include "Game/Game3DQuaternion.hpp"
#include "Engine/Core/DebugRender.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

static const char* G2PACHINKO_TEXT = "Quaternion (3D):";


//-----------------------------------------------------------------------------------------------
Game3DQuaternion::Game3DQuaternion()
{
	m_cursorMode = CursorMode::FPS;

	m_camera.SetPerspectiveView(WINDOW_ASPECT, 60.f, 0.1f, 100.0f);
	m_camera.SetCameraToRenderTransform(Mat44::DIRECTX_C2R);
	DebugAddBasis(Mat44(), -1.f, 1.f, 0.05f);

	InitializeModel();
	InitializeTransforms();

	RandomizeSceneObjects();
}

Game3DQuaternion::~Game3DQuaternion()
{
	DebugRenderClear();
}

void Game3DQuaternion::Update()
{
	UpdateDeveloperCheats();

	HandleInput();
	UpdatePlayer();
	UpdateObjects();

	UpdateCameras();
}

void Game3DQuaternion::Render() const
{
	g_theRenderer->BeginCamera(m_camera);
	DrawObjects();
	g_theRenderer->EndCamera(m_camera);
	DebugRenderWorld(m_camera);

	g_theRenderer->BeginCamera(m_screenCamera);
	DrawUsage();
	g_theRenderer->EndCamera(m_screenCamera);
	DebugRenderScreen(m_screenCamera);
}

void Game3DQuaternion::RandomizeSceneObjects()
{
	// Randomize the quaternion list
	int size = (int)(sizeof(m_quatList) / sizeof(m_quatList[0]));
	for (int i = 0; i < size; ++i)
	{
		m_quatList[i] = Quat(g_rng.RollRandomFloatInRange(-1.f, 1.f), g_rng.RollRandomFloatInRange(-1.f, 1.f), g_rng.RollRandomFloatInRange(-1.f, 1.f), g_rng.RollRandomFloatInRange(-1.f, 1.f));
		m_quatList[i].Normalize();
	}
}

void Game3DQuaternion::UpdateCameras()
{
	m_camera.SetPositionAndOrientation(m_playerPosition, m_playerOrientation);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game3DQuaternion::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	std::string usageText = Stringf(G2PACHINKO_TEXT);
	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game3DQuaternion::UpdatePlayer()
{
	float deltaSeconds = static_cast<float>(m_clock->GetDeltaSeconds());
	UpdatePlayerPosition(deltaSeconds);
	UpdatePlayerOrientation();

	UpdatePlayerCameraCenterAxis();

}

void Game3DQuaternion::UpdatePlayerPosition(float deltaSeconds)
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

void Game3DQuaternion::UpdatePlayerOrientation()
{
	Vec2 cursorPositionDelta = g_theInput->GetCursorClientDelta();
	float deltaYaw = -cursorPositionDelta.x * 0.125f;
	float deltaPitch = cursorPositionDelta.y * 0.125f;

	m_playerOrientation.m_yawDegrees += deltaYaw;
	m_playerOrientation.m_pitchDegrees += deltaPitch;
	m_playerOrientation.m_pitchDegrees = GetClamped(m_playerOrientation.m_pitchDegrees, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);
}

void Game3DQuaternion::UpdatePlayerCameraCenterAxis()
{
	// Add camera center axis
	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_playerOrientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);
	Mat44 cameraCenterAxisTransform;
	cameraCenterAxisTransform.SetTranslation3D(m_playerPosition + 0.2f * forwardIBasis);
	DebugAddBasis(cameraCenterAxisTransform, 0.f, 0.006f, 0.0003f);
}

void Game3DQuaternion::InitializeModel()
{
	m_modelTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");

	m_modelVerts.reserve(36);
	//Vec3 halfDimensions(0.5f, 0.5f, 0.5f);
	//AddVertsForAABB3D(m_modelVerts, AABB3(-halfDimensions, halfDimensions));
	//AddVertsForColoredCube3D(m_modelVerts);
	AddVertsForColoredAABB3D(m_modelVerts, AABB3(Vec3(0.f, -0.07f, -0.07f), Vec3(0.7f, 0.07f, 0.07f)));
}

void Game3DQuaternion::InitializeTransforms()
{
	constexpr float MODEL_SPACING = 1.8f;
	const Vec3 LABEL_OFFSET = Vec3(0.f, 0.f, 1.f);
	int index = 1;

	m_lerp.m_position = Vec3::FORWARD * (static_cast<float>(index) * MODEL_SPACING);
	DebugAddWorldBillboardText("Lerp", m_lerp.m_position + LABEL_OFFSET, 0.125f, -1.f, 0.7f, Vec2(0.5f, 0.5f));
	index++;

	m_nlerp.m_position = Vec3::FORWARD * (static_cast<float>(index) * MODEL_SPACING);
	DebugAddWorldBillboardText("Nlerp", m_nlerp.m_position + LABEL_OFFSET, 0.125f, -1.f, 0.7f, Vec2(0.5f, 0.5f));
	index++;

	m_slerp.m_position = Vec3::FORWARD * (static_cast<float>(index) * MODEL_SPACING);
	DebugAddWorldBillboardText("Slerp", m_slerp.m_position + LABEL_OFFSET, 0.125f, -1.f, 0.7f, Vec2(0.5f, 0.5f));
	index++;

	m_nlerpFullPath.m_position = Vec3::FORWARD * (static_cast<float>(index) * MODEL_SPACING);
	DebugAddWorldBillboardText("Nlerp Full Path", m_nlerpFullPath.m_position + LABEL_OFFSET, 0.125f, -1.f, 0.7f, Vec2(0.5f, 0.5f));
	index++;

	m_slerpFullPath.m_position = Vec3::FORWARD * (static_cast<float>(index) * MODEL_SPACING);
	DebugAddWorldBillboardText("Slerp Full Path", m_slerpFullPath.m_position + LABEL_OFFSET, 0.125f, -1.f, 0.7f, Vec2(0.5f, 0.5f));
	index++;
}

void Game3DQuaternion::UpdateObjects()
{
	constexpr float FREQUENCY = 0.4f;
	int size = (int)(sizeof(m_quatList) / sizeof(m_quatList[0]));
	float currentSeconds = (float)m_clock->GetTotalSeconds();
	float currentKey = fmodf(currentSeconds * FREQUENCY, (float)(size - 1));
	int currentIndex = RoundDownToInt(currentKey);
	float t = currentKey - (float)currentIndex;
	Quat q0 = m_quatList[currentIndex];
	Quat q1 = m_quatList[currentIndex + 1];

	m_lerp.m_rotation			= Quat::Lerp(q0, q1, t);
	m_nlerp.m_rotation			= Quat::Nlerp(q0, q1, t);
	m_slerp.m_rotation			= Quat::Slerp(q0, q1, t);
	m_nlerpFullPath.m_rotation	= Quat::Nlerp(q0, q1, t, false);
	m_slerpFullPath.m_rotation	= Quat::SlerpFullPath(q0, q1, t);


	// Debug Axis
	constexpr float AXIS_RADIUS = 0.02f;
	const Rgba8 START_COLOR = Rgba8::GREEN;
	const Rgba8 END_COLOR = Rgba8::BLUE;
	const Rgba8 LERP_COLOR = Rgba8::CYAN;

	Vec3 const startAxis = q0.GetRotationAxis();
	Vec3 const endAxis = q1.GetRotationAxis();

	DebugAddWorldArrow(m_lerp.m_position, startAxis + m_lerp.m_position, AXIS_RADIUS, 0.f, START_COLOR, START_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_lerp.m_position, endAxis + m_lerp.m_position, AXIS_RADIUS, 0.f, END_COLOR, END_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_lerp.m_position, m_lerp.m_rotation.GetRotationAxis() + m_lerp.m_position, AXIS_RADIUS, 0.f, LERP_COLOR, LERP_COLOR, DebugRenderMode::X_RAY);

	DebugAddWorldArrow(m_nlerp.m_position, startAxis + m_nlerp.m_position, AXIS_RADIUS, 0.f, START_COLOR, START_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_nlerp.m_position, endAxis + m_nlerp.m_position, AXIS_RADIUS, 0.f, END_COLOR, END_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_nlerp.m_position, m_nlerp.m_rotation.GetRotationAxis() + m_nlerp.m_position, AXIS_RADIUS, 0.f, LERP_COLOR, LERP_COLOR, DebugRenderMode::X_RAY);

	DebugAddWorldArrow(m_slerp.m_position, startAxis + m_slerp.m_position, AXIS_RADIUS, 0.f, START_COLOR, START_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_slerp.m_position, endAxis + m_slerp.m_position, AXIS_RADIUS, 0.f, END_COLOR, END_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_slerp.m_position, m_slerp.m_rotation.GetRotationAxis() + m_slerp.m_position, AXIS_RADIUS, 0.f, LERP_COLOR, LERP_COLOR, DebugRenderMode::X_RAY);

	DebugAddWorldArrow(m_nlerpFullPath.m_position, startAxis + m_nlerpFullPath.m_position, AXIS_RADIUS, 0.f, START_COLOR, START_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_nlerpFullPath.m_position, endAxis + m_nlerpFullPath.m_position, AXIS_RADIUS, 0.f, END_COLOR, END_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_nlerpFullPath.m_position, m_nlerpFullPath.m_rotation.GetRotationAxis() + m_nlerpFullPath.m_position, AXIS_RADIUS, 0.f, LERP_COLOR, LERP_COLOR, DebugRenderMode::X_RAY);

	DebugAddWorldArrow(m_slerpFullPath.m_position, startAxis + m_slerpFullPath.m_position, AXIS_RADIUS, 0.f, START_COLOR, START_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_slerpFullPath.m_position, endAxis + m_slerpFullPath.m_position, AXIS_RADIUS, 0.f, END_COLOR, END_COLOR, DebugRenderMode::X_RAY);
	DebugAddWorldArrow(m_slerpFullPath.m_position, m_slerpFullPath.m_rotation.GetRotationAxis() + m_slerpFullPath.m_position, AXIS_RADIUS, 0.f, LERP_COLOR, LERP_COLOR, DebugRenderMode::X_RAY);
}

void Game3DQuaternion::DrawObjects() const
{
	//g_theRenderer->BindTexture(m_modelTexture);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	Mat44 modelTransform;

	modelTransform = Mat44::MakeFromNonUnitQuat(m_lerp.m_rotation);
	modelTransform.SetTranslation3D(m_lerp.m_position);
	g_theRenderer->SetModelConstants(modelTransform);
	g_theRenderer->DrawVertexArray(m_modelVerts);

	modelTransform = Mat44::MakeFromUnitQuat(m_nlerp.m_rotation);
	modelTransform.SetTranslation3D(m_nlerp.m_position);
	g_theRenderer->SetModelConstants(modelTransform);
	g_theRenderer->DrawVertexArray(m_modelVerts);

	modelTransform = Mat44::MakeFromUnitQuat(m_slerp.m_rotation);
	modelTransform.SetTranslation3D(m_slerp.m_position);
	g_theRenderer->SetModelConstants(modelTransform);
	g_theRenderer->DrawVertexArray(m_modelVerts);

	modelTransform = Mat44::MakeFromUnitQuat(m_nlerpFullPath.m_rotation);
	modelTransform.SetTranslation3D(m_nlerpFullPath.m_position);
	g_theRenderer->SetModelConstants(modelTransform);
	g_theRenderer->DrawVertexArray(m_modelVerts);

	modelTransform = Mat44::MakeFromUnitQuat(m_slerpFullPath.m_rotation);
	modelTransform.SetTranslation3D(m_slerpFullPath.m_position);
	g_theRenderer->SetModelConstants(modelTransform);
	g_theRenderer->DrawVertexArray(m_modelVerts);
}

void Game3DQuaternion::HandleInput()
{

}

