#include "TransformComponent.h"

TransformComponent::TransformComponent() : Component()
{
	localPosition = Vector3::Zero;
	localRotation = Quaternion::Identity;
	scale = Vector3(1, 1, 1);
	parent = nullptr;
}

TransformComponent::TransformComponent(Vector3 position, Quaternion rotation)
{
	localPosition = position;
	localRotation = rotation;
	parent = nullptr;
}

void TransformComponent::Initialize()
{

}

void TransformComponent::Update()
{

}

Vector3 TransformComponent::GetPosition() const
{
	if (parent)
	{
		return Vector3::Transform(localPosition, parent->GetModel());
	}
	else
	{
		return localPosition;
	}
}

Quaternion TransformComponent::GetRotation() const
{
	if (parent)
	{
		return parent->GetRotation() * localRotation;
	}
	else
	{
		return localRotation;
	}
}

void TransformComponent::SetPosition(const Vector3& position)
{
	if (parent)
	{
		localPosition = Vector3::Transform(position, parent->GetView());
	}
	else
	{
		localPosition = position;
	}
}

void TransformComponent::SetRotation(const Quaternion& rotation)
{
	if (parent)
	{
		Quaternion quaternion;
		parent->GetRotation().Inverse(quaternion);
		localRotation = rotation * quaternion;
	}
	else
	{
		localRotation = rotation;
	}
}

Matrix TransformComponent::GetView() const
{
	if (parent)
	{
		return parent->GetView() * GetLocalView();
	}
	else
	{
		return GetLocalView();
	}
}

Matrix TransformComponent::GetLocalView() const
{
	return Matrix::CreateTranslation( - localPosition) *
		Matrix::CreateFromQuaternion(localRotation).Transpose() * Matrix::CreateScale(Vector3(1 / scale.x, 1 / scale.y, 1 / scale.z));
}

Matrix TransformComponent::GetModel() const
{
	if (parent)
	{
		return GetLocalModel() * parent->GetModel();
	}
	else
	{
		return GetLocalModel();
	}
}

Matrix TransformComponent::GetLocalModel() const
{
	return Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(localRotation) * Matrix::CreateTranslation(localPosition);
}

Vector3 TransformComponent::GetLocalLeft() const
{
	return Vector3::TransformNormal(Vector3::Right, GetLocalModel());
}

Vector3 TransformComponent::GetLocalUp() const
{
	return Vector3::TransformNormal(Vector3::Up, GetLocalModel());
}

Vector3 TransformComponent::GetLocalForward() const
{
	return Vector3::TransformNormal(Vector3::Forward, GetLocalModel());
}

Vector3 TransformComponent::GetLeft() const
{
	return Vector3::TransformNormal(Vector3::Right, GetModel());
}

Vector3 TransformComponent::GetUp() const
{
	return Vector3::TransformNormal(Vector3::Up, GetModel());
}

Vector3 TransformComponent::GetForward() const
{
	return Vector3::TransformNormal(Vector3::Forward, GetModel());
}
