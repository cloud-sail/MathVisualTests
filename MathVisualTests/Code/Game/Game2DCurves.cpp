#include "Game/Game2DCurves.hpp"
#include "Game/App.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

static const char* G2C_TEXT = "Easing, Curves, Splines (2D): W/E = prev/next Easing function; N/M = curve subdivisions (%d),  F1 Highlight Layout";
static std::string SENTENCE = "The quick brown fox jumps over the lazy dog";
static constexpr float G2C_POINT_RADIUS = 4.f;
static constexpr float G2C_LINE_WIDTH = 2.5f;

//-----------------------------------------------------------------------------------------------
Game2DCurves::Game2DCurves()
{
	//m_camera.SetCameraToRenderTransform(Mat44::DIRECTX_C2R);
	//m_camera.SetPositionAndOrientation(Vec3(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f, 0.5f), EulerAngles(90.f, 90.f, 30.f));

	AABB2 const clientDimensions = AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y);
	float constexpr PADDING = -15.f;

	AABB2 panel = clientDimensions;
	panel.ChopOffTop(0.05f);

	m_topLeftQuarterPane = panel;
	m_topLeftQuarterPane.ChopOffBottom(0.5f);
	m_topLeftQuarterPane.ChopOffRight(0.5f);
	m_topLeftQuarterPane.AddPadding(PADDING, PADDING);

	m_topRightQuarterPane = panel;
	m_topRightQuarterPane.ChopOffBottom(0.5f);
	m_topRightQuarterPane.ChopOffLeft(0.5f);
	m_topRightQuarterPane.AddPadding(PADDING, PADDING);

	m_bottomHalfPane = panel;
	m_bottomHalfPane.ChopOffTop(0.5f);
	m_bottomHalfPane.AddPadding(PADDING, PADDING);


	m_easingFunctions = {
		{ SmoothStart2, "SmoothStart2 (EaseInQuadratic)" },
		{ SmoothStart3, "SmoothStart3 (EaseInCubic)" },
		{ SmoothStart4, "SmoothStart4 (EaseInQuartic)" },
		{ SmoothStart5, "SmoothStart5 (EaseInQuintic)" },
		{ SmoothStart6, "SmoothStart6 (EaseIn6thOrder)" },
		{ SmoothEnd2,   "SmoothEnd2 (EaseOutQuadratic)" },
		{ SmoothEnd3,   "SmoothEnd3 (EaseOutCubic)" },
		{ SmoothEnd4,   "SmoothEnd4 (EaseOutQuartic)" },
		{ SmoothEnd5,   "SmoothEnd5 (EaseOutQuintic)" },
		{ SmoothEnd6,   "SmoothEnd6 (EaseOut6thOrder)" },
		{ SmoothStep3,	"SmoothStep3 (EaseInOutCubic)" },
		{ SmoothStep5,	"SmoothStep5 (EaseInOutQuintic)" },
		{ Hesitate3,	"Hesitate3" },
		{ Hesitate5,	"Hesitate5" },
		{ BounceEndBezier5,	"Funky BounceEnd" },

	};

	RandomizeSceneObjects();
}

Game2DCurves::~Game2DCurves()
{

}

void Game2DCurves::Update()
{
	UpdateDeveloperCheats();

	HandleInput();

	UpdateCameras();
}

void Game2DCurves::Render() const
{
	g_theRenderer->BeginCamera(m_camera);

	DrawLayout();
	DrawEasingFunction();
	DrawCubicCurve();
	DrawCubicSpline();
	DrawObjects();



	DrawUsage();


	g_theRenderer->EndCamera(m_camera);
}

