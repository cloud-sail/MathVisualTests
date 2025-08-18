#include "Game/Game3DTestShapes.hpp"
#include "Game/App.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

//-----------------------------------------------------------------------------------------------
static constexpr int	LINE_SEGMENT_NUM = 20;
static constexpr float LINE_SEGMENT_THICKNESS = 5.f;

//const std::string G3D_TEXT = "Game3DTestShapes: WASD(fly horizontal), QE(fly vertical), space(lock/unlock raycast), LMB(grab/release object)";
static const char* G3D_TEXT = "Game3DTestShapes: WASD(fly horizontal), QE(fly vertical), space(%s raycast)";
static const float NEAREST_POINT_SPHERE_RADIUS = 0.05f;

//-----------------------------------------------------------------------------------------------
Game3DTestShapes::Game3DTestShapes()
{
	m_cursorMode = CursorMode::FPS;
	m_camera.SetPerspectiveView(WINDOW_ASPECT, 60.f, 0.1f, 100.0f);
	m_camera.SetCameraToRenderTransform(Mat44::DIRECTX_C2R);

	DebugAddBasis(Mat44(), -1.f, 1.f, 0.05f);

	RandomizeSceneObjects();
}

Game3DTestShapes::~Game3DTestShapes()
{
	for (TestShape* ptr : m_shapeList) {
		delete ptr;
	}
	m_shapeList.clear();
	DebugRenderClear();
}

void Game3DTestShapes::Update()
{
	UpdateDeveloperCheats();


	for (TestShape* shape : m_shapeList)
	{
		shape->m_isSelected = false;
		shape->m_isOverlapping = false;
	}

	UpdatePlayer();
	UpdateRay();
	DoRaycast();
	UpdateCameras();
	UpdateGrabAndLock(); 

	// update grabbed transform

	CheckIfOverlapping();
}

void Game3DTestShapes::Render() const
{
	g_theRenderer->BeginCamera(m_camera);
	DrawObjects();
	DrawRaycastResult();
	g_theRenderer->EndCamera(m_camera);
	DebugRenderWorld(m_camera);

	g_theRenderer->BeginCamera(m_screenCamera);
	DrawUsage();
	g_theRenderer->EndCamera(m_screenCamera);
	DebugRenderScreen(m_screenCamera);
}

void Game3DTestShapes::UpdateCameras()
{
	m_camera.SetPositionAndOrientation(m_playerPosition, m_playerOrientation);
	m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game3DTestShapes::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	const char* raycastText = (m_isRaycastLocked) ? "unlock" : "lock";

	std::string usageText = Stringf(G3D_TEXT, raycastText);
	if (m_grabbedObject != nullptr)
	{
		usageText += ", LMB(release object)";
	}
	else if (m_hitObject)
	{
		usageText += ", LMB(grab object)";
	}

	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game3DTestShapes::UpdatePlayerPosition(float deltaSeconds)
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

void Game3DTestShapes::UpdatePlayerOrientation()
{
	Vec2 cursorPositionDelta = g_theInput->GetCursorClientDelta();
	float deltaYaw = -cursorPositionDelta.x * 0.125f;
	float deltaPitch = cursorPositionDelta.y * 0.125f;

	m_playerOrientation.m_yawDegrees += deltaYaw;
	m_playerOrientation.m_pitchDegrees += deltaPitch;
	m_playerOrientation.m_pitchDegrees = GetClamped(m_playerOrientation.m_pitchDegrees, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);
}

void Game3DTestShapes::UpdatePlayerCameraCenterAxis()
{
	// Add camera center axis
	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_playerOrientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);
	Mat44 cameraCenterAxisTransform;
	cameraCenterAxisTransform.SetTranslation3D(m_playerPosition + 0.2f * forwardIBasis);
	DebugAddBasis(cameraCenterAxisTransform, 0.f, 0.006f, 0.0003f);
}

void Game3DTestShapes::UpdateGrabAndLock()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		m_isRaycastLocked = !m_isRaycastLocked;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		if (m_grabbedObject == nullptr)
		{
			TryToGrabObject();
		}
		else
		{
			ReleaseGrabbedObject();
		}
	}

	if (m_grabbedObject != nullptr)
	{

		if (m_grabbedObject->m_type != TestShape::Type::eType_Plane3)
		{
			m_grabbedObject->m_position = m_camera.GetCameraToWorldTransform().TransformPosition3D(m_grabbedObjectCameraSpacePosition);
			UpdateGrabbedOBB();
		}
		else
		{
			Vec3 newPosOnPlane = m_camera.GetCameraToWorldTransform().TransformPosition3D(m_grabbedObjectCameraSpacePosition);
			m_grabbedObject->m_plane.MoveToPoint(newPosOnPlane);
		}


	}
}

