#include "Game/Game2DPachinkoMachine.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"



namespace PachinkoMachine
{
	const char* README_TEXT = "Pachinko Machine (2D): LMB/RMB/ESDF/IJKL move, space/N= ball (%d), e=%.2f (G,H), B=Bottom warp %s, ";
	const char* README_TEXT_FIXED = "timestep=%.2fms (P,[,]), dt=%.1fms";
	const char* README_TEXT_VARIABLE = "variable timestep (P), dt=%.1fms";


	constexpr float MOVESPEED = 175.f;
	constexpr float ARROWSIZE = 15.f;
	constexpr float LINETHICKNESS = 2.f;

	constexpr float MIN_BALLRADIUS = 5.f;
	constexpr float MAX_BALLRADIUS = 25.f;

	constexpr int	NUM_DISCBUMPER = 10;
	constexpr float MIN_DISCBUMPER_RADIUS = 5.f;
	constexpr float MAX_DISCBUMPER_RADIUS = 50.f;

	constexpr int	NUM_CAPSULEBUMPER = 10;
	constexpr float MIN_CAPSULEBUMPER_HALFHEIGHT = 1.f;
	constexpr float MAX_CAPSULEBUMPER_HALFHEIGHT = 75.f;
	constexpr float MIN_CAPSULEBUMPER_RADIUS = 5.f;
	constexpr float MAX_CAPSULEBUMPER_RADIUS = 50.f;

	constexpr int	NUM_OBBBUMPER = 10;
	constexpr float MIN_OBBBUMPER_WIDTH = 5.f;
	constexpr float MAX_OBBBUMPER_WIDTH = 80.f;


	constexpr float MIN_BUMPER_ELASTICITY = 0.01f;
	constexpr float MAX_BUMPER_ELASTICITY = 0.99f;

	constexpr float WALL_ELASTICITY = 0.90f;

	constexpr unsigned char BUMPER_ALPHA = 128;

	constexpr float DEFAULT_GRAVITY_ACCELERATION = 700.f;

	constexpr float ZOOM_SPEED = 1.f;
	constexpr float ROLL_TURNRATE = 45.f;


	//-----------------------------------------------------------------------------------------------
	void BounceDiscOffFixedPoint(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2& mobileDiscVelocity, Vec2 const& fixedPoint, float combinedElasticity)
	{
		Vec2 originalMobileDiscCenter = mobileDiscCenter;

		if (!PushDiscOutOfFixedPoint2D(mobileDiscCenter, mobileDiscRadius, fixedPoint))
		{
			return; // not pushed
		}

		Vec2 dispAToB = fixedPoint - originalMobileDiscCenter;
		if (DotProduct2D(-mobileDiscVelocity, dispAToB) < 0.f) // Converging = relative Velocity (B to A) is opposite to displacement(A to B)
		{
			// Converging, exchange velocity
			Vec2 aNormalVelocity = GetProjectedOnto2D(mobileDiscVelocity, dispAToB);

			//mobileDiscVelocity = mobileDiscVelocity - aNormalVelocity - aNormalVelocity * combinedElasticity;
			mobileDiscVelocity = mobileDiscVelocity - (1.f + combinedElasticity) * aNormalVelocity;
		}
	}

	void BounceDiscOffEachOther(Vec2& aCenter, Vec2& bCenter, float aRadius, float bRadius, Vec2& aVelocity, Vec2& bVelocity, float combinedElasticity)
	{
		Vec2 originalACenter = aCenter;
		Vec2 originalBCenter = bCenter;

		if (!PushDiscsOutOfEachOther2D(aCenter, aRadius, bCenter, bRadius))
		{
			return; // not pushed
		}

		Vec2 dispAToB = originalBCenter - originalACenter;
		// Converging = relative Velocity (B to A) is opposite to displacement(A to B)
		if (DotProduct2D(bVelocity - aVelocity, dispAToB) < 0.f)
		{
			// Converging, exchange velocity
			Vec2 aNormalVelocity = GetProjectedOnto2D(aVelocity, dispAToB);
			Vec2 bNormalVelocity = GetProjectedOnto2D(bVelocity, dispAToB);

			aVelocity = aVelocity - aNormalVelocity + combinedElasticity * bNormalVelocity;
			bVelocity = bVelocity - bNormalVelocity + combinedElasticity * aNormalVelocity;
		}
	}

