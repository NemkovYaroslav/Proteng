#pragma once
#include "includes.h"
#include "InputDevice.h"
#include "Component.h"

class TransformComponent;

using namespace DirectX::SimpleMath;

class CameraArmControllerComponent : public Component
{
public:

	TransformComponent* aim;
	float yaw;
	float pitch;
	float minPitch;
	float maxPitch;
	float armLength;

	CameraArmControllerComponent();
	virtual void Update(float deltaTime) override;
	virtual void Initialize() override;
	virtual void MouseEventHandler(const InputDevice::MouseMoveEventArgs& mouseData) override;
};
