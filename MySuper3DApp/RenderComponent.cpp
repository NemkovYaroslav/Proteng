#include "RenderComponent.h"

#include <WICTextureLoader.h>
#include "Game.h"
#include "DisplayWin32.h"
#include "CameraComponent.h"
#include "RenderSystem.h"
#include "GameObject.h"
#include "RenderShadows.h"
#include "PointLightComponent.h"
#include "GBuffer.h"

RenderComponent::RenderComponent(ModelComponent* modelComponent)
{
	this->modelComponent = modelComponent;
}

struct alignas(16) CameraData
{
	Matrix camView;
	Matrix camProjection;
	Matrix objModel;
};

struct MaterialData
{
	Vector4 matAmbient;
	Vector4 matDiffuse;
	Vector4 matSpecular;
};
struct DirectionalLightData
{
	Vector4 dirLightColor;
	Vector4 dirDirection;
};
struct PointLightData
{
	Vector4 poiLightColor;
	Vector4 poiConstLinearQuadCount;
	Vector4 poiPosition;
};
struct alignas(16) LightData
{
	MaterialData         MatData;
	DirectionalLightData RemLight;
	PointLightData       PoiLight;
};

struct alignas(16) ShadowData
{
	Matrix viewProjMats[4]; //
	float distances[4];     //
};

void RenderComponent::Initialize()
{
	Game::GetInstance()->GetRenderSystem()->renderComponents.push_back(this);

	constBuffer = new ID3D11Buffer * [3];

	D3D11_BUFFER_DESC firstConstBufferDesc = {};
	firstConstBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	firstConstBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	firstConstBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	firstConstBufferDesc.MiscFlags = 0;
	firstConstBufferDesc.StructureByteStride = 0;
	firstConstBufferDesc.ByteWidth = sizeof(CameraData);
	auto result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&firstConstBufferDesc, nullptr, &constBuffer[0]);
	assert(SUCCEEDED(result));

	D3D11_BUFFER_DESC secondConstBufferDesc = {};
	secondConstBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	secondConstBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	secondConstBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	secondConstBufferDesc.MiscFlags = 0;
	secondConstBufferDesc.StructureByteStride = 0;
	secondConstBufferDesc.ByteWidth = sizeof(LightData);
	result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&secondConstBufferDesc, nullptr, &constBuffer[1]);
	assert(SUCCEEDED(result));

	D3D11_BUFFER_DESC thirdConstBufferDesc = {};
	thirdConstBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	thirdConstBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	thirdConstBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	thirdConstBufferDesc.MiscFlags = 0;
	thirdConstBufferDesc.StructureByteStride = 0;
	thirdConstBufferDesc.ByteWidth = sizeof(ShadowData);
	result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&thirdConstBufferDesc, nullptr, &constBuffer[2]);
	assert(SUCCEEDED(result));
}

void RenderComponent::Update(float deltaTime) {}

/*
void RenderComponent::Draw()
{
	const CameraData cameraData
	{
		Game::GetInstance()->currentCamera->gameObject->transformComponent->GetView(),
		Game::GetInstance()->currentCamera->GetProjection(),
		gameObject->transformComponent->GetModel(),
		Game::GetInstance()->currentCamera->gameObject->transformComponent->GetPosition()
	};
	D3D11_MAPPED_SUBRESOURCE firstMappedResource;
	Game::GetInstance()->GetRenderSystem()->context->Map(constBuffer[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &firstMappedResource);
	memcpy(firstMappedResource.pData, &cameraData, sizeof(CameraData));
	Game::GetInstance()->GetRenderSystem()->context->Unmap(constBuffer[0], 0);

	LightData lightData {};
	// MATERIAL
	lightData.MatData.matAmbient  = modelComponent->material.ambient;
	lightData.MatData.matDiffuse  = modelComponent->material.diffuse;
	lightData.MatData.matSpecular = modelComponent->material.specular;
	// DIRECTIONAL LIGHT
	lightData.RemLight.dirLightColor = Game::GetInstance()->directionalLight->lightColor;
	lightData.RemLight.dirDirection  = Game::GetInstance()->directionalLight->direction;
	// POINT LIGHTS
	for (int i = 0; i < Game::GetInstance()->pointLights->size(); i++)
	{
		lightData.PoiLight[i].poiLightColor           = Game::GetInstance()->pointLights->at(i)->lightColor;
		lightData.PoiLight[i].poiConstLinearQuadCount = Vector4(1.0f, 0.09f, 0.032f, 2.0f);
		lightData.PoiLight[i].poiPosition             = Vector4(Game::GetInstance()->pointLights->at(i)->gameObject->transformComponent->GetPosition());
	}
	D3D11_MAPPED_SUBRESOURCE secondMappedResource;
	Game::GetInstance()->GetRenderSystem()->context->Map(constBuffer[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &secondMappedResource);
	memcpy(secondMappedResource.pData, &lightData, sizeof(LightData));
	Game::GetInstance()->GetRenderSystem()->context->Unmap(constBuffer[1], 0);

	const ShadowData lightShadowData
	{
		{
			Game::GetInstance()->directionalLight->lightViewProjectionMatrices.at(0), Game::GetInstance()->directionalLight->lightViewProjectionMatrices.at(1),
			Game::GetInstance()->directionalLight->lightViewProjectionMatrices.at(2), Game::GetInstance()->directionalLight->lightViewProjectionMatrices.at(3)
		}, //
		{
			Game::GetInstance()->directionalLight->shadowCascadeLevels.at(0),         Game::GetInstance()->directionalLight->shadowCascadeLevels.at(1),
			Game::GetInstance()->directionalLight->shadowCascadeLevels.at(2),         Game::GetInstance()->directionalLight->shadowCascadeLevels.at(3)
		} //
	};
	D3D11_MAPPED_SUBRESOURCE thirdMappedResource;
	Game::GetInstance()->GetRenderSystem()->context->Map(constBuffer[2], 0, D3D11_MAP_WRITE_DISCARD, 0, &thirdMappedResource);
	memcpy(thirdMappedResource.pData, &lightShadowData, sizeof(ShadowData));
	Game::GetInstance()->GetRenderSystem()->context->Unmap(constBuffer[2], 0);

	Game::GetInstance()->GetRenderSystem()->context->PSSetShaderResources(0, 1, modelComponent->textureView.GetAddressOf());
	Game::GetInstance()->GetRenderSystem()->context->PSSetSamplers(0, 1, Game::GetInstance()->GetRenderSystem()->samplerState.GetAddressOf());

	Game::GetInstance()->GetRenderSystem()->context->PSSetShaderResources(1, 1, Game::GetInstance()->directionalLight->textureResourceView.GetAddressOf());
	Game::GetInstance()->GetRenderSystem()->context->PSSetSamplers(1, 1, Game::GetInstance()->GetRenderShadowsSystem()->sSamplerState.GetAddressOf());

	Game::GetInstance()->GetRenderSystem()->context->RSSetState(Game::GetInstance()->GetRenderSystem()->rastState.Get());
	Game::GetInstance()->GetRenderSystem()->context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	UINT strides[] { 48 };
	UINT offsets[] { 0 };
	Game::GetInstance()->GetRenderSystem()->context->IASetVertexBuffers(0, 1, modelComponent->vertexBuffer.GetAddressOf(), strides, offsets);
	Game::GetInstance()->GetRenderSystem()->context->IASetInputLayout(Game::GetInstance()->GetRenderSystem()->inputLayout.Get());
	Game::GetInstance()->GetRenderSystem()->context->IASetIndexBuffer(modelComponent->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	Game::GetInstance()->GetRenderSystem()->context->VSSetShader(Game::GetInstance()->GetRenderSystem()->vertexShader.Get(), nullptr, 0);
	Game::GetInstance()->GetRenderSystem()->context->PSSetShader(Game::GetInstance()->GetRenderSystem()->pixelShader.Get(), nullptr, 0);
	Game::GetInstance()->GetRenderSystem()->context->GSSetShader(nullptr, nullptr, 0);
	
	Game::GetInstance()->GetRenderSystem()->context->VSSetConstantBuffers(0, 3, constBuffer);
	Game::GetInstance()->GetRenderSystem()->context->PSSetConstantBuffers(0, 3, constBuffer);
	
	Game::GetInstance()->GetRenderSystem()->context->DrawIndexed(modelComponent->indices.size(), 0, 0);
}
*/

