#include "RenderSystem.h"

#include "DisplayWin32.h"
#include "RenderComponent.h"
#include "Game.h"
#include "RenderShadows.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "PointLightComponent.h"

RenderSystem::RenderSystem()
{
	//FRAME
	D3D_FEATURE_LEVEL featureLevel[] = { D3D_FEATURE_LEVEL_11_1 }; //ok
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Width = Game::GetInstance()->GetDisplay()->GetClientWidth();
	swapChainDesc.BufferDesc.Height = Game::GetInstance()->GetDisplay()->GetClientHeight();
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = Game::GetInstance()->GetDisplay()->GetHWnd();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	auto result = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_DEBUG,
		featureLevel,
		1,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		swapChain.GetAddressOf(),
		device.GetAddressOf(),
		nullptr,
		context.GetAddressOf()
	);
	assert(SUCCEEDED(result));
	result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	assert(SUCCEEDED(result));
	result = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderView);
	assert(SUCCEEDED(result));

	D3D11_TEXTURE2D_DESC depthTexDesc = {}; //ok
	depthTexDesc.ArraySize = 1;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.MiscFlags = 0;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.Width = Game::GetInstance()->GetDisplay()->GetClientWidth();
	depthTexDesc.Height = Game::GetInstance()->GetDisplay()->GetClientHeight();
	depthTexDesc.SampleDesc = { 1, 0 };
	result = device->CreateTexture2D(&depthTexDesc, nullptr, depthBuffer.GetAddressOf());
	assert(SUCCEEDED(result));
	result = device->CreateDepthStencilView(depthBuffer.Get(), nullptr, &depthView);
	assert(SUCCEEDED(result));

	D3D11_BLEND_DESC BlendStateOpaqueDesc; /// 
	ZeroMemory(&BlendStateOpaqueDesc, sizeof(D3D11_BLEND_DESC));
	BlendStateOpaqueDesc.RenderTarget[0].BlendEnable           = FALSE;
	BlendStateOpaqueDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	BlendStateOpaqueDesc.IndependentBlendEnable                = false;
	device->CreateBlendState(&BlendStateOpaqueDesc, &blendStateOpaque);

	D3D11_BLEND_DESC BlendStateLightDesc; ///
	ZeroMemory(&BlendStateLightDesc, sizeof(D3D11_BLEND_DESC));
	BlendStateLightDesc.RenderTarget[0].BlendEnable           = TRUE;
	BlendStateLightDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	BlendStateLightDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
	BlendStateLightDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ZERO;
	BlendStateLightDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ONE;
	BlendStateLightDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
	BlendStateLightDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
	BlendStateLightDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
	BlendStateLightDesc.IndependentBlendEnable                = false;
	device->CreateBlendState(&BlendStateLightDesc, &blendStateLight);

	D3D11_DEPTH_STENCIL_DESC dsDesc = {};        ///
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	result = device->CreateDepthStencilState(&dsDesc, &dsOpaque);

	D3D11_DEPTH_STENCIL_DESC dsDescLess = {};    ///
	dsDescLess.DepthEnable    = TRUE;
	dsDescLess.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDescLess.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
	result = device->CreateDepthStencilState(&dsDescLess, &dsLightingLess);

	D3D11_DEPTH_STENCIL_DESC dsDescGreater = {}; ///
	dsDescLess.DepthEnable    = TRUE;
	dsDescLess.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDescLess.DepthFunc      = D3D11_COMPARISON_GREATER_EQUAL;
	result = device->CreateDepthStencilState(&dsDescLess, &dsLightingGreater);

	D3D11_DEPTH_STENCIL_DESC dsDirLightDesc = {};
	dsDescLess.DepthEnable    = FALSE;
	dsDescLess.StencilEnable  = FALSE;
	dsDescLess.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	result = device->CreateDepthStencilState(&dsDirLightDesc, &dsDirLight);

	viewport = std::make_shared<D3D11_VIEWPORT>();
	viewport->TopLeftX = 0;
	viewport->TopLeftY = 0;
	viewport->Width = static_cast<float>(Game::GetInstance()->GetDisplay()->GetClientWidth());
	viewport->Height = static_cast<float>(Game::GetInstance()->GetDisplay()->GetClientHeight());
	viewport->MinDepth = 0;
	viewport->MaxDepth = 1.0f;

	//SHADERS
	//InitializeShader("../Shaders/LightShader.hlsl");

	InitializeOpaqueShader("../Shaders/DeferredShader.hlsl");
	InitializeLightingShader("../Shaders/DeferredLighting.hlsl");
	InitializeLightingShaderPoi("../Shaders/DeferredLightingPoi.hlsl");

	//RASTERIZERS
	CD3D11_RASTERIZER_DESC rastCullBackDesc = {}; ///
	rastCullBackDesc.CullMode = D3D11_CULL_BACK;
	rastCullBackDesc.FillMode = D3D11_FILL_SOLID;
	result = device->CreateRasterizerState(&rastCullBackDesc, &rastCullBack);
	assert(SUCCEEDED(result));

	CD3D11_RASTERIZER_DESC rastCullFrontDesc = {}; ///
	rastCullFrontDesc.CullMode = D3D11_CULL_FRONT;
	rastCullFrontDesc.FillMode = D3D11_FILL_SOLID;
	result = device->CreateRasterizerState(&rastCullFrontDesc, &rastCullFront);
	assert(SUCCEEDED(result));

	//TEXTURE
	D3D11_SAMPLER_DESC samplerStateDesc = {};
	samplerStateDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerStateDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerStateDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerStateDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	result = device->CreateSamplerState(&samplerStateDesc, samplerState.GetAddressOf());
	assert(SUCCEEDED(result));
}