	//-----------------------------------------------------------------------------------------------
	void Bumper::BounceBallOffOf(Ball& ball, float ballElasticity)
	{
		float combinedElasticity = m_elasticity * ballElasticity;
		if (m_type == BumperType::DISC)
		{
			BounceDiscOffFixedPoint(ball.m_center, ball.m_radius + m_radius, ball.m_velocity, m_center, combinedElasticity);
		}
		else if (m_type == BumperType::CAPSULE)
		{
			Vec2 fixedPoint = GetNearestPointOnLineSegment2D(ball.m_center, m_center - m_capsuleHalfOffset, m_center + m_capsuleHalfOffset);
			BounceDiscOffFixedPoint(ball.m_center, ball.m_radius + m_radius, ball.m_velocity, fixedPoint, combinedElasticity);
		}
		else if (m_type == BumperType::OBB)
		{
			OBB2 obbShape = OBB2(m_center, m_iBasisNormal, m_halfDimensions);
			Vec2 fixedPoint = GetNearestPointOnOBB2D(ball.m_center, obbShape);
			BounceDiscOffFixedPoint(ball.m_center, ball.m_radius + m_radius, ball.m_velocity, fixedPoint, combinedElasticity);
		}
	}

}



//-----------------------------------------------------------------------------------------------
Game2DPachinkoMachine::Game2DPachinkoMachine()
{
	m_balls.reserve(1500);
	m_camera.SetCameraToRenderTransform(Mat44::DIRECTX_C2R);
	RandomizeSceneObjects();
}

Game2DPachinkoMachine::~Game2DPachinkoMachine()
{

}

void Game2DPachinkoMachine::Update()
{
	HandleInput();

	float deltaSeconds = (float)m_clock->GetDeltaSeconds();
	m_owedPhysicsSeconds += deltaSeconds;

	if (m_isFixedTimeStep)
	{
		while (m_owedPhysicsSeconds >= m_fixedTimeStep)
		{
			UpdatePhysics(m_fixedTimeStep);
			m_owedPhysicsSeconds -= m_fixedTimeStep;
		}
	}
	else
	{
		UpdatePhysics(m_owedPhysicsSeconds);
		m_owedPhysicsSeconds = 0.f;
	}


	UpdateCameras();
}

void Game2DPachinkoMachine::Render() const
{
	g_theRenderer->BeginCamera(m_camera);

	DrawPachinkoMachine();
	DrawObjects();
	DrawUsage();

	g_theRenderer->EndCamera(m_camera);
}