void Game3DTestShapes::TryToGrabObject()
{
	if (m_hitObject != nullptr)
	{
		Mat44 worldToCamera = m_camera.GetWorldToCameraTransform();
		if (m_hitObject->m_type != TestShape::Type::eType_Plane3)
		{
			m_grabbedObjectCameraSpacePosition = worldToCamera.TransformPosition3D(m_hitObject->m_position);
		}
		else
		{
			// for plane transform the hit pos on the plane
			m_grabbedObjectCameraSpacePosition = worldToCamera.TransformPosition3D(m_raycastResult.m_impactPos);
		}


		m_grabbedObject = m_hitObject;
		m_grabbedObject->m_isGrabbed = true;
	}
}

void Game3DTestShapes::ReleaseGrabbedObject()
{
	m_grabbedObject->m_isGrabbed = false;
	m_grabbedObject = nullptr;
}

void Game3DTestShapes::UpdateGrabbedOBB()
{
	if (m_grabbedObject == nullptr || m_grabbedObject->m_type != TestShape::Type::eType_OBB3)
	{
		return;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_O))
	{
		m_grabbedObject->m_obbOrientation.m_yawDegrees -= 10.f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_I))
	{
		m_grabbedObject->m_obbOrientation.m_yawDegrees += 10.f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_J))
	{
		m_grabbedObject->m_obbOrientation.m_pitchDegrees -= 10.f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_K))
	{
		m_grabbedObject->m_obbOrientation.m_pitchDegrees += 10.f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_N))
	{
		m_grabbedObject->m_obbOrientation.m_rollDegrees -= 10.f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_M))
	{
		m_grabbedObject->m_obbOrientation.m_rollDegrees += 10.f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_U))
	{
		m_grabbedObject->m_obbOrientation = EulerAngles();
	}

	const char* OBB_INFO = "Selected OBB has Yaw=%.0f, Pitch=%.0f, Roll=%.0f, Modify Yaw[O,I] or Pitch[J,K] or Roll[N,M] Reset to Identity[U]";
	AABB2 textBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.90f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.93f);
	DebugAddScreenText(Stringf(OBB_INFO, m_grabbedObject->m_obbOrientation.m_yawDegrees, m_grabbedObject->m_obbOrientation.m_pitchDegrees, m_grabbedObject->m_obbOrientation.m_rollDegrees), 
		textBox, 15.f, Vec2(0.f, 0.5f), 0.f, 0.8f);
	// Debug Draw?
}

void Game3DTestShapes::CheckIfOverlapping()
{

	for (int i = 0; i < m_shapeList.size(); ++i) 
	{
		TestShape* shapeA = m_shapeList[i];
		for (int j = i + 1; j < m_shapeList.size(); ++j)
		{
			TestShape* shapeB = m_shapeList[j];
			if (shapeA->IsOverlappingWithOtherShape(*shapeB))
			{
				shapeA->m_isOverlapping = true;
				shapeB->m_isOverlapping = true;
			}
		}
	}
}

void Game3DTestShapes::UpdateRay()
{
	if (!m_isRaycastLocked)
	{
		m_rayStart = m_playerPosition;
		m_rayFwdNormal = m_playerOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D();
	}
}

void Game3DTestShapes::RandomizeSceneObjects()
{
	for (TestShape* ptr : m_shapeList) {
		delete ptr;
	}
	m_shapeList.clear();
	m_shapeList.reserve(20);
	Vec3 sceneDimensions = Vec3(5.f, 5.f, 5.f);
	for (int i = 0; i < 2; ++i)
	{
		m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_Sphere, false));
		m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_Sphere, true));
		m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_AABB3, false));
		m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_AABB3, true));
		m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_ZCylinder, false));
		m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_ZCylinder, true));
		m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_OBB3, false));
		m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_OBB3, true));
	}
	//m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_Sphere, false));
	//m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_AABB3, false));
	//m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_ZCylinder, false));
	//m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_OBB3, false));
	m_shapeList.push_back(new TestShape(sceneDimensions, TestShape::Type::eType_Plane3, false));

	m_grabbedObject = nullptr;
	m_isRaycastLocked = false;
}

