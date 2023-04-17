#include "KatamariControllerComponent.h"
#include "GameObject.h"
#include "TransformComponent.h"

KatamariControllerComponent::KatamariControllerComponent()
{
	this->katamariSpeed = 0.0f;
	this->cameraTransform = nullptr;
}

void KatamariControllerComponent::Update(float deltaTime)
{
	Vector3 left = cameraTransform->GetLeft();
	Vector3 forward = left.Cross(Vector3::UnitY);
	if (Game::GetInstance()->GetInputDevice()->IsKeyDown(Keys::A))
	{
		gameObject->transformComponent->SetRotation(gameObject->transformComponent->GetRotation() *
			Quaternion::CreateFromAxisAngle(forward, - katamariSpeed * deltaTime / 1.0f));
		gameObject->transformComponent->SetPosition(gameObject->transformComponent->GetPosition() + katamariSpeed * deltaTime * left);
	}
	if (Game::GetInstance()->GetInputDevice()->IsKeyDown(Keys::D))
	{
		gameObject->transformComponent->SetRotation(gameObject->transformComponent->GetRotation() *
			Quaternion::CreateFromAxisAngle(forward, katamariSpeed * deltaTime / 1.0f));
		gameObject->transformComponent->SetPosition(gameObject->transformComponent->GetPosition() - katamariSpeed * deltaTime * left);
	}
	if (Game::GetInstance()->GetInputDevice()->IsKeyDown(Keys::W))
	{
		gameObject->transformComponent->SetRotation(gameObject->transformComponent->GetRotation() *
			Quaternion::CreateFromAxisAngle(left, katamariSpeed * deltaTime / 1.0f));
		gameObject->transformComponent->SetPosition(gameObject->transformComponent->GetPosition() + katamariSpeed * deltaTime * forward);
	}
	if (Game::GetInstance()->GetInputDevice()->IsKeyDown(Keys::S))
	{
		gameObject->transformComponent->SetRotation(gameObject->transformComponent->GetRotation() *
			Quaternion::CreateFromAxisAngle(left, - katamariSpeed * deltaTime / 1.0f));
		gameObject->transformComponent->SetPosition(gameObject->transformComponent->GetPosition() - katamariSpeed * deltaTime * forward);
	}
}
