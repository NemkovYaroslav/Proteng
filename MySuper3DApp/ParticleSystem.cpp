#include "ParticleSystem.h"

#include "RenderSystem.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "CameraComponent.h"

#include <iostream>
#include <random>
#include <ctime>

float RandomFloat(float min, float max)
{
	return ((float)rand() / RAND_MAX) * (max - min) + min;
}

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

ParticleSystem::ParticleSystem()
{
	Width    = 100;
	Height   = 100;
	Length   = 100;
	Position = Vector3(0, 10, -10);
}

void ParticleSystem::Initialize()
{
	LoadShaders("../Shaders/ParticlesRender.hlsl"); // создаем шейдеры
	CreateBuffers();                                // создаем буферы

	D3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_BACK;
	rastDesc.FillMode = D3D11_FILL_SOLID;
	Game::GetInstance()->GetRenderSystem()->device->CreateRasterizerState(&rastDesc, &rastState);

	auto blendStateDesc = D3D11_BLEND_DESC{false, false};
	blendStateDesc.RenderTarget[0].BlendEnable           = TRUE;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	blendStateDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
	Game::GetInstance()->GetRenderSystem()->device->CreateBlendState(&blendStateDesc, &blendState);

	auto depthDesc = D3D11_DEPTH_STENCIL_DESC {};
	depthDesc.DepthEnable      = TRUE;
	depthDesc.StencilEnable    = FALSE;
	depthDesc.DepthFunc        = D3D11_COMPARISON_LESS;
	depthDesc.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.StencilReadMask  = 0x00;
	depthDesc.StencilWriteMask = 0x00;
	Game::GetInstance()->GetRenderSystem()->device->CreateDepthStencilState(&depthDesc, &depthState);
}

void ParticleSystem::GetGroupSize(int partCount, int& groupSizeX, int& groupSizeY)
{
	int numGroups = (partCount % 256 != 0) ? ((partCount / 256) + 1) : (partCount / 256);
	double secondRoot = std::pow((double)numGroups, (double)(1.0f / 2.0f));
	secondRoot = std::ceil(secondRoot); // вычисл€ет наименьшее integer значение не меньше чем secondRoot
	groupSizeX = (int)secondRoot;
	groupSizeY = groupSizeX;
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

	std::vector<ComputeFlags> flags = {
		ComputeFlags::INJECTION,
		ComputeFlags::SIMULATION,
		ComputeFlags::SIMULATION | ComputeFlags::ADD_GRAVITY,
	};

	for (auto &flag : flags)
	{
		auto macros = GetMacros(flag);
		Microsoft::WRL::ComPtr<ID3DBlob> computerShaderByteCode;
		result = D3DCompileFromFile(
			fileName.c_str(),
			&macros[0],
			nullptr,
			"CSMain",
			"cs_5_0",
			D3DCOMPILE_PACK_MATRIX_ROW_MAJOR,
			0,
			computerShaderByteCode.GetAddressOf(),
			&errorCode
		);
		assert(SUCCEEDED(result));
		Microsoft::WRL::ComPtr<ID3DBlob> compShaderByteCode;
		ID3D11ComputeShader* compShader;
		Game::GetInstance()->GetRenderSystem()->device->CreateComputeShader(
			compShaderByteCode->GetBufferPointer(),
			compShaderByteCode->GetBufferSize(),
			nullptr, &compShader
		);
		ComputeShaders.emplace(flag, compShader);
	}
}