void RenderComponent::DrawOpaque()
{
	const CameraData cameraData
	{
		Game::GetInstance()->currentCamera->gameObject->transformComponent->GetView(),
		Game::GetInstance()->currentCamera->GetProjection(),
		gameObject->transformComponent->GetModel()
	};
	D3D11_MAPPED_SUBRESOURCE firstMappedResource;
	Game::GetInstance()->GetRenderSystem()->context->Map(constBuffer[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &firstMappedResource);
	memcpy(firstMappedResource.pData, &cameraData, sizeof(CameraData));
	Game::GetInstance()->GetRenderSystem()->context->Unmap(constBuffer[0], 0);

	Game::GetInstance()->GetRenderSystem()->context->OMSetBlendState(Game::GetInstance()->GetRenderSystem()->blendStateOpaque, nullptr, 0xffffffff); //-//

	Game::GetInstance()->GetRenderSystem()->context->OMSetDepthStencilState(Game::GetInstance()->GetRenderSystem()->dsOpaque, 0); //-//

	Game::GetInstance()->GetRenderSystem()->context->PSSetShaderResources(0, 1, modelComponent->textureView.GetAddressOf()); 
	Game::GetInstance()->GetRenderSystem()->context->PSSetSamplers(0, 1, Game::GetInstance()->GetRenderSystem()->samplerState.GetAddressOf()); //-//

	Game::GetInstance()->GetRenderSystem()->context->RSSetState(Game::GetInstance()->GetRenderSystem()->rastCullFront); //-//
	Game::GetInstance()->GetRenderSystem()->context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //-//

	UINT strides[]{ 48 }; //-//
	UINT offsets[]{ 0 };  //-//
	Game::GetInstance()->GetRenderSystem()->context->IASetVertexBuffers(0, 1, modelComponent->vertexBuffer.GetAddressOf(), strides, offsets); //-//
	Game::GetInstance()->GetRenderSystem()->context->IASetInputLayout(Game::GetInstance()->GetRenderSystem()->layoutOpaque); //-//
	Game::GetInstance()->GetRenderSystem()->context->IASetIndexBuffer(modelComponent->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0); //-//

	Game::GetInstance()->GetRenderSystem()->context->VSSetShader(Game::GetInstance()->GetRenderSystem()->vsOpaque, nullptr, 0); //-//
	Game::GetInstance()->GetRenderSystem()->context->PSSetShader(Game::GetInstance()->GetRenderSystem()->psOpaque, nullptr, 0); //-//
	//Game::GetInstance()->GetRenderSystem()->context->GSSetShader(nullptr, nullptr, 0);

	Game::GetInstance()->GetRenderSystem()->context->VSSetConstantBuffers(0, 1, &constBuffer[0]); //-//
	//Game::GetInstance()->GetRenderSystem()->context->PSSetConstantBuffers(0, 1, &constBuffer[0]); //-//

	Game::GetInstance()->GetRenderSystem()->context->DrawIndexed(modelComponent->indices.size(), 0, 0); //-//
}