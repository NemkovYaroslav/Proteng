#include "ParticleSystemEx.h"

#include "RenderSystem.h"

// COMMON

template<typename T>
std::vector<D3D_SHADER_MACRO> GetMacros(T flags)
{
	std::vector<D3D_SHADER_MACRO> macros;

	constexpr auto& entries = magic_enum::enum_entries<T>();

	for (const std::pair<T, std::string_view>& p : entries)
	{
		if (static_cast<uint32_t>(flags & p.first) > 0)
		{
			D3D_SHADER_MACRO macro;
			macro.Name = p.second.data();
			macro.Definition = "1";

			macros.push_back(macro);
		}
	}

	macros.push_back(D3D_SHADER_MACRO{ nullptr, nullptr });
	return macros;
}

// PARTICLE SYSTEM EX

ParticleSystemEx::ParticleSystemEx()
{
	Width    = 100;
	Height   = 100;
	Length   = 100;
	Position = Vector3(100, 10, 10);
}

void ParticleSystemEx::Initialize()
{
	LoadShaders();
	CreateBuffers();

	sort = new BitonicSort(Game::GetInstance());

	D3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_BACK;
	rastDesc.FillMode = D3D11_FILL_SOLID;
	Game::GetInstance()->GetRenderSystem()->device->CreateRasterizerState(&rastDesc, &rastState);

	auto blendStateDesc = D3D11_BLEND_DESC{ false, false };
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	Game::GetInstance()->GetRenderSystem()->device->CreateBlendState(&blendStateDesc, &blendState);

	auto depthDesc = D3D11_DEPTH_STENCIL_DESC{};
	depthDesc.DepthEnable = TRUE;
	depthDesc.StencilEnable = FALSE;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.StencilReadMask = 0x00;
	depthDesc.StencilWriteMask = 0x00;
	Game::GetInstance()->GetRenderSystem()->device->CreateDepthStencilState(&depthDesc, &depthState);

	//Initialize();
}

void ParticleSystemEx::CreateBuffers()
{
	D3D11_BUFFER_DESC constBufDesc;
	constBufDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	constBufDesc.Usage          = D3D11_USAGE_DEFAULT;
	constBufDesc.MiscFlags      = 0;
	constBufDesc.CPUAccessFlags = 0;
	constBufDesc.ByteWidth      = sizeof(ConstData);
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&constBufDesc, nullptr, &constBuf);

	D3D11_BUFFER_DESC poolBufDesc;
	poolBufDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	poolBufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	poolBufDesc.Usage               = D3D11_USAGE_DEFAULT;
	poolBufDesc.CPUAccessFlags      = 0;
	poolBufDesc.StructureByteStride = sizeof(Particle);
	poolBufDesc.ByteWidth           = MaxParticlesCount * sizeof(Particle);
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&poolBufDesc, nullptr, &particlesPool);
	Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(particlesPool, nullptr, &srvPool);
	Game::GetInstance()->GetRenderSystem()->device->CreateUnorderedAccessView(particlesPool, nullptr, &uavPool);

	//sortBuffer, deadBuf
	D3D11_BUFFER_DESC sortBufDesc;
	sortBufDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	sortBufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sortBufDesc.Usage               = D3D11_USAGE_DEFAULT;
	sortBufDesc.CPUAccessFlags      = 0;
	poolBufDesc.StructureByteStride = sizeof(Vector2);
	sortBufDesc.ByteWidth           = MaxParticlesCount * sizeof(Vector2);
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&sortBufDesc, nullptr, &sortBuffer);

	auto indeces = new UINT[MaxParticlesCount];
	for (UINT i = 0; i < MaxParticlesCount; i++)
	{
		indeces[i] = MaxParticlesCount - 1 - i; // в обратном порядке
	}
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem          = &indeces[0];
	data.SysMemPitch      = 0;
	data.SysMemSlicePitch = 0;
	D3D11_BUFFER_DESC deadBufDesc;
	deadBufDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	deadBufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	deadBufDesc.Usage               = D3D11_USAGE_DEFAULT;
	deadBufDesc.CPUAccessFlags      = 0;
	deadBufDesc.StructureByteStride = sizeof(UINT);
	deadBufDesc.ByteWidth           = MaxParticlesCount * sizeof(UINT);
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&deadBufDesc, &data, &deadBuf);
	delete[] indeces;

	Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(sortBuffer, nullptr, &srvSorted);

	//GG//
	/*
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
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uavSrc, &MaxParticlesCount); // если хотим сохранить предыдущее значение, то пишем в конце -1
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &nuPtr, nullptr);

	D3D11_BUFFER_DESC countBufDesc;
	countBufDesc.BindFlags = 0;
	countBufDesc.Usage = D3D11_USAGE_STAGING; // мы читаем из него
	countBufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	countBufDesc.MiscFlags = 0;
	countBufDesc.StructureByteStride = 0;
	countBufDesc.ByteWidth = 4;
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&countBufDesc, nullptr, &countBuf); // буфер, с помощью которого узнаем значения, которые поместили в UnorderedAccessView выше

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
	*/
}

// BITONIC SORT

void BitonicSort::Sort(ID3D11UnorderedAccessView* uav, ID3D11ShaderResourceView* srv)
{
	Game::GetInstance()->GetRenderSystem()->context->ClearState();
	//Game::GetInstance()->RestoreTargets();

	UINT initialCount = -1;

	// First sort the rows for the levels <= to the block size
	for (unsigned int level = 2; level <= BitonicBlockSize; level = level * 2)
	{
		SetConstants(level, level, MatrixWidth, MatrixHeight);

		// Sort the row data
		Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uav, &initialCount);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::BITONIC_SORT], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(NumberOfElements / BitonicBlockSize, 1, 1);
	}

	for (UINT level = (BitonicBlockSize * 2); level <= NumberOfElements; level = level * 2)
	{
		SetConstants((level / BitonicBlockSize), (UINT)(level & -NumberOfElements) / BitonicBlockSize, MatrixWidth, MatrixHeight);

		// Transpose the data from buffer 1 into buffer 2
		ID3D11ShaderResourceView* nulResView = nullptr;
		Game::GetInstance()->GetRenderSystem()->context->CSSetShaderResources(0, 1, &nulResView);
		Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uavBuf, &initialCount);
		Game::GetInstance()->GetRenderSystem()->context->CSSetShaderResources(0, 1, &srv);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::TRANSPOSE], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(MatrixWidth / TransposeBlockSize, MatrixHeight / TransposeBlockSize, 1);

		// Sort the transposed column data
		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::BITONIC_SORT], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(NumberOfElements / BitonicBlockSize, 1, 1);

		SetConstants(BitonicBlockSize, level, MatrixWidth, MatrixHeight);

		// Transpose the data from buffer 2 back into buffer 1
		Game::GetInstance()->GetRenderSystem()->context->CSSetShaderResources(0, 1, &nulResView);
		Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uav, &initialCount);
		Game::GetInstance()->GetRenderSystem()->context->CSSetShaderResources(0, 1, &srvBuf);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::TRANSPOSE], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(MatrixHeight / TransposeBlockSize, MatrixHeight / TransposeBlockSize, 1);

		// Sort the row data
		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::BITONIC_SORT], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(NumberOfElements / BitonicBlockSize, 1, 1);
	}
}