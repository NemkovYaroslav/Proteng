#include "CameraControllerComponent.h"
#include "CameraComponent.h"
#include <algorithm>
#include "GameObject.h"

CameraArmControllerComponent::CameraArmControllerComponent()
{
	this->aim = nullptr;
	this->yaw = 0;
	this->pitch = 0;
	this->minPitch = 0.0 / 180.0 * DirectX::XM_PI;
	this->maxPitch = 90.0 / 180.0 * DirectX::XM_PI;
	this->armLength = 5.0f;
}

void CameraArmControllerComponent::Initialize()
{
	Game::GetInstance()->GetInputDevice()->MouseMove.AddRaw(this, &CameraArmControllerComponent::MouseEventHandler);
	this->pitch = minPitch;
}

void CameraArmControllerComponent::Update(float deltaTime)
{
	if (Game::GetInstance()->GetInputDevice()->IsKeyDown(Keys::E))
	{
		std::cout << " X: " << gameObject->transformComponent->GetLeft().x << " Y: " << gameObject->transformComponent->GetLeft().y << " Z: " << gameObject->transformComponent->GetLeft().z << std::endl;
	}
	Vector3 arm = Vector3::Transform(Vector3(0,0, - armLength), Matrix::CreateFromYawPitchRoll(yaw, pitch, 0));
	gameObject->transformComponent->SetPosition(aim->GetPosition() + arm);
	gameObject->transformComponent->SetRotation(Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0));
}

void CameraArmControllerComponent::MouseEventHandler(const InputDevice::MouseMoveEventArgs& mouseData)
{
	yaw   -= mouseData.Offset.x * 0.003f;
	pitch += mouseData.Offset.y * 0.003f;
	if (pitch > maxPitch) { pitch = maxPitch; }
	if (pitch < minPitch) { pitch = minPitch; }
}