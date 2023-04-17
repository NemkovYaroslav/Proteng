#pragma once
#include "Component.h"

class TransformComponent;

class KatamariControllerComponent : public Component
{
public:

	float katamariSpeed;

	TransformComponent* cameraTransform;

	KatamariControllerComponent();
	virtual void Update(float deltaTime) override;
};

