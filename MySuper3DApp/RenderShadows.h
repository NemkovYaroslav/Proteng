#pragma once

#include "DirectionalLightComponent.h"

class RenderShadowsComponent;

class RenderShadows
{
public:

	RenderShadows();

	void PrepareFrame();
	void Draw();
	void EndFrame();

	void InitializeShader(std::string shaderFileName);

	Microsoft::WRL::ComPtr<ID3D11InputLayout>  sInputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> sVertexShader;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> sGeometryShader;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sSamplerState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> sRastState;

	std::vector<RenderShadowsComponent*> renderShadowsComponents;
};