#include "RenderShadowsComponent.h"
#include "Game.h"
#include "RenderSystem.h"
#include "RenderShadows.h"
#include "DirectionalLightComponent.h"
#include "GameObject.h"
#include "CameraComponent.h"


struct alignas(16) CascadeData
{
	Matrix model;
	Matrix viewProjMats[4]; //
};

RenderShadowsComponent::RenderShadowsComponent(ModelComponent* modelComponent)
{
	this->modelComponent = modelComponent;
}

void RenderShadowsComponent::Initialize()
{
	Game::GetInstance()->GetRenderShadowsSystem()->renderShadowsComponents.push_back(this);

	D3D11_BUFFER_DESC constBufferDesc = {};
	constBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constBufferDesc.MiscFlags = 0;
	constBufferDesc.StructureByteStride = 0;
	constBufferDesc.ByteWidth = sizeof(CascadeData);
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&constBufferDesc, nullptr, sConstBuffer.GetAddressOf());	
}

void RenderShadowsComponent::Update(float deltaTime)
{

}

void RenderShadowsComponent::Draw()
{
	Game::GetInstance()->directionalLight->lightViewProjectionMatrices = Game::GetInstance()->directionalLight->GetLightSpaceMatrices();
	const CascadeData cascadeData
	{
		gameObject->transformComponent->GetModel(),
		{ 
			Game::GetInstance()->directionalLight->lightViewProjectionMatrices.at(0), Game::GetInstance()->directionalLight->lightViewProjectionMatrices.at(1),
		    Game::GetInstance()->directionalLight->lightViewProjectionMatrices.at(2), Game::GetInstance()->directionalLight->lightViewProjectionMatrices.at(3)
		} //
	};
	D3D11_MAPPED_SUBRESOURCE firstMappedResource;
	Game::GetInstance()->GetRenderSystem()->context->Map(sConstBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &firstMappedResource);
	memcpy(firstMappedResource.pData, &cascadeData, sizeof(CascadeData));
	Game::GetInstance()->GetRenderSystem()->context->Unmap(sConstBuffer.Get(), 0);


	Game::GetInstance()->GetRenderSystem()->context->RSSetState(Game::GetInstance()->GetRenderShadowsSystem()->sRastState.Get());	
	Game::GetInstance()->GetRenderSystem()->context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT strides[] = { 48 };
	UINT offsets[] = { 0 };
	Game::GetInstance()->GetRenderSystem()->context->IASetVertexBuffers(0, 1, modelComponent->vertexBuffer.GetAddressOf(), strides, offsets); //
	Game::GetInstance()->GetRenderSystem()->context->IASetInputLayout(Game::GetInstance()->GetRenderShadowsSystem()->sInputLayout.Get());
	Game::GetInstance()->GetRenderSystem()->context->IASetIndexBuffer(modelComponent->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	Game::GetInstance()->GetRenderSystem()->context->VSSetShader(Game::GetInstance()->GetRenderShadowsSystem()->sVertexShader.Get(), nullptr, 0);
	Game::GetInstance()->GetRenderSystem()->context->PSSetShader(nullptr, nullptr, 0);
	Game::GetInstance()->GetRenderSystem()->context->GSSetShader(Game::GetInstance()->GetRenderShadowsSystem()->sGeometryShader.Get(), nullptr, 0);

	Game::GetInstance()->GetRenderSystem()->context->VSSetConstantBuffers(0, 1, sConstBuffer.GetAddressOf());
	Game::GetInstance()->GetRenderSystem()->context->GSSetConstantBuffers(0, 1, sConstBuffer.GetAddressOf());

	Game::GetInstance()->GetRenderSystem()->context->DrawIndexed(modelComponent->indices.size(), 0, 0);
}