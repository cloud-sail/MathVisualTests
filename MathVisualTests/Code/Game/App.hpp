#pragma once
#include "Game/GameCommon.hpp"

//-----------------------------------------------------------------------------------------------
class Game;

//-----------------------------------------------------------------------------------------------
class App
{
public:
    App();
    ~App();
    void Startup();
    void Shutdown();
    void RunMainLoop();
    void RunFrame();

    void HandleQuitRequested();
    bool IsQuitting() const { return m_isQuitting; }

private:
    void BeginFrame();
    void Update();
    void Render() const;
    void EndFrame();
    
    void LoadGameConfig(char const* gameConfigXmlFilePath);
    Game* CreateNewGameForMode(GameMode mode);
private:
    GameMode m_currentGameMode = GAME_MODE_3D_CURVES;
	Game* m_theGame = nullptr;
    bool m_isQuitting = false;
};
