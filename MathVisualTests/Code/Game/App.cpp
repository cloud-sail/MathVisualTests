#include "Game/App.hpp"
#include "Game/GameNearestPoint.hpp"
#include "Game/GameRaycastVsDiscs.hpp"
#include "Game/GameRaycastVsLineSegments.hpp"
#include "Game/GameRaycastVsAABBs.hpp"
#include "Game/Game3DTestShapes.hpp"
#include "Game/Game2DCurves.hpp"
#include "Game/Game2DPachinkoMachine.hpp"
#include "Game/Game2DFastVoxelRaycast.hpp"
#include "Game/Game2DExposureAvoidance.hpp"
#include "Game/Game2DFlowField.hpp"
#include "Game/Game3DQuaternion.hpp"
#include "Game/Game3DCurves.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/DX11Renderer.hpp"
#include "Engine/Window/Window.hpp"


//-----------------------------------------------------------------------------------------------
App*			g_theApp		= nullptr;		// Created and owned by Main_Windows.cpp
Window*			g_theWindow		= nullptr;		// Created and owned by the App
Renderer*		g_theRenderer	= nullptr;		// Created and owned by the App
bool			g_isDebugDraw	= false;
RandomNumberGenerator g_rng;

//-----------------------------------------------------------------------------------------------
bool OnQuitEvent(EventArgs& args)
{
	UNUSED(args);
	g_theApp->HandleQuitRequested();
	return false;
}

//-----------------------------------------------------------------------------------------------
App::App()
{
}

App::~App()
{
}

void App::Startup()
{
	// Parse Data/GameConfig.xml
	LoadGameConfig("Data/GameConfig.xml");

	// Create all Engine subsystems
	EventSystemConfig eventSystemConfig;
	g_theEventSystem = new EventSystem(eventSystemConfig);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_aspectRatio = WINDOW_ASPECT;
	windowConfig.m_windowTitle = "Math Visual Tests";
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;
	g_theRenderer = new DX11Renderer(rendererConfig);

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	debugRenderConfig.m_messageCellHeight = 15.f;
	debugRenderConfig.m_messageAspectRatio = 0.75f;


	// Start up all Engine subsystems
	g_theEventSystem->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	DebugRenderSystemStartup(debugRenderConfig);

	// Initialize game-related stuff: create and start the game
	g_theEventSystem->SubscribeEventCallbackFunction("Quit", OnQuitEvent);
	m_theGame = CreateNewGameForMode(m_currentGameMode);
}

void App::Shutdown()
{
	// Destroy game-related stuff
	delete m_theGame;
	m_theGame = nullptr;

	// Shut down all Engine subsystems
	DebugRenderSystemShutdown();
	g_theRenderer->Shutdown();
	g_theWindow->Shutdown();
	g_theInput->Shutdown();
	g_theEventSystem->Shutdown();

	// Destroy all engine subsystems
	delete g_theRenderer;
	g_theRenderer = nullptr;
	delete g_theWindow;
	g_theWindow = nullptr;
	delete g_theInput;
	g_theInput = nullptr;
	delete g_theEventSystem;
	g_theEventSystem = nullptr;
}

void App::RunMainLoop()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

void App::RunFrame()
{
	Clock::TickSystemClock();

	BeginFrame();			// Engine pre-frame stuff
	Update();	// Game updates / moves/ spawns / hurts/ kills stuffs
	Render();				// Game draws current state of things
	EndFrame();				// Engine post-frame stuff
}

void App::HandleQuitRequested()
{
	m_isQuitting = true;
}

void App::BeginFrame()
{
	g_theEventSystem->BeginFrame();
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	DebugRenderBeginFrame();
}

void App::Update()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE))
	{
		g_theApp->HandleQuitRequested();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		m_theGame->RandomizeSceneObjects();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		delete m_theGame;
		m_currentGameMode = static_cast<GameMode>((m_currentGameMode + 1) % GAME_MODE_NUM);
		m_theGame = CreateNewGameForMode(m_currentGameMode);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		delete m_theGame;
		m_currentGameMode = static_cast<GameMode>((m_currentGameMode + GAME_MODE_NUM - 1) % GAME_MODE_NUM);
		m_theGame = CreateNewGameForMode(m_currentGameMode);
	}

	// No devconsole in MathVisualTests Now, no need to check g_theDevConsole->GetMode() == DevConsoleMode::HIDDEN
	if (!g_theWindow->IsFocused())
	{
		g_theInput->SetCursorMode(CursorMode::POINTER);
	}
	else
	{
		g_theInput->SetCursorMode(m_theGame->GetCursorMode());
	}

	m_theGame->Update();
}

void App::Render() const
{
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0));
	m_theGame->Render();
}

void App::EndFrame()
{
	DebugRenderEndFrame();
	g_theRenderer->EndFrame();
	g_theWindow->EndFrame();
    g_theInput->EndFrame();
	g_theEventSystem->EndFrame();
}

Game* App::CreateNewGameForMode(GameMode mode)
{
	switch (mode)
	{
	case GAME_MODE_NEAREST_POINT:
		return new GameNearestPoint();
		break;
	case GAME_MODE_RAYCAST_VS_DISCS:
		return new GameRaycastVsDiscs();
		break;
	case GAME_MODE_RAYCAST_VS_LINE_SEGMENTS:
		return new GameRaycastVsLineSegments();
		break;
	case GAME_MODE_RAYCAST_VS_AABBS:
		return new GameRaycastVsAABBs();
		break;
	case GAME_MODE_3D_TEST_SHAPES:
		return new Game3DTestShapes();
		break;
	case GAME_MODE_2D_CURVES:
		return new Game2DCurves();
		break;
	case GAME_MODE_2D_PACHIKO_MACHINE:
		return new Game2DPachinkoMachine();
		break;
	case GAME_MODE_2D_FAST_VOXEL:
		return new Game2DFastVoxelRaycast();
		break;
	case GAME_MODE_2D_EXPOSURE_AVOIDANCE:
		return new Game2DExposureAvoidance();
		break;
	case GAME_MODE_2D_FLOW_FIELD:
		return new Game2DFlowField();
		break;
	case GAME_MODE_3D_QUATERNION:
		return new Game3DQuaternion();
		break;
	case GAME_MODE_3D_CURVES:
		return new Game3DCurves();
		break;
	}
	ERROR_AND_DIE("Invalid Game Mode");
}

void App::LoadGameConfig(char const* gameConfigXmlFilePath)
{
	XmlDocument gameConfigXml;
	XmlResult result = gameConfigXml.LoadFile(gameConfigXmlFilePath);
	if (result != tinyxml2::XML_SUCCESS)
	{
		DebuggerPrintf("WARNING: failed to load game config from file \"%s\"\n", gameConfigXmlFilePath);
		return;
	}
	XmlElement* rootElement = gameConfigXml.RootElement();
	if (rootElement == nullptr)
	{
		DebuggerPrintf("WARNING: game config from file \"%s\" was invalid (missing root element)\n", gameConfigXmlFilePath);
		return;
	}
	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
}
