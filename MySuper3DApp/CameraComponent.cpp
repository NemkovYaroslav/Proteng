#include "CameraComponent.h"
#include "Game.h"
#include "DisplayWin32.h"
#include "CameraControllerComponent.h"

CameraComponent::CameraComponent()
{
	aspect = 1.0f;
	nearZ = 0.1f;
	farZ = 100.0f;
}

void CameraComponent::Initialize()
{
	aspect = Game::GetInstance()->GetDisplay()->GetClientWidth() / Game::GetInstance()->GetDisplay()->GetClientHeight();
}

Matrix CameraComponent::GetProjection()
{
	Matrix rotation = Matrix::CreateFromAxisAngle(Vector3::Up, DirectX::XM_PI);
	return rotation * Matrix::CreatePerspectiveFieldOfView(
		fovAngle,
		aspect,
		nearZ,
		farZ
	);
}