#pragma once
#include "includes.h"
#include "InputDevice.h"

class GameObject;

class Component
{
public:

	GameObject* gameObject;

	virtual ~Component() {}
	virtual void Initialize() {}
	virtual void Update(float deltaTime) {}

	virtual void MouseEventHandler(const InputDevice::MouseMoveEventArgs& mouseData) {}
};