#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include <vector>

//-----------------------------------------------------------------------------------------------
class InputSystem;
class Window;
class Renderer;
class App;
class RandomNumberGenerator;
class Clock;
class Texture;

struct Mat44;
struct Vec2;
struct Vec3;
struct Rgba8;
struct Vertex_PCU;
struct AABB3;

//-----------------------------------------------------------------------------------------------
extern InputSystem*		g_theInput;
extern Renderer*		g_theRenderer;
extern Window*			g_theWindow;
extern App*				g_theApp;
extern RandomNumberGenerator g_rng;

//-----------------------------------------------------------------------------------------------
extern bool g_isDebugDraw;


//-----------------------------------------------------------------------------------------------
enum GameMode
{
	GAME_MODE_NEAREST_POINT,
	GAME_MODE_RAYCAST_VS_DISCS,
	GAME_MODE_RAYCAST_VS_LINE_SEGMENTS,
	GAME_MODE_RAYCAST_VS_AABBS,
	GAME_MODE_2D_CURVES,
	GAME_MODE_2D_PACHIKO_MACHINE,
	GAME_MODE_2D_FAST_VOXEL,
	GAME_MODE_2D_EXPOSURE_AVOIDANCE,
	GAME_MODE_2D_FLOW_FIELD,
	GAME_MODE_3D_TEST_SHAPES,
	GAME_MODE_3D_QUATERNION,
	GAME_MODE_3D_CURVES,
	GAME_MODE_NUM
};

const std::string GAME_TEXT = "F6 - previous, F7 - next, F8 - re-randomize, T - Slow motion";

constexpr float WINDOW_ASPECT = 2.f;
//-----------------------------------------------------------------------------------------------
// Gameplay Constants
constexpr float SCREEN_SIZE_X = 1600.f;
constexpr float SCREEN_SIZE_Y = 800.f;

constexpr float CAMERA_MOVE_SPEED = 5.f;
constexpr float CAMERA_YAW_TURN_RATE = 60.f;
constexpr float CAMERA_PITCH_TURN_RATE = 60.f;
constexpr float CAMERA_ROLL_TURN_RATE = 90.f;

constexpr float CAMERA_MAX_PITCH = 89.9f;


const Rgba8 LIGHT_BLUE{ 0, 128, 255 };
const Rgba8 DARK_BLUE{ 0, 90, 179 };
const Rgba8 DARKER_BLUE{ 0, 45, 90 };
const Rgba8 DARK_GREY{ 85, 85, 85 };
const Rgba8 ORANGE{ 255, 128, 0 };
const Rgba8 TRANSLUCENT_WHITE{ 255, 255, 255, 30 };
const Rgba8 LIGHT_GREEN{ 181, 230, 29 };

constexpr float RAYCAST_MOVESPEED = 200.f;
constexpr float RAYCAST_WHITE_DOT_RADIUS = 6.f;
constexpr float RAYCAST_LINE_THICKNESS = 3.f;
constexpr float RAYCAST_ARROW_SIZE = 25.f;
constexpr float RAYCAST_NORMAL_ARROW_LENGTH = 100.f;



//-----------------------------------------------------------------------------------------------
Vec2 MapMouseCursorToWorldCoords2D(AABB2 const& cameraBounds, AABB2 const& viewport = AABB2::ZERO_TO_ONE);

void AddVertsForSimpleLine2D(std::vector<Vertex_PCU>& verts, std::vector<Vec2> const& points, float thickness, Rgba8 const& color);

void AddVertsForColoredCube3D(std::vector<Vertex_PCU>& verts);
void AddVertsForColoredAABB3D(std::vector<Vertex_PCU>& verts, AABB3 const& bounds);
void AddVertsForScratchyLines(std::vector<Vertex_PCU>& verts, std::vector<Vec3> points, float radius, Rgba8 const& color = Rgba8::OPAQUE_WHITE);