/*
void RenderSystem::InitializeShader(std::string shaderFileName)
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
			std::cout << "Missing Shader File: " << shaderFileName << std::endl;
		}
		return;
	}
	result = device->CreateVertexShader(
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		nullptr, vertexShader.GetAddressOf()
	);
	assert(SUCCEEDED(result));

	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;
	result = D3DCompileFromFile(
		fileName.c_str(),
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		pixelShaderByteCode.GetAddressOf(),
		&errorCode
	);
	assert(SUCCEEDED(result));
	result = device->CreatePixelShader(
		pixelShaderByteCode->GetBufferPointer(),
		pixelShaderByteCode->GetBufferSize(),
		nullptr, pixelShader.GetAddressOf()
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
		},
		D3D11_INPUT_ELEMENT_DESC {
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			D3D11_APPEND_ALIGNED_ELEMENT,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		D3D11_INPUT_ELEMENT_DESC {
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			D3D11_APPEND_ALIGNED_ELEMENT,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};
	result = device->CreateInputLayout(
		inputElements,
		3,
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		inputLayout.GetAddressOf()
	);
	assert(SUCCEEDED(result));

	CD3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_FRONT;
	rastDesc.FillMode = D3D11_FILL_SOLID;
	result = device->CreateRasterizerState(&rastDesc, rastState.GetAddressOf());
	assert(SUCCEEDED(result));
}
*/

void RenderSystem::InitializeOpaqueShader(std::string shaderFileName)
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
			std::cout << "Missing Shader File: " << shaderFileName << std::endl;
		}
		return;
	}
	result = device->CreateVertexShader(
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		nullptr, &vsOpaque
	);
	assert(SUCCEEDED(result));

	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;
	result = D3DCompileFromFile(
		fileName.c_str(),
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		pixelShaderByteCode.GetAddressOf(),
		&errorCode
	);
	assert(SUCCEEDED(result));
	result = device->CreatePixelShader(
		pixelShaderByteCode->GetBufferPointer(),
		pixelShaderByteCode->GetBufferSize(),
		nullptr, &psOpaque
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
		},
		D3D11_INPUT_ELEMENT_DESC {
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			D3D11_APPEND_ALIGNED_ELEMENT,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		D3D11_INPUT_ELEMENT_DESC {
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			D3D11_APPEND_ALIGNED_ELEMENT,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};
	result = device->CreateInputLayout(
		inputElements,
		3,
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		&layoutOpaque
	);
	assert(SUCCEEDED(result));
}

