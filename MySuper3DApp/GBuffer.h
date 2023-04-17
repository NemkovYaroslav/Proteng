#pragma once
#include <d3d11.h>
#include "includes.h"

class GBuffer
{
public:

	ID3D11Texture2D* diffuseTex = nullptr;
	ID3D11Texture2D* normalTex = nullptr;
	ID3D11Texture2D* worldPositionTex = nullptr;

	ID3D11ShaderResourceView* diffuseSRV = nullptr;
	ID3D11ShaderResourceView* normalSRV = nullptr;
	ID3D11ShaderResourceView* worldPositionSRV = nullptr;

	ID3D11RenderTargetView* diffuseRTV = nullptr;
	ID3D11RenderTargetView* normalRTV = nullptr;
	ID3D11RenderTargetView* worldPositionRTV = nullptr;

	GBuffer();
};

