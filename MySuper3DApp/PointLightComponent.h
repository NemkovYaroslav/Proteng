#pragma once

#include "TransformComponent.h"
#include "Component.h"
#include "ModelComponent.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX;

class PointLightComponent : public Component
{
public:

	PointLightComponent(float constant, float linear, float quadratic);
	PointLightComponent() = delete;

	virtual void Initialize() override;

	void Draw();

	ID3D11Buffer** constBuffer;

	Vector4 lightColor = { Vector3(1.0f, 1.0f, 1.0f) };

	float sphereRadius;

	Microsoft::WRL::ComPtr<ID3D11Buffer> poiVertexBuffer;
	std::vector<Vector4> poiPoints;
	Microsoft::WRL::ComPtr<ID3D11Buffer> poiIndexBuffer;
	std::vector<int> poiIndices;

	void PoiAddMesh(float sphereRadius, std::string objectFileName);
	void ProcessNode(aiNode* node, const aiScene* scene, float sphereRadius);
	void ProcessMesh(aiMesh* mesh, const aiScene* scene, float sphereRadius);
};