void RenderSystem::InitializeLightingShader(std::string shaderFileName)
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
			std::cout << "Missing Shader File: " << shaderFileName << std::endl;
		}
		return;
	}
	// DIRECTIONAL
	result = device->CreateVertexShader(
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		nullptr, &vsLighting
	);
	assert(SUCCEEDED(result));

	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;
	result = D3DCompileFromFile(
		fileName.c_str(),
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		pixelShaderByteCode.GetAddressOf(),
		&errorCode
	);
	assert(SUCCEEDED(result));
	// DIRECTIONAL
	result = device->CreatePixelShader(
		pixelShaderByteCode->GetBufferPointer(),
		pixelShaderByteCode->GetBufferSize(),
		nullptr, &psLighting
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
	// DIRECTIONAL
	result = device->CreateInputLayout(
		inputElements,
		1,
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		&layoutLighting
	);
	assert(SUCCEEDED(result));
}

void RenderSystem::InitializeLightingShaderPoi(std::string shaderFileName)
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
			std::cout << "Missing Shader File: " << shaderFileName << std::endl;
		}
		return;
	}
	// POINT
	result = device->CreateVertexShader(
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		nullptr, &vsLightingPoi
	);
	assert(SUCCEEDED(result));

	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;
	result = D3DCompileFromFile(
		fileName.c_str(),
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		pixelShaderByteCode.GetAddressOf(),
		&errorCode
	);
	assert(SUCCEEDED(result));
	// POINT
	result = device->CreatePixelShader(
		pixelShaderByteCode->GetBufferPointer(),
		pixelShaderByteCode->GetBufferSize(),
		nullptr, &psLightingPoi
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
	// POINT
	result = device->CreateInputLayout(
		inputElements,
		1,
		vertexShaderByteCode->GetBufferPointer(),
		vertexShaderByteCode->GetBufferSize(),
		&layoutLightingPoi
	);
	assert(SUCCEEDED(result));
}

void RenderSystem::PrepareFrame()
{
	//context->ClearState();
	//context->OMSetRenderTargets(1, renderView.GetAddressOf(), depthView.Get());
	//context->RSSetViewports(1, viewport.get());
	//float backgroundColor[] = { 0.2f, 0.2f, 0.2f };
	//context->ClearRenderTargetView(renderView.Get(), backgroundColor);
	//context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void RenderSystem::Draw()
{
	context->ClearState();
	context->RSSetViewports(1, viewport.get()); //-//
	context->ClearRenderTargetView(gBuffer->diffuseRTV,       Color(0.0f, 0.0f, 0.0f, 1.0f));
	context->ClearRenderTargetView(gBuffer->normalRTV,        Color(0.0f, 0.0f, 0.0f, 1.0f));
	context->ClearRenderTargetView(gBuffer->worldPositionRTV, Color(0.0f, 0.0f, 0.0f, 1.0f));
	ID3D11RenderTargetView* views[] = {
		gBuffer->diffuseRTV,
		gBuffer->normalRTV,
		gBuffer->worldPositionRTV,
		nullptr, nullptr, nullptr, nullptr, nullptr
	};
	context->OMSetRenderTargets(8, views, depthView.Get());
	context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	for (auto& renderComponent : renderComponents)
	{
		renderComponent->DrawOpaque();
	}

	context->ClearState();
	context->OMSetRenderTargets(1, renderView.GetAddressOf(), depthView.Get());
	context->RSSetViewports(1, viewport.get()); //-//
	float backgroundColor[] = { 0.2f, 0.2f, 0.2f };
	context->ClearRenderTargetView(renderView.Get(), backgroundColor);

	Game::GetInstance()->directionalLight->Draw();

	Game::GetInstance()->pointLights->at(0)->Draw();

	Game::GetInstance()->pointLights->at(1)->Draw();
}

void RenderSystem::EndFrame()
{
	context->OMSetRenderTargets(0, nullptr, nullptr);
	swapChain->Present(1, 0);
}