void Game2DPachinkoMachine::RandomizeSceneObjects()
{
	using namespace PachinkoMachine;

	m_gravityAcceleration = DEFAULT_GRAVITY_ACCELERATION;
	m_gravityDirection = Vec2(0.f, -1.f);

	m_balls.clear();
	m_bumpers.clear();
	m_bumperVerts.clear();

	AABB2 sceneBox = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	sceneBox.AddPadding(sceneBox.GetDimensions().x * -0.1f, sceneBox.GetDimensions().y * -0.1f);


	for (int i = 0; i < NUM_DISCBUMPER; ++i)
	{
		Bumper disc;
		disc.m_radius			= g_rng.RollRandomFloatInRange(MIN_DISCBUMPER_RADIUS, MAX_DISCBUMPER_RADIUS);
		disc.m_center			= sceneBox.GetPointAtUV(GetRandomUVs());
		disc.m_boundingRadius	= disc.m_radius;
		disc.m_elasticity		= g_rng.RollRandomFloatInRange(MIN_BUMPER_ELASTICITY, MAX_BUMPER_ELASTICITY);
		disc.m_type				= BumperType::DISC;


		m_bumpers.push_back(disc);
		Rgba8 bumperColor = Rgba8::MakeFromZeroToOne(disc.m_elasticity);
		//Rgba8 bumperColor = Interpolate(Rgba8::RED, Rgba8::GREEN, disc.m_elasticity);
		bumperColor.a = BUMPER_ALPHA;

		AddVertsForDisc2D(m_bumperVerts, disc.m_center, disc.m_radius, bumperColor);
		//AddVertsForGradientDisc2D(m_bumerVerts, disc.m_center, disc.m_radius, Rgba8::OPAQUE_WHITE, bumperColor);
	}

	for (int i = 0; i < NUM_CAPSULEBUMPER; ++i)
	{
		Bumper capsule;
		capsule.m_radius = g_rng.RollRandomFloatInRange(MIN_CAPSULEBUMPER_RADIUS, MAX_CAPSULEBUMPER_RADIUS);
		capsule.m_center = sceneBox.GetPointAtUV(GetRandomUVs());
		float halfHeight = g_rng.RollRandomFloatInRange(MIN_CAPSULEBUMPER_HALFHEIGHT, MAX_CAPSULEBUMPER_HALFHEIGHT);
		capsule.m_capsuleHalfOffset = Vec2::MakeFromPolarDegrees(g_rng.RollRandomFloatInRange(0.f, 360.f), halfHeight);
		capsule.m_elasticity = g_rng.RollRandomFloatInRange(MIN_BUMPER_ELASTICITY, MAX_BUMPER_ELASTICITY);
		capsule.m_boundingRadius = capsule.m_radius + halfHeight;
		capsule.m_type = BumperType::CAPSULE;

		m_bumpers.push_back(capsule);
		Rgba8 bumperColor = Rgba8::MakeFromZeroToOne(capsule.m_elasticity);
		bumperColor.a = BUMPER_ALPHA;

		AddVertsForCapsule2D(m_bumperVerts, capsule.m_center - capsule.m_capsuleHalfOffset, capsule.m_center + capsule.m_capsuleHalfOffset, capsule.m_radius, bumperColor);
	}

	for (int i = 0; i < NUM_OBBBUMPER; ++i)
	{
		Bumper obb;
		obb.m_center = sceneBox.GetPointAtUV(GetRandomUVs());
		obb.m_halfDimensions.x = g_rng.RollRandomFloatInRange(MIN_OBBBUMPER_WIDTH, MAX_OBBBUMPER_WIDTH);
		obb.m_halfDimensions.y = g_rng.RollRandomFloatInRange(MIN_OBBBUMPER_WIDTH, MAX_OBBBUMPER_WIDTH);
		obb.m_iBasisNormal = Vec2::MakeFromPolarDegrees(g_rng.RollRandomFloatInRange(0.f, 360.f));
		obb.m_elasticity = g_rng.RollRandomFloatInRange(MIN_BUMPER_ELASTICITY, MAX_BUMPER_ELASTICITY);
		obb.m_boundingRadius = obb.m_halfDimensions.GetLength();
		obb.m_type = BumperType::OBB;

		m_bumpers.push_back(obb);
		Rgba8 bumperColor = Rgba8::MakeFromZeroToOne(obb.m_elasticity);
		bumperColor.a = BUMPER_ALPHA;

		OBB2 obbShape = OBB2(obb.m_center, obb.m_iBasisNormal, obb.m_halfDimensions);
		AddVertsForOBB2D(m_bumperVerts, obbShape, bumperColor);
	}
}

void Game2DPachinkoMachine::UpdateCameras()
{
	//m_camera.SetOrthographicView();
	m_camera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	Vec2 halfDimension = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);
	Vec2 cameraCenter = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);
	m_camera.SetOrthographicView(-halfDimension* m_cameraSizeScale, halfDimension * m_cameraSizeScale);

	m_camera.SetPositionAndOrientation(Vec3(cameraCenter, 0.5f), EulerAngles(90.f, 90.f, -m_gravityDirectionDegrees - 90.f));

}

