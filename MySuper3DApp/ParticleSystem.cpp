#include "ParticleSystem.h"

#include "RenderSystem.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "CameraComponent.h"

#include <iostream>
#include <random>
#include <ctime>

ParticleSystem::ParticleSystem()
{
	Width = 100;
	Height = 100;
	Length = 100;
	Position = Vector3(0, 10, -10);
}

void ParticleSystem::Initialize()
{
	LoadShaders("../Shaders/ParticlesRender.hlsl");

	CreateBuffers();
	D3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_BACK;
	rastDesc.FillMode = D3D11_FILL_SOLID;

	Game::GetInstance()->GetRenderSystem()->device->CreateRasterizerState(&rastDesc, &rastState);

	auto blendStateLightDesc = D3D11_BLEND_DESC{false, false};
	blendStateLightDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateLightDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	blendStateLightDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateLightDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateLightDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateLightDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendStateLightDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateLightDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	Game::GetInstance()->GetRenderSystem()->device->CreateBlendState(&blendStateLightDesc, &blendState);

	auto depthDesc = D3D11_DEPTH_STENCIL_DESC {};
	depthDesc.DepthEnable = TRUE;
	depthDesc.StencilEnable = FALSE;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.StencilReadMask = 0x00;
	depthDesc.StencilWriteMask = 0x00;
	Game::GetInstance()->GetRenderSystem()->device->CreateDepthStencilState(&depthDesc, &depthState);
}

void ParticleSystem::LoadShaders(std::string shaderFileName)
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
		D3DCOMPILE_PACK_MATRIX_ROW_MAJOR,
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
		nullptr, &vertShader
	);
	assert(SUCCEEDED(result));

	Microsoft::WRL::ComPtr<ID3DBlob> geometryShaderByteCode;
	result = D3DCompileFromFile(
		fileName.c_str(),
		nullptr,
		nullptr,
		"GSMain",
		"gs_5_0",
		D3DCOMPILE_PACK_MATRIX_ROW_MAJOR,
		0,
		geometryShaderByteCode.GetAddressOf(),
		&errorCode
	);
	assert(SUCCEEDED(result));
	result = Game::GetInstance()->GetRenderSystem()->device->CreateGeometryShader(
		geometryShaderByteCode->GetBufferPointer(),
		geometryShaderByteCode->GetBufferSize(),
		nullptr, &geomShader
	);

	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;
	result = D3DCompileFromFile(
		fileName.c_str(),
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_PACK_MATRIX_ROW_MAJOR,
		0,
		pixelShaderByteCode.GetAddressOf(),
		&errorCode
	);
	assert(SUCCEEDED(result));
	result = Game::GetInstance()->GetRenderSystem()->device->CreatePixelShader(
		pixelShaderByteCode->GetBufferPointer(),
		pixelShaderByteCode->GetBufferSize(),
		nullptr, &pixShader
	);
	assert(SUCCEEDED(result));

	///

	Microsoft::WRL::ComPtr<ID3DBlob> compShaderByteCode;
	ID3D11ComputeShader* compShader;
	Game::GetInstance()->GetRenderSystem()->device->CreateComputeShader(
		compShaderByteCode->GetBufferPointer(),
		compShaderByteCode->GetBufferSize(),
		nullptr, &compShader
	);
}

float RandomFloat(float min, float max)
{
	return ((float)rand() / RAND_MAX) * (max - min) + min;
}

