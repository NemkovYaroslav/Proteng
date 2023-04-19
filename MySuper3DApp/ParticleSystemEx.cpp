#include "ParticleSystemEx.h"

#include "RenderSystem.h"

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

void BitonicSort::Sort(ID3D11UnorderedAccessView* uav, ID3D11ShaderResourceView* srv)
{
	Game::GetInstance()->GetRenderSystem()->context->ClearState();
	//Game::GetInstance()->RestoreTargets();

	UINT initialCount = -1;

	for (unsigned int level = 2; level <= BitonicBlockSize; level = level * 2)
	{
		SetConstants(level, level, MatrixWidth, MatrixHeight);

		Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uav, &initialCount);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::BITONIC_SORT], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(NumberOfElements / BitonicBlockSize, 1, 1);
	}

	for (UINT level = (BitonicBlockSize * 2); level <= NumberOfElements; level = level * 2)
	{
		SetConstants((level / BitonicBlockSize), (UINT)(level & -NumberOfElements) / BitonicBlockSize, MatrixWidth, MatrixHeight);

		ID3D11ShaderResourceView* nulResView = nullptr;
		Game::GetInstance()->GetRenderSystem()->context->CSSetShaderResources(0, 1, &nulResView);
		Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uavBuf, &initialCount);
		Game::GetInstance()->GetRenderSystem()->context->CSSetShaderResources(0, 1, &srv);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::TRANSPOSE], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(MatrixWidth / TransposeBlockSize, MatrixHeight / TransposeBlockSize, 1);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::BITONIC_SORT], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(NumberOfElements / BitonicBlockSize, 1, 1);

		SetConstants(BitonicBlockSize, level, MatrixWidth, MatrixHeight);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShaderResources(0, 1, &nulResView);
		Game::GetInstance()->GetRenderSystem()->context->CSSetUnorderedAccessViews(0, 1, &uav, &initialCount);
		Game::GetInstance()->GetRenderSystem()->context->CSSetShaderResources(0, 1, &srvBuf);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::TRANSPOSE], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(MatrixHeight / TransposeBlockSize, MatrixHeight / TransposeBlockSize, 1);

		Game::GetInstance()->GetRenderSystem()->context->CSSetShader(ComputeShaders[ComputeFlags::BITONIC_SORT], nullptr, 0);
		Game::GetInstance()->GetRenderSystem()->context->Dispatch(NumberOfElements / BitonicBlockSize, 1, 1);
	}
}