void Game3DTestShapes::UpdatePlayer()
{
	float deltaSeconds = static_cast<float>(m_clock->GetDeltaSeconds());
	UpdatePlayerPosition(deltaSeconds);
	UpdatePlayerOrientation();

	UpdatePlayerCameraCenterAxis();

}

void Game3DTestShapes::DoRaycast()
{
	m_hitObject = nullptr;
	m_raycastResult = RaycastResult3D();

	for (TestShape* shape : m_shapeList)
	{
		RaycastResult3D result = shape->GetRaycastResult(m_rayStart, m_rayFwdNormal, m_rayLength);
		if (result.m_didImpact)
		{
			if (!m_raycastResult.m_didImpact)
			{
				m_raycastResult = result;
				m_hitObject = shape;
			}
			else
			{
				if (result.m_impactDist < m_raycastResult.m_impactDist)
				{
					m_raycastResult = result;
					m_hitObject = shape;
				}
			}
		}
	}

	// Color the hit object


	if (m_hitObject != nullptr)
	{
		m_hitObject->m_isSelected = true;
	}
}

void Game3DTestShapes::DrawObjects() const
{
	// Draw Shapes
	for (TestShape* shape : m_shapeList)
	{
		shape->Render();
	}

	// Draw Nearest Point
	std::vector<Vec3> nearestPoints;
	nearestPoints.reserve(m_shapeList.size());
	float minDistanceSquared = 10000000000.f;
	TestShape* nearestShape = nullptr;

	for (TestShape* shape : m_shapeList)
	{
		Vec3 nearestPoint = shape->GetNearestPoint(m_rayStart);
		float distanceSquared = GetDistanceSquared3D(m_rayStart, nearestPoint);
		if (distanceSquared < minDistanceSquared)
		{
			minDistanceSquared = distanceSquared;
			nearestShape = shape;
		}
	}

	if (nearestShape != nullptr)
	{
		Vec3 nearestPoint = nearestShape->GetNearestPoint(m_rayStart);
		DebugAddWorldSphere(nearestPoint, NEAREST_POINT_SPHERE_RADIUS, 0.f, Rgba8::GREEN);
	}

	for (TestShape* shape : m_shapeList)
	{
		if (nearestShape != shape)
		{
			Vec3 nearestPoint = shape->GetNearestPoint(m_rayStart);
			DebugAddWorldSphere(nearestPoint, NEAREST_POINT_SPHERE_RADIUS, 0.f, ORANGE);
		}
	}
}



void Game3DTestShapes::DrawRaycastResult() const
{

	if (m_raycastResult.m_didImpact)
	{
		DebugAddWorldSphere(m_raycastResult.m_impactPos, NEAREST_POINT_SPHERE_RADIUS, 0.f, Rgba8::OPAQUE_WHITE);
		DebugAddWorldArrow(m_raycastResult.m_impactPos, m_raycastResult.m_impactPos + 0.5f * m_raycastResult.m_impactNormal, 0.01f, 0.f, Rgba8::YELLOW);
		if (m_isRaycastLocked)
		{
			DebugAddWorldArrow(m_rayStart, m_rayStart + m_rayFwdNormal * m_rayLength, 0.009f, 0.f, DARK_GREY);
			DebugAddWorldArrow(m_rayStart, m_rayStart + m_rayFwdNormal * m_raycastResult.m_impactDist, 0.01f, 0.f, Rgba8::RED);
		}
	}
	else
	{
		if (m_isRaycastLocked)
		{
			DebugAddWorldArrow(m_rayStart, m_rayStart + m_rayFwdNormal * m_rayLength, 0.01f, 0.f, Rgba8::GREEN);
		}

	}
}

