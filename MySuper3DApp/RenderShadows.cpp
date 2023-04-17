#include "RenderShadows.h"

#include "DisplayWin32.h"
#include "RenderComponent.h"
#include "Game.h"
#include "RenderSystem.h"
#include "RenderShadowsComponent.h"

void RenderShadows::InitializeShader(std::string shaderFileName)
{
	std::wstring fileName(shaderFileName.begin(), shaderFileName.end());
	ID3DBlob* errorCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	auto result = D3DCompileFromFile(
		fileName.c_str(),
		nullptr,
		nullptr,
		"VSMain",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		vertexShaderByteCode.GetAddressOf(),
		&errorCode
	);
	if (FAILED(result))
	{
		if (errorCode)
		{
			const char* compileErrors = (char*)(errorCode->GetBufferPointer());
			std::cout << compileErrors << std::endl;
		}
		else
		{
			std::cout << "Missing Shader File: " << std::endl;
		}
		return;
	}
	result = Game::GetInstance()->GetRenderSystem()->device->CreateVertexShader(
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		nullptr, sVertexShader.GetAddressOf()
	);
	assert(SUCCEEDED(result));

	D3D11_INPUT_ELEMENT_DESC inputElements[] = {
		D3D11_INPUT_ELEMENT_DESC {
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			0,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};
	result = Game::GetInstance()->GetRenderSystem()->device->CreateInputLayout(
		inputElements,
		1,
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		sInputLayout.GetAddressOf()
	);
	assert(SUCCEEDED(result));

	Microsoft::WRL::ComPtr<ID3DBlob> geometryShaderByteCode;
	result = D3DCompileFromFile(
		fileName.c_str(),
		nullptr,
		nullptr,
		"GSMain",
		"gs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		geometryShaderByteCode.GetAddressOf(),
		&errorCode
	);
	assert(SUCCEEDED(result));
	result = Game::GetInstance()->GetRenderSystem()->device->CreateGeometryShader(
		geometryShaderByteCode->GetBufferPointer(),
		geometryShaderByteCode->GetBufferSize(),
		nullptr, sGeometryShader.GetAddressOf()
	);

	CD3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_BACK;
	rastDesc.FillMode = D3D11_FILL_SOLID;
	result = Game::GetInstance()->GetRenderSystem()->device->CreateRasterizerState(&rastDesc, sRastState.GetAddressOf());
	assert(SUCCEEDED(result));
}


RenderShadows::RenderShadows()
{
	//SHADER
	InitializeShader("../Shaders/DepthShader.hlsl");

	D3D11_SAMPLER_DESC sComparisonSamplerDesc;
	ZeroMemory(&sComparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
	sComparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sComparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sComparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sComparisonSamplerDesc.BorderColor[0] = 1.0f;
	sComparisonSamplerDesc.BorderColor[1] = 1.0f;
	sComparisonSamplerDesc.BorderColor[2] = 1.0f;
	sComparisonSamplerDesc.BorderColor[3] = 1.0f;
	sComparisonSamplerDesc.MinLOD = 0.f;
	sComparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	sComparisonSamplerDesc.MipLODBias = 0.f;
	sComparisonSamplerDesc.MaxAnisotropy = 0;
	sComparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;        //
	sComparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; //
	auto result = Game::GetInstance()->GetRenderSystem()->device->CreateSamplerState(&sComparisonSamplerDesc, sSamplerState.GetAddressOf());
	assert(SUCCEEDED(result));
}

void RenderShadows::PrepareFrame()
{
	Game::GetInstance()->GetRenderSystem()->context->RSSetState(sRastState.Get());
	Game::GetInstance()->GetRenderSystem()->context->OMSetRenderTargets(0, nullptr, Game::GetInstance()->directionalLight->depthStencilView.Get());
	Game::GetInstance()->GetRenderSystem()->context->RSSetViewports(1, Game::GetInstance()->directionalLight->viewport.get());
	Game::GetInstance()->GetRenderSystem()->context->ClearDepthStencilView(Game::GetInstance()->directionalLight->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void RenderShadows::Draw()
{
	for (auto& renderShadowsComponent : renderShadowsComponents)
	{
		renderShadowsComponent->Draw();
	}
}

void RenderShadows::EndFrame()
{
	Game::GetInstance()->GetRenderSystem()->context->OMSetRenderTargets(0, nullptr, nullptr);
}