void Game2DPachinkoMachine::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	verts.reserve(500);
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.91f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	std::string usageText = Stringf(PachinkoMachine::README_TEXT, (int)m_balls.size(), m_ballElasticity, (m_isBottomWallPresent? "on" : "off"));
	std::string usageText2;

	if (m_isFixedTimeStep)
	{
		usageText2 = Stringf(PachinkoMachine::README_TEXT_FIXED, m_fixedTimeStep * 1000.f, m_clock->GetDeltaSeconds() * 1000.f);
	}
	else
	{
		usageText2 = Stringf(PachinkoMachine::README_TEXT_VARIABLE, m_clock->GetDeltaSeconds() * 1000.f);
	}

	std::string usageText3 = "\nR/U rotate, W/O zoom";

	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText + usageText2 + usageText3, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game2DPachinkoMachine::HandleInput()
{
	HandleInputMoveLauncher();
	m_clock->SetTimeScale(g_theInput->IsKeyDown(KEYCODE_T) ? 0.05 : 1.0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		SpawnBall();
	}
	if (g_theInput->IsKeyDown(KEYCODE_N))
	{
		SpawnBall();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_B))
	{
		m_isBottomWallPresent = !m_isBottomWallPresent;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_G))
	{
		m_ballElasticity -= 0.05f;
		m_ballElasticity = GetClampedZeroToOne(m_ballElasticity);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_H))
	{
		m_ballElasticity += 0.05f;
		m_ballElasticity = GetClampedZeroToOne(m_ballElasticity);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTBRACKET))
	{
		m_fixedTimeStep /= 1.1f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTBRACKET))
	{
		m_fixedTimeStep *= 1.1f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_P))
	{
		m_isFixedTimeStep = !m_isFixedTimeStep;
	}
	//-----------------------------------------------------------------------------------------------

	HandleInputCameraAndGravityDirection();
}

void Game2DPachinkoMachine::HandleInputMoveLauncher()
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
	m_startPos += startPosIntention * PachinkoMachine::MOVESPEED * deltaSeconds;
	m_endPos += endPosIntention * PachinkoMachine::MOVESPEED * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		m_startPos = MapMouseCursorToWorldPoint();
		//m_startPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight())); // TODO camera space to world space
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
	{
		m_endPos = MapMouseCursorToWorldPoint();
		//m_endPos = MapMouseCursorToWorldCoords2D(AABB2(m_camera.GetOrthoBottomLeft(), m_camera.GetOrthoTopRight()));
	}
}

void Game2DPachinkoMachine::HandleInputCameraAndGravityDirection()
{
	float deltaSeconds = (float)m_clock->GetDeltaSeconds();

	float posRoll = (g_theInput->IsKeyDown(KEYCODE_R)) ? 1.f : 0.f;
	float negRoll = (g_theInput->IsKeyDown(KEYCODE_U)) ? -1.f : 0.f;
	float deltaRoll = (posRoll + negRoll) * deltaSeconds * PachinkoMachine::ROLL_TURNRATE;

	m_gravityDirectionDegrees += deltaRoll;
	m_gravityDirection = Vec2::MakeFromPolarDegrees(m_gravityDirectionDegrees);

	//-----------------------------------------------------------------------------------------------
	float posScale = (g_theInput->IsKeyDown(KEYCODE_O)) ? 1.f : 0.f;
	float negScale = (g_theInput->IsKeyDown(KEYCODE_W)) ? -1.f : 0.f;
	float deltaScale = (posScale + negScale) * deltaSeconds * PachinkoMachine::ZOOM_SPEED;

	m_cameraSizeScale += deltaScale;
	if (m_cameraSizeScale < 1.f)
	{
		m_cameraSizeScale = 1.f;
	}
}

