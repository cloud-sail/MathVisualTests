#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"


Game::Game()
{
	m_clock = new Clock();
}

Game::~Game()
{
	delete m_clock;
}

CursorMode Game::GetCursorMode()
{
	return m_cursorMode;
}

void Game::UpdateDeveloperCheats()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		g_isDebugDraw = !g_isDebugDraw;
	}

	m_clock->SetTimeScale(g_theInput->IsKeyDown(KEYCODE_T) ? 0.1 : 1.0);
	if (g_theInput->WasKeyJustPressed(KEYCODE_P))
	{
		m_clock->TogglePause();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_O))
	{
		m_clock->StepSingleFrame();
	}
}
