#pragma once
#include "includes.h"
#include "TransformComponent.h"
#include "Component.h"

using namespace DirectX::SimpleMath;

class CameraComponent : public Component
{
public:

	CameraComponent();

	virtual void Initialize() override;

	Matrix GetProjection();
	float fovAngle = DirectX::XM_PIDIV2;
	float aspect;
	float nearZ;
	float farZ;
};