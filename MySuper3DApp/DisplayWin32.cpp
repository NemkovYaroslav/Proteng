#include "DisplayWin32.h"

#include "Game.h"

DisplayWin32::DisplayWin32(const LPCWSTR& applicationName, int clientWidth, int clientHeight, WNDPROC WndProc)
{
	this->applicationName = applicationName;
	this->clientWidth = clientWidth;
	this->clientHeight = clientHeight;

	hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = applicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	RegisterClassEx(&wc);

	RECT windowRect;
	windowRect = {
		0,
		0,
		static_cast<LONG>(clientWidth),
		static_cast<LONG>(clientHeight)
	};

	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	auto dwStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME;

	auto posX = (GetSystemMetrics(SM_CXSCREEN) - clientWidth) / 2;
	auto posY = (GetSystemMetrics(SM_CYSCREEN) - clientHeight) / 2;

	hWnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		applicationName,
		applicationName,
		dwStyle,
		posX, posY,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	ShowCursor(true);
}

DisplayWin32::~DisplayWin32()
{
	UnregisterClass(applicationName, hInstance);
	DestroyWindow(hWnd);
}

HWND DisplayWin32::GetHWnd()        { return hWnd; }
int DisplayWin32::GetClientWidth()  { return clientWidth; }
int DisplayWin32::GetClientHeight() { return clientHeight;}