void Game2DCurves::UpdateCameras()
{
	//Vec2 halfDimension = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);
	//Vec2 cameraCenter = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);
	//m_camera.SetOrthographicView(-halfDimension, halfDimension);

	//m_camera.SetPositionAndOrientation(Vec3(cameraCenter, 0.5f), EulerAngles(90.f, 90.f, (float)m_clock->GetTotalSeconds() * 90.f));
	
	m_camera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game2DCurves::DrawUsage() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 usageBox(SCREEN_SIZE_X * 0.005f, SCREEN_SIZE_Y * 0.93f, SCREEN_SIZE_X * 0.9995f, SCREEN_SIZE_Y * 0.99f);
	Vec2 alignment(0.f, 1.f);
	float cellAspect = 0.6f;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	std::string usageText = Stringf(G2C_TEXT, m_numSubdivisions);
	testFont->AddVertsForTextInBox2D(verts, GAME_TEXT + '\n' + usageText, usageBox, 20.f, Rgba8(255, 150, 50), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&testFont->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Game2DCurves::RandomizeSceneObjects()
{
	// Easing Functions
	m_currentEasingIndex = 0;

	// Cubic Curve
	m_topRightQuarterPane;

	m_cubicCurvePoints[0] = m_topRightQuarterPane.GetPointAtUV(GetRandomUVs());
	m_cubicCurvePoints[1] = m_topRightQuarterPane.GetPointAtUV(GetRandomUVs());
	m_cubicCurvePoints[2] = m_topRightQuarterPane.GetPointAtUV(GetRandomUVs());
	m_cubicCurvePoints[3] = m_topRightQuarterPane.GetPointAtUV(GetRandomUVs());

	m_cubicCurve.ClearAllSplinePoints();

	m_cubicCurve.AddPoint(SplinePoint2D::MakeFromContinuousCubicBezierFromNextGuidePos(0.f, m_cubicCurvePoints[0], m_cubicCurvePoints[1]), false);
	m_cubicCurve.AddPoint(SplinePoint2D::MakeFromContinuousCubicBezierFromPrevGuidePos(1.f, m_cubicCurvePoints[3], m_cubicCurvePoints[2]), false);

	// Cubic Splines

	int numPoints = g_rng.RollRandomIntInRange(3, 6);
	m_cubicSplinePoints.clear();
	for (int i = 0; i < numPoints; ++i)
	{
		m_cubicSplinePoints.push_back(m_bottomHalfPane.GetPointAtUV(GetRandomUVs()));
	}
	BubbleSortVec2X(m_cubicSplinePoints);

	m_cubicSpline.ClearAllSplinePoints();
	m_cubicSpline.SetFromCatmullRomAlgorithm(m_cubicSplinePoints, false);


	RefreshCurves();
}

void Game2DCurves::HandleInput()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_isLayoutHighlighted = !m_isLayoutHighlighted;
	}

	int numEasingFunctions = (int)m_easingFunctions.size();
	if (g_theInput->WasKeyJustPressed(KEYCODE_W))
	{
		m_currentEasingIndex = (m_currentEasingIndex + numEasingFunctions - 1) % numEasingFunctions;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_E))
	{
		m_currentEasingIndex = (m_currentEasingIndex + 1) % numEasingFunctions;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_N))
	{
		m_numSubdivisions /= 2;
		if (m_numSubdivisions < 1)
		{
			m_numSubdivisions = 1;
		}
		RefreshCurves();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_M))
	{
		m_numSubdivisions *= 2;
		RefreshCurves();
	}
}


void Game2DCurves::DrawLayout() const
{
	Rgba8 const color = Rgba8(128, 0, 0);
	if (m_isLayoutHighlighted)
	{
		std::vector<Vertex_PCU> verts;

		AddVertsForAABB2D(verts, m_topLeftQuarterPane, color);
		AddVertsForAABB2D(verts, m_topRightQuarterPane, color);
		AddVertsForAABB2D(verts, m_bottomHalfPane, color);

		g_theRenderer->BindTexture(nullptr);

		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

		g_theRenderer->DrawVertexArray(verts);
	}
}