void ParticleSystem::CreateBuffers()
{
	D3D11_BUFFER_DESC constBufDesc;
	constBufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constBufDesc.Usage = D3D11_USAGE_DEFAULT;
	constBufDesc.MiscFlags = 0;
	constBufDesc.CPUAccessFlags = 0;
	constBufDesc.ByteWidth = sizeof(ConstData);

	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&constBufDesc, nullptr, &constBuf);

	srand(time(NULL));
	auto partsTempBuf = new Particle[MaxParticlesCount];
	for (int i = 0; i < MaxParticlesCount; i++)
	{
		auto pos = Vector4(Length * RandomFloat(-1.0f, 1.0f), Height * RandomFloat(-1.0f, 1.0f), Width * RandomFloat(-1.0f, 1.0f), 1.0f); //???????//
		pos.Normalize();
		pos = pos * 3;
		pos.w = 1.0f;
		auto vel = -pos * 70.01f;
		vel.w = 0.0f;
		vel.y *= 0.01f;
		vel.y += 100.01f;

		partsTempBuf[i] = Particle
		{
			pos,
			vel,
			Vector4(RandomFloat(0.0f, 1.0f), RandomFloat(0.0f, 1.0f), RandomFloat(0.0f, 1.0f), 1.0f),
			Vector2(1.1f, 0.5f),
			RandomFloat(5.0f, 12.0f)
		};
	}

	D3D11_BUFFER_DESC bufDesc;
	bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufDesc.Usage = D3D11_USAGE_DEFAULT;
	bufDesc.CPUAccessFlags = 0;
	bufDesc.StructureByteStride = sizeof(Particle);
	bufDesc.ByteWidth = MaxParticlesCount * sizeof(Particle);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &partsTempBuf[0];
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&bufDesc, &data, &bufFirst);
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&bufDesc, nullptr, &bufSecond);

	delete[] partsTempBuf;

	Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(bufFirst, nullptr, &srvFirst);
	Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(bufSecond, nullptr, &srvSecond);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer = D3D11_BUFFER_UAV
	{
		0,
		MaxParticlesCount,
		D3D11_BUFFER_UAV_FLAG_APPEND
	};

	Game::GetInstance()->GetRenderSystem()->device->CreateUnorderedAccessView(bufFirst, &uavDesc, &uavFirst);
	Game::GetInstance()->GetRenderSystem()->device->CreateUnorderedAccessView(bufSecond, &uavDesc, &uavSecond);

	srvSrc = srvFirst;
	uavSrc = uavFirst;
	srvDst = srvSecond;
	uavDst = uavSecond;

	ID3D11UnorderedAccessView* nuPtr = nullptr;
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uavSrc, &MaxParticlesCount);
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &nuPtr, nullptr);

	D3D11_BUFFER_DESC countBufDesc;
	countBufDesc.BindFlags = 0;
	countBufDesc.Usage = D3D11_USAGE_STAGING;
	countBufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	countBufDesc.MiscFlags = 0;
	countBufDesc.StructureByteStride = 0;
	countBufDesc.ByteWidth = 4;

	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&countBufDesc, nullptr, &countBuf);

	D3D11_BUFFER_DESC injBufDesc;
	injBufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	injBufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	injBufDesc.Usage = D3D11_USAGE_DEFAULT;
	injBufDesc.CPUAccessFlags = 0;
	injBufDesc.StructureByteStride = sizeof(Particle);
	injBufDesc.ByteWidth = MaxParticlesInjectionCount * sizeof(Particle);

	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&injBufDesc, nullptr, &injectionBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC injUavDesc;
	injUavDesc.Format = DXGI_FORMAT_UNKNOWN;
	injUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	injUavDesc.Buffer = D3D11_BUFFER_UAV
	{
		0,
		MaxParticlesInjectionCount,
		D3D11_BUFFER_UAV_FLAG_APPEND
	};

	Game::GetInstance()->GetRenderSystem()->device->CreateUnorderedAccessView(injectionBuf, &injUavDesc, &injUav);
}

void ParticleSystem::Update(float deltaTime)
{
	if (Game::GetInstance()->inputDevice->IsKeyDown(Keys::V))
	{
		deltaTime = 0.0f;
	}

	int groupSizeX, groupSizeY;
	GetGroupSize(ParticlesCount, groupSizeX, groupSizeY);

	constData.World = gameObject->transformComponent->GetModel();
	constData.View = Game::GetInstance()->currentCamera->gameObject->transformComponent->GetView();
	constData.Projection = Game::GetInstance()->currentCamera->GetProjection();
	constData.DeltaTimeMaxParticlesGroupdim = Vector4(deltaTime, ParticlesCount, groupSizeY, 0);

	Game::GetInstance()->GetRenderSystem()->context->UpdateSubresource(constBuf, 0, nullptr, &constData, 0, 0);

	Game::GetInstance()->GetRenderSystem()->context->CSSetConstantBuffers(0, 1, &constBuf);

	const UINT counterKeepValue = -1;
	const UINT counterZero = 0;

	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uavSrc, &counterKeepValue);
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(1, 1, &uavDst, &counterZero);

	Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders, nullptr, 0); //???????//

	if (groupSizeX > 0)
	{
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(groupSizeX, groupSizeY, 1);
	}

	if (injectionCount > 0)
	{
		Game::GetInstance()->GetRenderSystem()->context->CSSetConstantBuffers(0, 1, nullptr);

		int injSizeX, injSizeY;
		GetGroupSize(injectionCount, injSizeX, injSizeY);

		constData.DeltaTimeMaxParticlesGroupdim = Vector4(deltaTime, injectionCount, injSizeY, 0);

		Game::GetInstance()->GetRenderSystem()->context->UpdateSubresource(constBuf, 0, nullptr, &constData, 0, 0);
		Game::GetInstance()->GetRenderSystem()->context->CSSetConstantBuffers(0, 1, &constBuf);

		Game::GetInstance()->GetRenderSystem()->context->UpdateSubresource(injectionBuf, 0, nullptr, injectionParticles, 0, 0);

		Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &injUav, &injectionCount);
		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders, nullptr, 0);

		Game::GetInstance()->GetRenderSystem()->context->Dispatch(injSizeX, injSizeY, 1);

		injectionCount = 0;
	}

	///

}