void ParticleSystem::CreateBuffers()
{
	D3D11_BUFFER_DESC constBufDesc;
	constBufDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	constBufDesc.Usage          = D3D11_USAGE_DEFAULT;
	constBufDesc.MiscFlags      = 0;
	constBufDesc.CPUAccessFlags = 0;
	constBufDesc.ByteWidth      = sizeof(ConstData);
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&constBufDesc, nullptr, &constBuf);

	srand(time(NULL));
	auto partsTempBuf = new Particle[MaxParticlesCount]; // берем наш массив частиц
	for (int i = 0; i < MaxParticlesCount; i++)
	{
		// задаем начальную позицию, скорости
		auto pos = Vector4(Length * RandomFloat(-1.0f, 1.0f), Height * RandomFloat(-1.0f, 1.0f), Width * RandomFloat(-1.0f, 1.0f), 1.0f); //???????//
		pos.Normalize();
		pos = pos * 3;
		pos.w = 1.0f;
		auto vel = -pos * 70.01f;
		vel.w = 0.0f;
		vel.y *= 0.01f;
		vel.y += 100.01f;

		partsTempBuf[i] = Particle // инициализируем каждую частицу
		{
			pos,
			vel,
			Vector4(RandomFloat(0.0f, 1.0f), RandomFloat(0.0f, 1.0f), RandomFloat(0.0f, 1.0f), 1.0f),
			Vector2(1.1f, 0.5f),
			RandomFloat(5.0f, 12.0f)
		};
	}

	// делаем константный буфер —“–” “”–Ќџћ
	D3D11_BUFFER_DESC bufDesc;
	bufDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE; // UNORDERED_ACCESS (можем читать и писать)
	bufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; //                 // SHADER_RESOURCE  (можем читать в обычном графическом пайплайне)
	bufDesc.Usage               = D3D11_USAGE_DEFAULT;
	bufDesc.CPUAccessFlags      = 0;
	bufDesc.StructureByteStride = sizeof(Particle); // указываем размер 1 структуры
	bufDesc.ByteWidth           = MaxParticlesCount * sizeof(Particle); // указываем максимальный размер буфера
	// создаем "начальные данные"
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem          = &partsTempBuf[0];
	data.SysMemPitch      = 0;
	data.SysMemSlicePitch = 0;
	// подгружаем начальные данные массива частиц в "первый" буфер
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&bufDesc,  &data,  &bufFirst);
	// второй буфер оставл€ем пустым (в него будем переливать наши частицы)
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&bufDesc, nullptr, &bufSecond);
	delete[] partsTempBuf;
	// на основе буферов создали ShaderResourceView'ы
	Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(bufFirst, nullptr, &srvFirst);
	Game::GetInstance()->GetRenderSystem()->device->CreateShaderResourceView(bufSecond, nullptr, &srvSecond);

	// т.к. используем UNORDERED, заполн€ем эту штуку
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format        = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer = D3D11_BUFFER_UAV
	{
		0, // первый элемент
		MaxParticlesCount, // количество
		D3D11_BUFFER_UAV_FLAG_APPEND // UNORDERED_ACCESS_VIEW может быть как APPEND и как CONSUME
	};
	Game::GetInstance()->GetRenderSystem()->device->CreateUnorderedAccessView(bufFirst, &uavDesc, &uavFirst);
	Game::GetInstance()->GetRenderSystem()->device->CreateUnorderedAccessView(bufSecond, &uavDesc, &uavSecond);

	// т.к. дальше будем их мен€ть
	srvSrc = srvFirst;  // source
	uavSrc = uavFirst;
	srvDst = srvSecond; // destination
	uavDst = uavSecond;

	ID3D11UnorderedAccessView* nuPtr = nullptr;
	// здесь &MaxParticlesCount - значение счЄтчика (если хотим сохранить значение, которое было, нужно указать -1)
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uavSrc, &MaxParticlesCount);
	// ???
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &nuPtr, nullptr);

	// буфер, с помощью которого узнаем значени€, хранищиес€ в счЄтчике
	D3D11_BUFFER_DESC countBufDesc;
	countBufDesc.BindFlags           = 0;
	countBufDesc.Usage               = D3D11_USAGE_STAGING;   // т.к. это ресурс, из которого мы читаем
	countBufDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
	countBufDesc.MiscFlags           = 0;
	countBufDesc.StructureByteStride = 0;
	countBufDesc.ByteWidth           = 4; // в 1 int'е 4 байта
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&countBufDesc, nullptr, &countBuf); // буфер, с помощью которого узнаем значени€, которые поместили в UnorderedAccessView выше

	// буфер дл€ добавлени€ частиц за 1 кадр
	D3D11_BUFFER_DESC injBufDesc;
	injBufDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	injBufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	injBufDesc.Usage               = D3D11_USAGE_DEFAULT;
	injBufDesc.CPUAccessFlags      = 0;
	injBufDesc.StructureByteStride = sizeof(Particle);
	injBufDesc.ByteWidth           = MaxParticlesInjectionCount * sizeof(Particle);
	Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&injBufDesc, nullptr, &injectionBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC injUavDesc;
	injUavDesc.Format        = DXGI_FORMAT_UNKNOWN;
	injUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	injUavDesc.Buffer = D3D11_BUFFER_UAV
	{
		0,
		MaxParticlesInjectionCount,
		D3D11_BUFFER_UAV_FLAG_APPEND
	};
	Game::GetInstance()->GetRenderSystem()->device->CreateUnorderedAccessView(injectionBuf, &injUavDesc, &injUav);
}

void ParticleSystem::AddParticle(const Particle& p)
{

}

void ParticleSystem::SwapBuffers()
{
	ID3D11ShaderResourceView* srvTemp;
	ID3D11UnorderedAccessView* uavTemp;

	srvTemp = srvSrc;
	srvSrc  = srvDst;
	srvDst  = srvTemp;

	uavTemp = uavSrc;
	uavSrc  = uavDst;
	uavDst  = uavTemp;
}