TestShape::TestShape(Vec3 const& sceneDimensions, Type shapeType, bool isWireFrame)
	: m_type(shapeType)
	, m_isWireFrame(isWireFrame)
{
	m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");

	m_position.x = g_rng.RollRandomFloatInRange(-sceneDimensions.x, sceneDimensions.x);
	m_position.y = g_rng.RollRandomFloatInRange(-sceneDimensions.y, sceneDimensions.y);
	m_position.z = g_rng.RollRandomFloatInRange(-sceneDimensions.z, sceneDimensions.z);


	if (m_type == eType_Sphere)
	{
		float radius = g_rng.RollRandomFloatInRange(sceneDimensions.x * 0.1f, sceneDimensions.x * 0.2f);
		SetSphereType(radius);
	}
	else if (m_type == eType_AABB3)
	{
		Vec3 halfDimensions;
		halfDimensions.x = g_rng.RollRandomFloatInRange(sceneDimensions.x * 0.05f, sceneDimensions.x * 0.1f);
		halfDimensions.y = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.05f, sceneDimensions.y * 0.1f);
		halfDimensions.z = g_rng.RollRandomFloatInRange(sceneDimensions.z * 0.05f, sceneDimensions.z * 0.1f);
		SetAABB3Type(halfDimensions);
	}
	else if (m_type == eType_ZCylinder)
	{
		float radius = g_rng.RollRandomFloatInRange(sceneDimensions.x * 0.1f, sceneDimensions.x * 0.2f);
		float halfHeight = g_rng.RollRandomFloatInRange(sceneDimensions.z * 0.1f, sceneDimensions.z * 0.2f);

		SetZCylinderType(radius, halfHeight);
	}
	else if (m_type == eType_OBB3)
	{
		Vec3 halfDimensions;
		halfDimensions.x = g_rng.RollRandomFloatInRange(sceneDimensions.x * 0.05f, sceneDimensions.x * 0.1f);
		halfDimensions.y = g_rng.RollRandomFloatInRange(sceneDimensions.y * 0.05f, sceneDimensions.y * 0.1f);
		halfDimensions.z = g_rng.RollRandomFloatInRange(sceneDimensions.z * 0.05f, sceneDimensions.z * 0.1f);

		EulerAngles orientation;
		orientation.m_yawDegrees = g_rng.RollRandomFloatInRange(-180.f, 180.f);
		orientation.m_pitchDegrees = g_rng.RollRandomFloatInRange(-90.f, 90.f);
		orientation.m_yawDegrees = g_rng.RollRandomFloatInRange(-180.f, 180.f);

		SetOBB3Type(halfDimensions, orientation);
	}
	else if (m_type == eType_Plane3)
	{
		float yawDegrees = g_rng.RollRandomFloatInRange(-180.f, 180.f);
		float pitchDegrees = g_rng.RollRandomFloatInRange(-90.f, 90.f);
		Vec3 normal = Vec3::MakeFromPolarDegrees(pitchDegrees, yawDegrees);

		float d = g_rng.RollRandomFloatInRange(-sceneDimensions.x, sceneDimensions.x);
		SetPlane3Type(normal, d);
	}
}

void TestShape::SetSphereType(float radius)
{
	m_type = eType_Sphere;
	m_sphereRadius = radius;

	m_vertexes.clear();
	m_vertexes.reserve(768);
	AddVertsForSphere3D(m_vertexes, Vec3(), radius, Rgba8::OPAQUE_WHITE, AABB2::ZERO_TO_ONE, 16, 8);
}

void TestShape::SetAABB3Type(Vec3 halfDimensions)
{
	m_type = eType_AABB3;
	m_boxHalfDimensions = halfDimensions;

	m_vertexes.clear();
	m_vertexes.reserve(36);
	AddVertsForAABB3D(m_vertexes, AABB3(-halfDimensions, halfDimensions));
}

void TestShape::SetZCylinderType(float radius, float halfHeight)
{
	m_type = eType_ZCylinder;
	m_cylinderRadius = radius;
	m_cylinderHalfHeight = halfHeight;

	m_vertexes.clear();
	m_vertexes.reserve(384);
	AddVertsForCylinderZ3D(m_vertexes, Vec2::ZERO, FloatRange(-halfHeight, halfHeight), radius, 8);
}

void TestShape::SetOBB3Type(Vec3 halfDimensions, EulerAngles orientation)
{
	m_type = eType_OBB3;
	m_boxHalfDimensions = halfDimensions;
	m_obbOrientation = orientation;

	m_vertexes.clear();
	m_vertexes.reserve(36);
	AddVertsForAABB3D(m_vertexes, AABB3(-halfDimensions, halfDimensions));
}

