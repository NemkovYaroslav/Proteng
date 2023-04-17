#include "Game.h"

#include "DisplayWin32.h"
#include "InputDevice.h"
#include "RenderSystem.h"
#include "RenderShadows.h"
#include "GameObject.h"
#include "CameraComponent.h"
#include "DirectionalLightComponent.h"
#include "PointLightComponent.h"
#include "GBuffer.h"

Game* Game::instance = nullptr;

Game::Game(LPCWSTR name, int clientWidth, int clientHeight)
{
	this->name = name;
	this->clientWidth = clientWidth;
	this->clientHeight = clientHeight;
	totalTime = 0;
	deltaTime = 0;
	frameCount = 0;
	startTime = std::make_shared<std::chrono::time_point<std::chrono::steady_clock>>();
	prevTime = std::make_shared<std::chrono::time_point<std::chrono::steady_clock>>();
	currentCamera = nullptr;
	directionalLight = nullptr;
}
Game* Game::CreateInstance(LPCWSTR name, int screenWidth, int screenHeight)
{
	if (!instance)
	{
		instance = new Game(name, screenWidth, screenHeight);
	}
	return instance;
}
Game* Game::GetInstance()
{
	return instance;
}

void Game::PrepareResources()
{
	display = std::make_shared<DisplayWin32>(name, clientWidth, clientHeight, WndProc);
	inputDevice = std::make_shared<InputDevice>();
	render = std::make_shared<RenderSystem>();
	render->gBuffer = new GBuffer(); ///
	renderShadows = std::make_shared<RenderShadows>();
}

void Game::Initialize()
{
	for (auto& object : gameObjects) {
		object->Initialize();
	}
}

void Game::Run()
{
	PrepareResources();
	Initialize();

	*startTime = std::chrono::steady_clock::now();
	*prevTime = *startTime;

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			auto curTime = std::chrono::steady_clock::now();
			deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(curTime - *prevTime).count() / 1000000.0f;
			*prevTime = curTime;
			totalTime += deltaTime;
			frameCount++;
			if (totalTime > 1.0f)
			{
				float fps = frameCount / totalTime;
				totalTime -= 1.0f;
				WCHAR text[256];
				swprintf_s(text, TEXT("FPS: %f"), fps);
				SetWindowText(Game::GetInstance()->GetDisplay()->GetHWnd(), text);
				frameCount = 0;
			}
			Update();
			Draw();
		}
	}
	DestroyResources();
}

void Game::UpdateInternal()
{
	if (inputDevice->IsKeyDown(Keys::Escape))
	{
		PostQuitMessage(0);
	}
}

void Game::Update()
{
	UpdateInternal();
	for (auto& object : gameObjects)
	{
		object->Update(deltaTime);
	}
}

void Game::Draw()
{
	Game::GetInstance()->GetRenderSystem()->context->ClearState();
	renderShadows->PrepareFrame();
	renderShadows->Draw();
	renderShadows->EndFrame();
	render->PrepareFrame();
	render->Draw();
	render->EndFrame();
}

void Game::DestroyResources()
{
	for (auto& object : gameObjects)
	{
		delete object;
	}
	gameObjects.clear();
}

void Game::AddGameObject(GameObject* gameObject)
{
	gameObjects.push_back(gameObject);
}

std::shared_ptr<DisplayWin32>  Game::GetDisplay()             { return display;       }
std::shared_ptr<InputDevice>   Game::GetInputDevice()         { return inputDevice;   }
std::shared_ptr<RenderSystem>  Game::GetRenderSystem()        { return render; }
std::shared_ptr<RenderShadows> Game::GetRenderShadowsSystem() { return renderShadows; }

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	return Game::GetInstance()->MessageHandler(hwnd, umessage, wparam, lparam);
}
LRESULT Game::MessageHandler(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	{
		switch (umessage)
		{
			case WM_CLOSE:
			{
				PostQuitMessage(0);
				return 0;
			}
			case WM_INPUT:
			{
				UINT dwSize = 0;
				GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
				LPBYTE lpb = new BYTE[dwSize];

				if (lpb == nullptr)
				{
					return 0;
				}

				if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
				{
					OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));
				}

				RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb);

				if (raw->header.dwType == RIM_TYPEKEYBOARD)
				{
					Game::GetInstance()->GetInputDevice()->OnKeyDown({
						raw->data.keyboard.MakeCode,
						raw->data.keyboard.Flags,
						raw->data.keyboard.VKey,
						raw->data.keyboard.Message
						});
				}
				else if (raw->header.dwType == RIM_TYPEMOUSE)
				{
					Game::GetInstance()->GetInputDevice()->OnMouseMove({
						raw->data.mouse.usFlags,
						raw->data.mouse.usButtonFlags,
						static_cast<int>(raw->data.mouse.ulExtraInformation),
						static_cast<int>(raw->data.mouse.ulRawButtons),
						static_cast<short>(raw->data.mouse.usButtonData),
						raw->data.mouse.lLastX,
						raw->data.mouse.lLastY
						});
				}

				delete[] lpb;
				return DefWindowProc(hwnd, umessage, wparam, lparam);
		}
		default:
		{
			return DefWindowProc(hwnd, umessage, wparam, lparam);
		}
		}
	}
}