void ParticleSystem::Update(float deltaTime)
{
	if (Game::GetInstance()->inputDevice->IsKeyDown(Keys::V))
	{
		deltaTime = 0.0f;
	}

	// Const Buffer Standart Values
	constData.World = gameObject->transformComponent->GetModel();
	constData.View = Game::GetInstance()->currentCamera->gameObject->transformComponent->GetView();
	constData.Projection = Game::GetInstance()->currentCamera->GetProjection();

	// Group size ParticlesCount
	int groupSizeX, groupSizeY;
	GetGroupSize(ParticlesCount, groupSizeX, groupSizeY);
	constData.DeltaTimeMaxParticlesGroupdim = Vector4(deltaTime, ParticlesCount, groupSizeY, 0);
	Game::GetInstance()->GetRenderSystem()->context->UpdateSubresource(constBuf, 0, nullptr, &constData, 0, 0);
	Game::GetInstance()->GetRenderSystem()->context->CSSetConstantBuffers(0, 1, &constBuf);

	// update points positions, remove too old
	const UINT counterKeepValue = -1;
	const UINT counterZero      = 0;
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uavSrc, &counterKeepValue); // читаем частицы, сохран€ем позиции точек через -1
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(1, 1, &uavDst, &counterZero);      // сюда записываем поэтому ставим 0

	Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::SIMULATION | ComputeFlags::ADD_GRAVITY], nullptr, 0); //???????//

	if (groupSizeX > 0)
	{
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(groupSizeX, groupSizeY, 1);
	}

	// add new points if any
	if (injectionCount > 0)
	{
		Game::GetInstance()->GetRenderSystem()->context->CSSetConstantBuffers(0, 1, nullptr);

		// Group size InjectionCount
		int injSizeX, injSizeY;
		GetGroupSize(injectionCount, injSizeX, injSizeY);
		constData.DeltaTimeMaxParticlesGroupdim = Vector4(deltaTime, injectionCount, injSizeY, 0);
		Game::GetInstance()->GetRenderSystem()->context->UpdateSubresource(constBuf, 0, nullptr, &constData, 0, 0);
		Game::GetInstance()->GetRenderSystem()->context->CSSetConstantBuffers(0, 1, &constBuf);

		Game::GetInstance()->GetRenderSystem()->context->UpdateSubresource(injectionBuf, 0, nullptr, injectionParticles, 0, 0);

		Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &injUav, &injectionCount);
		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::INJECTION], nullptr, 0); //???????//

		Game::GetInstance()->GetRenderSystem()->context->Dispatch(injSizeX, injSizeY, 1);

		injectionCount = 0;
	}

	ID3D11UnorderedAccessView* nuPtr = nullptr;
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &nuPtr, &counterZero);
	Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(1, 1, &nuPtr, &counterZero);

	// get points count
	Game::GetInstance()->GetRenderSystem()->context->CopyStructureCount(countBuf, 0, uavDst);

	D3D11_MAPPED_SUBRESOURCE subresource;
	Game::GetInstance()->GetRenderSystem()->context->Map(countBuf, 0, D3D11_MAP_READ, 0, &subresource);

	UINT* data = reinterpret_cast<UINT*>(subresource.pData);
	ParticlesCount = data[0];
	Game::GetInstance()->GetRenderSystem()->context->Unmap(countBuf, 0);

	SwapBuffers();
}

void ParticleSystem::Draw(float deltaTime)
{
	// draw points
	//Game::GetInstance()->GetRenderSystem()->context->ClearState();
	//Game::GetInstance()->RestoreTargets();//

	ID3D11RasterizerState* oldState = nullptr;
	Game::GetInstance()->GetRenderSystem()->context->RSGetState(&oldState);
	Game::GetInstance()->GetRenderSystem()->context->RSSetState(rastState);

	ID3D11BlendState* oldBlend = nullptr;
	UINT oldMask = 0;
	float old_blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	Game::GetInstance()->GetRenderSystem()->context->OMGetBlendState(&oldBlend, old_blend_factor, &oldMask);

	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	Game::GetInstance()->GetRenderSystem()->context->OMSetBlendState(blendState, blend_factor, 0xffffffff);

	ID3D11DepthStencilState* oldDepthState = nullptr;
	UINT oldStenBuf = 0;
	Game::GetInstance()->GetRenderSystem()->context->OMGetDepthStencilState(&oldDepthState, &oldStenBuf);
	Game::GetInstance()->GetRenderSystem()->context->OMSetDepthStencilState(depthState, 0);

	Game::GetInstance()->GetRenderSystem()->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	Game::GetInstance()->GetRenderSystem()->context->VSSetShader(vertShader, nullptr, 0);
	Game::GetInstance()->GetRenderSystem()->context->GSSetShader(geomShader, nullptr, 0);
	Game::GetInstance()->GetRenderSystem()->context->PSSetShader(pixShader, nullptr, 0);

	Game::GetInstance()->GetRenderSystem()->context->GSSetConstantBuffers(0, 1, &constBuf);
	Game::GetInstance()->GetRenderSystem()->context->GSSetShaderResources(0, 1, &srvSrc);

	Game::GetInstance()->GetRenderSystem()->context->Draw(ParticlesCount, 0);

	Game::GetInstance()->GetRenderSystem()->context->OMSetBlendState(oldBlend, old_blend_factor, oldMask);
	Game::GetInstance()->GetRenderSystem()->context->RSSetState(oldState);
	Game::GetInstance()->GetRenderSystem()->context->OMSetDepthStencilState(oldDepthState, oldStenBuf);
}