void TestShape::SetPlane3Type(Vec3 normal, float d)
{
	m_type = eType_Plane3;
	m_plane = Plane3(normal, d);
	// AddVerts When Rendering
}

Mat44 TestShape::GetModelToWorldTransform() const
{
	Mat44 result;
	if (m_type == eType_OBB3)
	{
		result = m_obbOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	}
	result.SetTranslation3D(m_position);
	return result;
}

void TestShape::Render() const
{

	if (m_type == eType_Plane3)
	{
		std::vector<Vertex_PCU> verts;

		AddVertsForGridPlane3D(verts, m_plane);
		Vec3 gridCenter = m_plane.GetNearestPoint(Vec3::ZERO);
		AddVertsForCylinder3D(verts, Vec3::ZERO, gridCenter, 0.02f, Rgba8(200, 200, 200, 128), AABB2::ZERO_TO_ONE, 4);
		AddVertsForSphere3D(verts, gridCenter, 0.06f, DARK_GREY, AABB2::ZERO_TO_ONE, 8, 4);
		AddVertsForArrow3D(verts, gridCenter, gridCenter + m_plane.m_normal, 0.03f, Rgba8(0, 0, 255, 150), 4);

		g_theRenderer->SetModelConstants();
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->DrawVertexArray(verts);

		return;
	}

	// Other Shapes
	Rgba8 tint = m_isWireFrame ? DARK_BLUE : Rgba8::OPAQUE_WHITE;
	if (m_isSelected)
	{
		tint = LIGHT_BLUE;
	}
	if (m_isGrabbed)
	{
		tint = Rgba8::RED;
	}
	if (m_isOverlapping)
	{
		float currentTime = static_cast<float>(Clock::GetSystemClock().GetTotalSeconds());
		float colorScale = 1.f - 0.5f * fabsf(SinRadians(currentTime * 2.f));
		tint = Interpolate(Rgba8(0, 0, 0), tint, colorScale);
	}




	g_theRenderer->SetModelConstants(GetModelToWorldTransform(), tint);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);

	if (m_isWireFrame)
	{
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
	}
	else
	{
		g_theRenderer->BindTexture(m_texture);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	}

	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->DrawVertexArray(m_vertexes);
}

Vec3 TestShape::GetNearestPoint(Vec3 const& point)
{
	if (m_type == eType_Sphere)
	{
		Vec3 sphereCenter = m_position;
		return GetNearestPointOnSphere3D(point, sphereCenter, m_sphereRadius);
	}
	else if (m_type == eType_AABB3)
	{
		AABB3 box(-m_boxHalfDimensions + m_position, m_boxHalfDimensions + m_position);
		return GetNearestPointOnAABB3D(point, box);
	}
	else if (m_type == eType_ZCylinder)
	{
		Vec2 cylinderCenterXY = Vec2(m_position.x, m_position.y);
		float cylinderRadius = m_cylinderRadius;
		FloatRange cylinderMinMaxZ = FloatRange(m_position.z - m_cylinderHalfHeight, m_position.z + m_cylinderHalfHeight);
		return GetNearestPointOnCylinderZ3D(point, cylinderCenterXY, cylinderRadius, cylinderMinMaxZ);
	}
	else if (m_type == eType_OBB3)
	{
		Vec3 iBasisNormal, jBasisNormal, kBasisNormal;
		m_obbOrientation.GetAsVectors_IFwd_JLeft_KUp(iBasisNormal, jBasisNormal, kBasisNormal);
		OBB3 orientedBox(m_position, iBasisNormal, jBasisNormal, kBasisNormal, m_boxHalfDimensions);
		return GetNearestPointOnOBB3D(point, orientedBox);
	}
	else if (m_type == eType_Plane3)
	{
		return m_plane.GetNearestPoint(point);
	}
	ERROR_AND_DIE("Test Shape Type not assigned!");
}

