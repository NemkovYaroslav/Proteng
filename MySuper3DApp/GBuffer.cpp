#include "GBuffer.h"
#include <cassert>
#include "Game.h"
#include "DisplayWin32.h"
#include "RenderSystem.h"

GBuffer::GBuffer()
{
	// DIFFUSE
	D3D11_TEXTURE2D_DESC diffuseTexDesc = {};
	ZeroMemory(&diffuseTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	diffuseTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	diffuseTexDesc.MipLevels = 1;
	diffuseTexDesc.ArraySize = 1;
	diffuseTexDesc.SampleDesc.Count = 1;
	diffuseTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	diffuseTexDesc.Width = Game::GetInstance()->GetDisplay()->GetClientWidth();
	diffuseTexDesc.Height = Game::GetInstance()->GetDisplay()->GetClientHeight();
	auto result = Game::GetInstance()->GetRenderSystem()->device->CreateTexture2D(&diffuseTexDesc, nullptr, &diffuseTex);
	assert(SUCCEEDED(result));
	result = Game::GetInstance()->GetRenderSystem()->device->CreateRenderTargetView(diffuseTex, nullptr, &diffuseRTV);
	assert(SUCCEEDED(result));

	D3D11_SHADER_RESOURCE_VIEW_DESC diffuseSRVDesc;
	ZeroMemory(&diffuseSRVDesc, sizeof(ID3D11ShaderResourceView));
	diffuseSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	diffuseSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	diffuseSRVDesc.Texture2D.MostDetailedMip = 0;
	diffuseSRVDesc.Texture2D.MipLevels = 1;
	result = Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(diffuseTex, &diffuseSRVDesc, &diffuseSRV);
	assert(SUCCEEDED(result));

	// NORMAL
	D3D11_TEXTURE2D_DESC normalTexDesc = {};
	ZeroMemory(&diffuseTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	normalTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	normalTexDesc.MipLevels = 1;
	normalTexDesc.ArraySize = 1;
	normalTexDesc.SampleDesc.Count = 1;
	normalTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	normalTexDesc.Width = Game::GetInstance()->GetDisplay()->GetClientWidth();
	normalTexDesc.Height = Game::GetInstance()->GetDisplay()->GetClientHeight();
	result = Game::GetInstance()->GetRenderSystem()->device->CreateTexture2D(&normalTexDesc, nullptr, &normalTex);
	assert(SUCCEEDED(result));
	result = Game::GetInstance()->GetRenderSystem()->device->CreateRenderTargetView(normalTex, nullptr, &normalRTV);
	assert(SUCCEEDED(result));

	D3D11_SHADER_RESOURCE_VIEW_DESC normalSRVDesc;
	ZeroMemory(&normalSRVDesc, sizeof(ID3D11ShaderResourceView));
	normalSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	normalSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	normalSRVDesc.Texture2D.MostDetailedMip = 0;
	normalSRVDesc.Texture2D.MipLevels = 1;
	result = Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(normalTex, &normalSRVDesc, &normalSRV);
	assert(SUCCEEDED(result));

	// WORLD POSITION
	D3D11_TEXTURE2D_DESC worldPositionTexDesc = {};
	ZeroMemory(&diffuseTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	worldPositionTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	worldPositionTexDesc.MipLevels = 1;
	worldPositionTexDesc.ArraySize = 1;
	worldPositionTexDesc.SampleDesc.Count = 1;
	worldPositionTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	worldPositionTexDesc.Width = Game::GetInstance()->GetDisplay()->GetClientWidth();
	worldPositionTexDesc.Height = Game::GetInstance()->GetDisplay()->GetClientHeight();
	result = Game::GetInstance()->GetRenderSystem()->device->CreateTexture2D(&worldPositionTexDesc, nullptr, &worldPositionTex);
	assert(SUCCEEDED(result));
	result = Game::GetInstance()->GetRenderSystem()->device->CreateRenderTargetView(worldPositionTex, nullptr, &worldPositionRTV);
	assert(SUCCEEDED(result));

	D3D11_SHADER_RESOURCE_VIEW_DESC worldPositionSRVDesc;
	ZeroMemory(&worldPositionSRVDesc, sizeof(ID3D11ShaderResourceView));
	worldPositionSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	worldPositionSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	worldPositionSRVDesc.Texture2D.MostDetailedMip = 0;
	worldPositionSRVDesc.Texture2D.MipLevels = 1;
	result = Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(worldPositionTex, &worldPositionSRVDesc, &worldPositionSRV);
	assert(SUCCEEDED(result));
}

