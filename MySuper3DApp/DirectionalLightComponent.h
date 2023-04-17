#pragma once

#include "TransformComponent.h"
#include "Component.h"
#include "CameraComponent.h"

using namespace DirectX;

class DirectionalLightComponent : public Component
{
public:

	DirectionalLightComponent(int shadowMapSize, float viewWidth, float viewHeight, float nearZ, float farZ);
	DirectionalLightComponent() = delete;
	
	virtual void Initialize() override;

	void Draw();

	ID3D11Buffer** constBuffer;

	Vector4 lightColor = { Vector3(0.5f, 0.5f, 0.5f) };

	Vector4 direction { Vector3(0.0f, -1.0f, 0.1f) };
	int shadowMapSize;
	float viewWidth;
	float viewHeight;
	float nearZ;
	float farZ;

	std::vector<Matrix> lightViewProjectionMatrices;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowMapTexture2D;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureResourceView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	std::shared_ptr<D3D11_VIEWPORT> viewport;

	Matrix GetViewMatrix();
	Matrix GetProjectionMatrix();

	std::vector<float> shadowCascadeLevels //
	{
		Game::GetInstance()->currentCamera->farZ / 20,
		Game::GetInstance()->currentCamera->farZ / 10,
		Game::GetInstance()->currentCamera->farZ / 5,
		Game::GetInstance()->currentCamera->farZ
	};
	std::vector<Vector4> GetFrustumCornerWorldSpace(const Matrix& projection, const Matrix& view); //
	Matrix GetLightSpaceMatrix(const float nearZ, const float farZ); //
	std::vector<Matrix> GetLightSpaceMatrices(); //
};