void Game2DPachinkoMachine::UpdatePhysics(float deltaSeconds)
{
	ApplyGravityAndMoveBalls(deltaSeconds);
	BounceBalls();
	BounceBallsWithBumpers();
	BounceBallsWithWalls();
}

void Game2DPachinkoMachine::ApplyGravityAndMoveBalls(float deltaSeconds)
{
	Vec2 acceleration = m_gravityDirection * m_gravityAcceleration;
	int numBalls = (int)m_balls.size();
	for (int i = 0; i < numBalls; ++i)
	{
		PachinkoMachine::Ball& ball = m_balls[i];
		ball.m_velocity += acceleration * deltaSeconds;
		ball.m_center += ball.m_velocity * deltaSeconds;
	}
}

void Game2DPachinkoMachine::BounceBalls()
{
	int numBalls = (int)m_balls.size();

	for (int i = 0; i < numBalls; ++i)
	{
		PachinkoMachine::Ball& ballA = m_balls[i];
		for (int j = i+1; j < numBalls; ++j)
		{
			PachinkoMachine::Ball& ballB = m_balls[j];



			PachinkoMachine::BounceDiscOffEachOther(ballA.m_center, ballB.m_center, ballA.m_radius, ballB.m_radius, ballA.m_velocity, ballB.m_velocity, m_ballElasticity * m_ballElasticity);
		}
	}
}

void Game2DPachinkoMachine::BounceBallsWithBumpers()
{
	int numBumpers = (int)m_bumpers.size();
	int numBalls = (int)m_balls.size();

	for (int i = 0; i < numBumpers; ++i)
	{
		PachinkoMachine::Bumper& bumper = m_bumpers[i];
		for (int j = 0; j < numBalls; ++j)
		{
			PachinkoMachine::Ball& ball = m_balls[j];

			// First Check bounding disc is overlapping
			if (!DoDiscsOverlap(bumper.m_center, bumper.m_boundingRadius, ball.m_center, ball.m_radius))
			{
				continue;
			}

			bumper.BounceBallOffOf(ball, m_ballElasticity);
		}
	}
}

void Game2DPachinkoMachine::BounceBallsWithWalls()
{
	float combinedElasticity = m_ballElasticity * PachinkoMachine::WALL_ELASTICITY;

	int numBalls = (int)m_balls.size();
	for (int i = 0; i < numBalls; ++i)
	{
		PachinkoMachine::Ball& ball = m_balls[i];
		// Using Bounce of with Large OBB is also ok
		// Left wall
		if ((ball.m_center.x - ball.m_radius) < m_leftWallX)
		{
			// Push
			ball.m_center.x = m_leftWallX + ball.m_radius;

			if (ball.m_velocity.x < 0.f)
			{
				ball.m_velocity.x = ball.m_velocity.x * -combinedElasticity;
			}
		}

		// Right Wall
		if ((ball.m_center.x + ball.m_radius) > m_rightWallX)
		{
			// Push
			ball.m_center.x = m_rightWallX - ball.m_radius;

			if (ball.m_velocity.x > 0.f)
			{
				ball.m_velocity.x = ball.m_velocity.x * -combinedElasticity;
			}
		}

		// Bottom Wall
		if (m_isBottomWallPresent)
		{
			if (ball.m_center.y - ball.m_radius < m_bottomWallY)
			{
				ball.m_center.y = m_bottomWallY + ball.m_radius;

				if (ball.m_velocity.y < 0.f)
				{
					ball.m_velocity.y = ball.m_velocity.y * -combinedElasticity;
				}
			}
		}
		else
		{
			if (ball.m_center.y + ball.m_radius < m_bottomWallY)
			{
				ball.m_center.y = m_topPortalY + ball.m_radius;
			}
		}
	}
}

