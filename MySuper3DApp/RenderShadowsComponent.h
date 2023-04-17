#pragma once
#include "Component.h"
#include "ModelComponent.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX::SimpleMath;

class RenderShadowsComponent : public Component
{
public:

    RenderShadowsComponent(ModelComponent* modelComponent);
    RenderShadowsComponent() = delete;

    ModelComponent* modelComponent;

    Microsoft::WRL::ComPtr<ID3D11Buffer> sConstBuffer;

    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;

    void Draw();
};

