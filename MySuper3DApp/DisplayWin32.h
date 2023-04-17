#pragma once

#include "includes.h"

class DisplayWin32
{
protected:

	LPCWSTR applicationName;
	int clientWidth;
	int clientHeight;

	HINSTANCE hInstance;
	HWND hWnd;

public:

	DisplayWin32(const LPCWSTR& applicationName, int clientWidth, int clientHeight, WNDPROC WndProc);
	~DisplayWin32();

	int GetClientWidth();
	int GetClientHeight();
	HWND GetHWnd();
};