void Game2DCurves::DrawEasingFunction() const
{
	EasingFunction1D* currentFunc = m_easingFunctions[m_currentEasingIndex].m_func;
	std::string currentName = m_easingFunctions[m_currentEasingIndex].m_name;

	AABB2 textBox = m_topLeftQuarterPane;
	textBox.ChopOffTop(0.9f);

	AABB2 plotBox = m_topLeftQuarterPane;
	plotBox.ChopOffBottom(0.11f);
	plotBox.ReduceToAspect(1.f);

	//-----------------------------------------------------------------------------------------------
	std::vector<Vertex_PCU> shapeVerts;
	shapeVerts.reserve(6 + (DEFAULT_SUBDIVISION + 1) * 6 + (m_numSubdivisions + 1) * 6 + 32 * 3 + 6 * 2);

	// Background (6)
	AddVertsForAABB2D(shapeVerts, plotBox, DARKER_BLUE);

	// Default Curve ((DEFAULT_SUBDIVISION + 1) * 6)
	std::vector<Vec2> greyCurvePoints;
	greyCurvePoints.reserve(DEFAULT_SUBDIVISION + 1);
	for (int i = 0; i <= DEFAULT_SUBDIVISION; ++i)
	{
		float x = static_cast<float>(i) * DEFAULT_STEP;
		float y = currentFunc(x);

		Vec2 point = plotBox.GetPointAtUV(Vec2(x, y));
		greyCurvePoints.push_back(point);
	}
	AddVertsForSimpleLine2D(shapeVerts, greyCurvePoints, G2C_LINE_WIDTH, DARK_GREY);

	// Curve ((m_numSubdivisions + 1) * 6)
	std::vector<Vec2> curvePoints;
	curvePoints.reserve(m_numSubdivisions + 1);
	float step = 1.f / static_cast<float>(m_numSubdivisions);
	for (int i = 0; i <= m_numSubdivisions; ++i)
	{
		float x = static_cast<float>(i) * step;
		float y = currentFunc(x);

		Vec2 point = plotBox.GetPointAtUV(Vec2(x, y));
		curvePoints.push_back(point);
	}

	AddVertsForSimpleLine2D(shapeVerts, curvePoints, G2C_LINE_WIDTH, Rgba8::GREEN);

	// Current Point and Projection Line (32*3 + 6*2)
	float t = GetCurrentFraction();
	Vec2 pos;
	pos.x = t;
	pos.y = currentFunc(pos.x);
	pos = plotBox.GetPointAtUV(pos);

	AddVertsForLineSegment2D(shapeVerts, pos, Vec2(pos.x, plotBox.m_mins.y), G2C_LINE_WIDTH, TRANSLUCENT_WHITE);
	AddVertsForLineSegment2D(shapeVerts, pos, Vec2(plotBox.m_mins.x, pos.y), G2C_LINE_WIDTH, TRANSLUCENT_WHITE);
	AddVertsForDisc2D(shapeVerts, pos, G2C_POINT_RADIUS, Rgba8::OPAQUE_WHITE);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(shapeVerts);

	//-----------------------------------------------------------------------------------------------
	// Text
	std::vector<Vertex_PCU> textVerts;
	textVerts.reserve(6);
	BitmapFont* font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");


	font->AddVertsForTextInBox2D(textVerts, currentName, textBox, 20.f, Rgba8(255, 150, 50), 0.6f, Vec2(0.5f, 0.5f), TextBoxMode::SHRINK_TO_FIT);
	g_theRenderer->BindTexture(&font->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(textVerts);

}

void Game2DCurves::DrawCubicCurve() const
{
	std::vector<Vertex_PCU> shapeVerts;
	shapeVerts.reserve(18 + 64 * 6 + m_numSubdivisions * 6 + 32 * 3 * 4 + 32 * 3 * 3);


	// ABCD ( 18 )
	std::vector<Vec2> ABCD(m_cubicCurvePoints, m_cubicCurvePoints + 4);

	AddVertsForSimpleLine2D(shapeVerts, ABCD, G2C_LINE_WIDTH, DARKER_BLUE);

	// Curve 64 * 6
	std::vector<Vec2> curvePoints1;
	curvePoints1.reserve(64 + 1);
	m_cubicCurve.GetPositionListWithSubdivisions(curvePoints1, 64);
	AddVertsForSimpleLine2D(shapeVerts, curvePoints1, G2C_LINE_WIDTH, DARK_GREY);

	// Curve m_numSubdivisions * 6
	std::vector<Vec2> curvePoints;
	curvePoints.reserve(m_numSubdivisions + 1);
	m_cubicCurve.GetPositionListWithSubdivisions(curvePoints, m_numSubdivisions);
	AddVertsForSimpleLine2D(shapeVerts, curvePoints, G2C_LINE_WIDTH, Rgba8::GREEN);

	// ABCD points (32 * 3 * 4)
	AddVertsForDisc2D(shapeVerts, m_cubicCurvePoints[0], G2C_POINT_RADIUS * 0.8f, LIGHT_BLUE);
	AddVertsForDisc2D(shapeVerts, m_cubicCurvePoints[1], G2C_POINT_RADIUS * 0.8f, LIGHT_BLUE);
	AddVertsForDisc2D(shapeVerts, m_cubicCurvePoints[2], G2C_POINT_RADIUS * 0.8f, LIGHT_BLUE);
	AddVertsForDisc2D(shapeVerts, m_cubicCurvePoints[3], G2C_POINT_RADIUS * 0.8f, LIGHT_BLUE);

	// white dot (32 * 3)
	float inputKey = GetCurrentFraction();
	Vec2 whiteDotPos = m_cubicCurve.GetPositionAtInputKey(inputKey);
	AddVertsForDisc2D(shapeVerts, whiteDotPos, G2C_POINT_RADIUS, Rgba8::OPAQUE_WHITE);


	// red dot (32 * 3)
	float curveDistance = inputKey * m_cubicCurve.GetSplineLength();
	float tempKey = m_cubicCurve.GetInputKeyAtDistanceAlongSpline(curveDistance);
	Vec2 redDotPos = m_cubicCurve.GetPositionAtInputKey(tempKey);
	AddVertsForDisc2D(shapeVerts, redDotPos, G2C_POINT_RADIUS, Rgba8::RED);

	// green dot (32 * 3)
	float linearCurveDistance = inputKey * m_linearCubicCurve.GetSplineLength();
	float tempKey2 = m_linearCubicCurve.GetInputKeyAtDistanceAlongSpline(linearCurveDistance);
	Vec2 greenDotPos = m_linearCubicCurve.GetPositionAtInputKey(tempKey2);
	AddVertsForDisc2D(shapeVerts, greenDotPos, G2C_POINT_RADIUS, LIGHT_GREEN);



	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(shapeVerts);
}

void Game2DCurves::DrawCubicSpline() const
{
	int numPoints = (int)m_cubicSplinePoints.size();
	int numSegments = (numPoints - 1);

	std::vector<Vertex_PCU> shapeVerts;
	shapeVerts.reserve(numSegments * 6 + 64 * numSegments * 6 + m_numSubdivisions * numSegments * 6 + 18 * (numPoints - 2) + numPoints * 32 * 3 + 32 * 3 * 3);

	// line 012... (numSegments * 6)
	AddVertsForSimpleLine2D(shapeVerts, m_cubicSplinePoints, G2C_LINE_WIDTH, DARKER_BLUE);

	// Curve 64 * numSegments * 6
	std::vector<Vec2> curvePoints1;
	curvePoints1.reserve(64 * numSegments + 1);
	m_cubicSpline.GetPositionListWithSubdivisions(curvePoints1, 64);
	AddVertsForSimpleLine2D(shapeVerts, curvePoints1, G2C_LINE_WIDTH, DARK_GREY);

	// Curve m_numSubdivisions * numSegments * 6
	std::vector<Vec2> curvePoints2;
	curvePoints2.reserve(m_numSubdivisions * numSegments + 1);
	m_cubicSpline.GetPositionListWithSubdivisions(curvePoints2, m_numSubdivisions);
	AddVertsForSimpleLine2D(shapeVerts, curvePoints2, G2C_LINE_WIDTH, Rgba8::GREEN);


	// Arrow 18 * (numPoints - 2)
	for (int i = 1; i <= (numPoints-2); ++i)
	{
		Vec2 startPos = m_cubicSplinePoints[i];
		Vec2 tangent = m_cubicSpline.GetTangentAtInputKey((float)i);
		AddVertsForArrow2D(shapeVerts, startPos, startPos + tangent, 10.f, 2.f, Rgba8::RED);
	}

	// dot numPoints * 32 * 3
	for (int i = 0; i < numPoints; ++i)
	{
		AddVertsForDisc2D(shapeVerts, m_cubicSplinePoints[i], G2C_POINT_RADIUS * 0.8f, LIGHT_BLUE);
	}

	float duration = static_cast<float>(numSegments);
	// white dot (32 * 3)
	float inputKey = GetCurrentFraction(numSegments);
	Vec2 whiteDotPos = m_cubicSpline.GetPositionAtInputKey(inputKey);
	AddVertsForDisc2D(shapeVerts, whiteDotPos, G2C_POINT_RADIUS, Rgba8::OPAQUE_WHITE);


	// red dot (32 * 3)
	float splineDistance = inputKey / duration * m_cubicSpline.GetSplineLength();
	float tempKey = m_cubicSpline.GetInputKeyAtDistanceAlongSpline(splineDistance);
	Vec2 redDotPos = m_cubicSpline.GetPositionAtInputKey(tempKey);
	AddVertsForDisc2D(shapeVerts, redDotPos, G2C_POINT_RADIUS, Rgba8::RED);

	// green dot (32 * 3)
	float linearSplineDistance = inputKey / duration * m_linearCubicSpline.GetSplineLength();
	float tempKey2 = m_linearCubicSpline.GetInputKeyAtDistanceAlongSpline(linearSplineDistance);
	Vec2 greenDotPos = m_linearCubicSpline.GetPositionAtInputKey(tempKey2);
	AddVertsForDisc2D(shapeVerts, greenDotPos, G2C_POINT_RADIUS, LIGHT_GREEN);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(shapeVerts);


	//-----------------------------------------------------------------------------------------------
	// Spline Text
	std::vector<Vertex_PCU> textVerts;
	textVerts.reserve(200);
	BitmapFont* font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");


	font->AddVertsForTextOnSpline2D(textVerts, m_cubicSpline, 15.f, SENTENCE, Rgba8::CYAN, 0.7f, splineDistance);
	g_theRenderer->BindTexture(&font->GetTexture());

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(textVerts);

}

void Game2DCurves::DrawObjects() const
{
	std::vector<Vertex_PCU> verts;


	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

float Game2DCurves::GetCurrentFraction(int numSegment) const
{
	float currentTime = static_cast<float>(m_clock->GetTotalSeconds());
	return  fmodf((currentTime * 0.5f), (float)numSegment);
}


Vec2 Game2DCurves::GetRandomUVs()
{
	return Vec2(g_rng.RollRandomFloatZeroToOne(), g_rng.RollRandomFloatZeroToOne());
}

void Game2DCurves::RefreshCurves()
{
	//-----------------------------------------------------------------------------------------------
	m_cubicCurve.SetSubdivisionsPerSegment(m_numSubdivisions);

	m_linearCubicCurve.ClearAllSplinePoints();
	std::vector<Vec2> curvePoints;
	curvePoints.reserve(m_numSubdivisions + 1);
	m_cubicCurve.GetPositionListWithSubdivisions(curvePoints, m_numSubdivisions);


	for (int i = 0; i < (int)curvePoints.size(); ++i)
	{
		m_linearCubicCurve.AddPoint(SplinePoint2D((float) i, curvePoints[i]), false);
	}
	m_linearCubicCurve.SetSubdivisionsPerSegment(1);

	//-----------------------------------------------------------------------------------------------
	m_cubicSpline.SetSubdivisionsPerSegment(m_numSubdivisions);

	m_linearCubicSpline.ClearAllSplinePoints();
	std::vector<Vec2> splinePoints;
	splinePoints.reserve(m_numSubdivisions * 6 + 1); // wrong
	m_cubicSpline.GetPositionListWithSubdivisions(splinePoints, m_numSubdivisions);

	for (int i = 0; i < (int)splinePoints.size(); ++i)
	{
		m_linearCubicSpline.AddPoint(SplinePoint2D((float)i, splinePoints[i]), false);
	}
	m_linearCubicSpline.SetSubdivisionsPerSegment(1);

}

void Game2DCurves::BubbleSortVec2X(std::vector<Vec2>& points)
{
	int n = (int)points.size();
	bool swapped;

	for (int i = 0; i < n - 1; ++i)
	{
		swapped = false;
		for (int j = 0; j < n - i - 1; ++j) {
			if (points[j].x > points[j + 1].x) {
				Vec2 temp = points[j];
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
