#pragma once
#include "Component.h"

using namespace DirectX::SimpleMath;

class TransformComponent : public Component
{
public:

    Vector3 localPosition;
    Quaternion localRotation;
    TransformComponent* parent;

    Vector3 scale;

    TransformComponent();
    TransformComponent(Vector3 position, Quaternion rotation);

    void Initialize();
    void Update();

    Vector3 GetPosition() const;
    Quaternion GetRotation() const;
    void SetPosition(const Vector3& position);
    void SetRotation(const Quaternion& rotation);
    Matrix GetView() const;
    Matrix GetLocalView() const;
    Matrix GetModel() const;
    Matrix GetLocalModel() const;
    
    Vector3 GetLocalLeft() const;
    Vector3 GetLocalUp() const;
    Vector3 GetLocalForward() const;
    Vector3 GetLeft() const;
    Vector3 GetUp() const;
    Vector3 GetForward() const;
};