bool TestShape::IsOverlappingWithOtherShape(TestShape const& other)
{
	if (m_type == eType_Sphere)
	{
		Vec3 sphereCenterA = m_position;
		float sphereRadiusA = m_sphereRadius;

		if (other.m_type == eType_Sphere)
		{
			Vec3 sphereCenterB = other.m_position;
			float sphereRadiusB = other.m_sphereRadius;
			return DoSpheresOverlap3D(sphereCenterA, sphereRadiusA, sphereCenterB, sphereRadiusB);
		}
		else if (other.m_type == eType_AABB3)
		{
			AABB3 boxB(-other.m_boxHalfDimensions + other.m_position, other.m_boxHalfDimensions + other.m_position);
			return DoSphereAndAABBOverlap3D(sphereCenterA, sphereRadiusA, boxB);
		}
		else if (other.m_type == eType_ZCylinder)
		{
			Vec2 cylinderCenterXYB = Vec2(other.m_position.x, other.m_position.y);
			float cylinderRadiusB = other.m_cylinderRadius;
			FloatRange cylinderMinMaxZB = FloatRange(other.m_position.z - other.m_cylinderHalfHeight, other.m_position.z + other.m_cylinderHalfHeight);
			return DoZCylinderAndSphereOverlap3D(cylinderCenterXYB, cylinderRadiusB, cylinderMinMaxZB, sphereCenterA, sphereRadiusA);
		}
		else if (other.m_type == eType_OBB3)
		{
			Vec3 iBasisNormal, jBasisNormal, kBasisNormal;
			other.m_obbOrientation.GetAsVectors_IFwd_JLeft_KUp(iBasisNormal, jBasisNormal, kBasisNormal);
			OBB3 orientedBoxB(other.m_position, iBasisNormal, jBasisNormal, kBasisNormal, other.m_boxHalfDimensions);
			return DoSphereAndOBBOverlap3D(sphereCenterA, sphereRadiusA, orientedBoxB);
		}
		else if (other.m_type == eType_Plane3)
		{
			return DoSphereAndPlaneOverlap3D(m_position, sphereRadiusA, other.m_plane);
		}
	}
	else if (m_type == eType_AABB3)
	{
		AABB3 boxA(-m_boxHalfDimensions + m_position, m_boxHalfDimensions + m_position);

		if (other.m_type == eType_Sphere)
		{
			Vec3 sphereCenterB = other.m_position;
			float sphereRadiusB = other.m_sphereRadius;
			return DoSphereAndAABBOverlap3D(sphereCenterB, sphereRadiusB, boxA);
		}
		else if (other.m_type == eType_AABB3)
		{
			AABB3 boxB(-other.m_boxHalfDimensions + other.m_position, other.m_boxHalfDimensions + other.m_position);
			return DoAABBsOverlap3D(boxA, boxB);
		}
		else if (other.m_type == eType_ZCylinder)
		{
			Vec2 cylinderCenterXYB = Vec2(other.m_position.x, other.m_position.y);
			float cylinderRadiusB = other.m_cylinderRadius;
			FloatRange cylinderMinMaxZB = FloatRange(other.m_position.z - other.m_cylinderHalfHeight, other.m_position.z + other.m_cylinderHalfHeight);
			return DoZCylinderAndAABBOverlap3D(cylinderCenterXYB, cylinderRadiusB, cylinderMinMaxZB, boxA);
		}
		else if (other.m_type == eType_Plane3)
		{
			return DoAABBAndPlaneOverlap3D(boxA, other.m_plane);
		}
	}
	else if (m_type == eType_ZCylinder)
	{
		Vec2 cylinderCenterXYA = Vec2(m_position.x, m_position.y);
		float cylinderRadiusA = m_cylinderRadius;
		FloatRange cylinderMinMaxZA = FloatRange(m_position.z - m_cylinderHalfHeight, m_position.z + m_cylinderHalfHeight);

		if (other.m_type == eType_Sphere)
		{
			Vec3 sphereCenterB = other.m_position;
			float sphereRadiusB = other.m_sphereRadius;
			return DoZCylinderAndSphereOverlap3D(cylinderCenterXYA, cylinderRadiusA, cylinderMinMaxZA, sphereCenterB, sphereRadiusB);
		}
		else if (other.m_type == eType_AABB3)
		{
			AABB3 boxB(-other.m_boxHalfDimensions + other.m_position, other.m_boxHalfDimensions + other.m_position);
			return DoZCylinderAndAABBOverlap3D(cylinderCenterXYA, cylinderRadiusA, cylinderMinMaxZA, boxB);
		}
		else if (other.m_type == eType_ZCylinder)
		{
			Vec2 cylinderCenterXYB = Vec2(other.m_position.x, other.m_position.y);
			float cylinderRadiusB = other.m_cylinderRadius;
			FloatRange cylinderMinMaxZB = FloatRange(other.m_position.z - other.m_cylinderHalfHeight, other.m_position.z + other.m_cylinderHalfHeight);
			return DoZCylindersOverlap3D(cylinderCenterXYA, cylinderRadiusA, cylinderMinMaxZA, cylinderCenterXYB, cylinderRadiusB, cylinderMinMaxZB);
		}

	}
	else if (m_type == eType_OBB3)
	{
		Vec3 iBasisNormal, jBasisNormal, kBasisNormal;
		m_obbOrientation.GetAsVectors_IFwd_JLeft_KUp(iBasisNormal, jBasisNormal, kBasisNormal);
		OBB3 orientedBoxA(m_position, iBasisNormal, jBasisNormal, kBasisNormal, m_boxHalfDimensions);
		if (other.m_type == eType_Sphere)
		{
			Vec3 sphereCenterB = other.m_position;
			float sphereRadiusB = other.m_sphereRadius;
			return DoSphereAndOBBOverlap3D(sphereCenterB, sphereRadiusB, orientedBoxA);
		}
		else if (other.m_type == eType_Plane3)
		{
			return DoOBBAndPlaneOverlap3D(orientedBoxA, other.m_plane);
		}
	}
	else if (m_type == eType_Plane3)
	{
		if (other.m_type == eType_Sphere)
		{
			Vec3 sphereCenterB = other.m_position;
			float sphereRadiusB = other.m_sphereRadius;
			return DoSphereAndPlaneOverlap3D(sphereCenterB, sphereRadiusB, m_plane);
		}
		else if (other.m_type == eType_AABB3)
		{
			AABB3 boxB(-other.m_boxHalfDimensions + other.m_position, other.m_boxHalfDimensions + other.m_position);
			return DoAABBAndPlaneOverlap3D(boxB, m_plane);
		}
		else if (other.m_type == eType_OBB3)
		{
			Vec3 iBasisNormal, jBasisNormal, kBasisNormal;
			other.m_obbOrientation.GetAsVectors_IFwd_JLeft_KUp(iBasisNormal, jBasisNormal, kBasisNormal);
			OBB3 orientedBoxB(other.m_position, iBasisNormal, jBasisNormal, kBasisNormal, other.m_boxHalfDimensions);
			return DoOBBAndPlaneOverlap3D(orientedBoxB, m_plane);
		}
	}

	return false;
	//ERROR_AND_DIE("Test Shape Type not assigned!");
}

