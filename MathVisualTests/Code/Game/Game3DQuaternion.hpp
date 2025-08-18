#pragma once
#include "Game/Game.hpp"
#include "Engine/Math/Quat.hpp"
#include "Engine/Renderer/Camera.hpp"


struct Game3DQuatTransform
{
	Quat m_rotation;
	Vec3 m_position;
	// no scale
};


// ----------------------------------------------------------------------------------------------
class Game3DQuaternion : public Game
{
public:
	Game3DQuaternion();
	virtual ~Game3DQuaternion();
	virtual void Update() override;
	virtual void Render() const override;
	virtual void RandomizeSceneObjects() override;

protected:
	virtual void UpdateCameras() override;
	virtual void DrawUsage() const override;

	void UpdatePlayer();
	void UpdatePlayerPosition(float deltaSeconds);
	void UpdatePlayerOrientation();
	void UpdatePlayerCameraCenterAxis();

	void InitializeModel();
	void InitializeTransforms();

	void UpdateObjects();

	void DrawObjects() const;

private:
	void HandleInput();

private:
	Camera m_screenCamera;

	Camera m_camera;
	Vec3 m_playerPosition = Vec3(2.f, 2.f, 2.f);
	EulerAngles m_playerOrientation = EulerAngles(-135.f, 0.f, 0.f);

	std::vector<Vertex_PCU> m_modelVerts;
	Texture* m_modelTexture = nullptr;

	// Quaternion List (random initialization play 0,1,2,3,0,1,2,3)
	Quat m_quatList[10];

	// CurrentQuaternion for each type
	// Position for each type

	Game3DQuatTransform m_lerp;
	Game3DQuatTransform m_nlerp;
	Game3DQuatTransform m_slerp;
	Game3DQuatTransform m_nlerpFullPath;
	Game3DQuatTransform m_slerpFullPath;


};