void Game2DPachinkoMachine::SpawnBall()
{
	PachinkoMachine::Ball ball;
	ball.m_center = m_startPos;
	ball.m_radius = g_rng.RollRandomFloatInRange(PachinkoMachine::MIN_BALLRADIUS, PachinkoMachine::MAX_BALLRADIUS);
	ball.m_velocity = 3 * (m_endPos - m_startPos);
	float t = g_rng.RollRandomFloatZeroToOne();
	ball.m_color = Interpolate(LIGHT_BLUE, DARK_BLUE, t); // TODO also spectrum but darker, when rendering set center to be white?

	m_balls.push_back(ball);
}

void Game2DPachinkoMachine::DrawObjects() const
{
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);


	g_theRenderer->DrawVertexArray(m_bumperVerts);

	std::vector<Vertex_PCU> verts;
	verts.reserve(m_balls.size() * 96 + 32 * 12 + 12);
	AddVertsForWalls(verts);
	AddVertsForBalls(verts);
	AddVertsForLauncher(verts);
	g_theRenderer->DrawVertexArray(verts);
}

void Game2DPachinkoMachine::AddVertsForLauncher(std::vector<Vertex_PCU>& verts) const
{
	AddVertsForArrow2D(verts, m_startPos, m_endPos, PachinkoMachine::ARROWSIZE, PachinkoMachine::LINETHICKNESS, Rgba8::OPAQUE_WHITE);
	AddVertsForRing2D(verts, m_startPos, PachinkoMachine::MIN_BALLRADIUS, PachinkoMachine::LINETHICKNESS, DARK_BLUE);
	AddVertsForRing2D(verts, m_startPos, PachinkoMachine::MAX_BALLRADIUS, PachinkoMachine::LINETHICKNESS, DARK_BLUE);
}

void Game2DPachinkoMachine::AddVertsForBalls(std::vector<Vertex_PCU>& verts) const
{
	int numBalls = (int)m_balls.size();
	for (int i = 0; i < numBalls; ++i)
	{
		PachinkoMachine::Ball const& ball = m_balls[i];
		AddVertsForGradientDisc2D(verts, ball.m_center, ball.m_radius, Rgba8::OPAQUE_WHITE, ball.m_color);
	}
}

void Game2DPachinkoMachine::AddVertsForWalls(std::vector<Vertex_PCU>& verts) const
{
	AddVertsForAABB2D(verts, AABB2(m_leftWallX - 200.f, m_bottomWallY, m_leftWallX, m_topPortalY), Rgba8(32, 26, 96));
	AddVertsForAABB2D(verts, AABB2(m_rightWallX, m_bottomWallY, m_rightWallX + 200.f, m_topPortalY), Rgba8(32, 26, 96));
}

void Game2DPachinkoMachine::DrawPachinkoMachine() const
{
	if (m_isBottomWallPresent)
	{
		std::vector<Vertex_PCU> verts;

		AddVertsForAABB2D(verts, AABB2(m_leftWallX, m_bottomWallY - SCREEN_SIZE_X * 0.125f, m_rightWallX, m_bottomWallY), Rgba8::OPAQUE_WHITE);

		Texture* banner = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/pachinko_banner.png");
		g_theRenderer->BindTexture(banner);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(verts);
	}

}

Vec2 Game2DPachinkoMachine::MapMouseCursorToWorldPoint() const
{
	// very special case, ortho cam, xy plane.
	Vec2 clientUV = g_theInput->GetCursorNormalizedPosition();
	AABB2 cameraBound(m_camera.GetOrthographicBottomLeft(), m_camera.GetOrthographicTopRight());
	Vec2 mousePosInCameraSpace = cameraBound.GetPointAtUV(clientUV);

	Mat44 camMat44 = m_camera.GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();

	Vec3 mouseWorldPos = m_camera.GetPosition() + (camMat44.GetJBasis3D() * -mousePosInCameraSpace.x) + (camMat44.GetKBasis3D() * mousePosInCameraSpace.y);
	return Vec2(mouseWorldPos.x, mouseWorldPos.y);
}

Vec2 Game2DPachinkoMachine::GetRandomUVs() const
{
	return Vec2(g_rng.RollRandomFloatZeroToOne(), g_rng.RollRandomFloatZeroToOne());
}