RaycastResult3D TestShape::GetRaycastResult(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength)
{
	if (m_type == eType_Sphere)
	{
		Vec3 sphereCenter = m_position;
		float sphereRadius = m_sphereRadius;

		return RaycastVsSphere3D(rayStart, rayForwardNormal, rayLength, sphereCenter, sphereRadius);
	}
	else if (m_type == eType_AABB3)
	{
		AABB3 box(-m_boxHalfDimensions + m_position, m_boxHalfDimensions + m_position);
		return RaycastVsAABB3D(rayStart, rayForwardNormal, rayLength, box);
	}
	else if (m_type == eType_ZCylinder)
	{
		Vec2 cylinderCenterXY = Vec2(m_position.x, m_position.y);
		float cylinderRadius = m_cylinderRadius;
		FloatRange cylinderMinMaxZ = FloatRange(m_position.z - m_cylinderHalfHeight, m_position.z + m_cylinderHalfHeight);
		return RaycastVsCylinderZ3D(rayStart, rayForwardNormal, rayLength, cylinderCenterXY, cylinderMinMaxZ, cylinderRadius);
	}
	else if (m_type == eType_OBB3)
	{
		Vec3 iBasisNormal, jBasisNormal, kBasisNormal;
		m_obbOrientation.GetAsVectors_IFwd_JLeft_KUp(iBasisNormal, jBasisNormal, kBasisNormal);
		OBB3 orientedBox(m_position, iBasisNormal, jBasisNormal, kBasisNormal, m_boxHalfDimensions);
		return RaycastVsOBB3D(rayStart, rayForwardNormal, rayLength, orientedBox);
	}
	else if (m_type == eType_Plane3)
	{
		return RaycastVsPlane3D(rayStart, rayForwardNormal, rayLength, m_plane);
	}

	return RaycastResult3D();
	//ERROR_RECOVERABLE("Test Shape Type not assigned!");
}

