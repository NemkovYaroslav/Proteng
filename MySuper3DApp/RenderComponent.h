#pragma once
#include "Component.h"

#include "ModelComponent.h"

#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX::SimpleMath;

class RenderComponent : public Component
{
public:

    RenderComponent(ModelComponent* modelComponent);
    RenderComponent() = delete;

    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;

    //void Draw();

    void DrawOpaque();

    ModelComponent* modelComponent;

    ID3D11Buffer** constBuffer;
};
