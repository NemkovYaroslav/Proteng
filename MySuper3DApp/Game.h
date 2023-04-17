#pragma once
#include "includes.h"

class DisplayWin32;
class InputDevice;
class RenderSystem;
class RenderShadows;
class GameObject;
class CameraComponent;
class DirectionalLightComponent;
class PointLightComponent;

class Game
{
public:

	LPCWSTR	name;
	int clientWidth;
	int clientHeight;
	static Game* instance;
	Game(LPCWSTR name, int clientWidth, int clientHeight);
	Game(const Game&) = delete;
	void operator = (const Game&) = delete;
	virtual ~Game() = default;
	static Game* CreateInstance(LPCWSTR name, int screenWidth, int screenHeight);
	static Game* GetInstance();
	float totalTime;
	float deltaTime;
	unsigned int frameCount;
	std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>> startTime;
	std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>> prevTime;
	std::shared_ptr<DisplayWin32>  display;
	std::shared_ptr<InputDevice>   inputDevice;
	std::shared_ptr<RenderSystem>  render;
	std::shared_ptr<RenderShadows> renderShadows;

	CameraComponent*                  currentCamera;
	DirectionalLightComponent*        directionalLight;
	std::vector<PointLightComponent*> pointLights[2];
	std::vector<GameObject*>          gameObjects;

	void Run();
	virtual void PrepareResources();
	virtual void Initialize();
	virtual void Update();
	virtual void UpdateInternal();
	virtual void Draw();
	virtual void DestroyResources();

	void AddGameObject(GameObject* gameObject);

	std::shared_ptr<DisplayWin32>  GetDisplay();
	std::shared_ptr<InputDevice>   GetInputDevice();
	std::shared_ptr<RenderSystem>  GetRenderSystem();
	std::shared_ptr<RenderShadows> GetRenderShadowsSystem();

	LRESULT MessageHandler(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);