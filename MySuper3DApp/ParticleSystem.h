#pragma once
#include "Component.h"

using namespace DirectX::SimpleMath;

class ParticleSystem : Component
{
public:

	ID3D11Buffer *bufFirst, *bufSecond, *countBuf, *injectionBuf, *constBuf;

	ID3D11ShaderResourceView *srvFirst, *srvSecond, *srvSrc, *srvDst;
	ID3D11UnorderedAccessView *uavFirst, *uavSecond, *uavSrc, *uavDst, *injUav;

	Vector3 Position;
	float Width, Height, Length;

	const unsigned int MaxParticlesCount = 256 * 256 * 128;
	const unsigned int MaxParticlesInjectionCount = 100;
	UINT injectionCount = 0;

	int ParticlesCount = MaxParticlesCount; // текущее количество частиц

	struct Particle // частица
	{
		Vector4 Position; // позиция
		Vector4 Velocity; // скорость
		Vector4 Color0; // цвет
		Vector2 Size0Size1; // два размера (размер частицы может меняться со временем)
		float LifeTime; // время жизни
	};

	struct ConstData
	{
		Matrix World;
		Matrix View;
		Matrix Projection;
		Vector4 DeltaTimeMaxParticlesGroupdim; // делта тайм, максимальное количество частиц, что-то...
	};

	// INJECTION   // добавляем новые частицы
	// SIMULATION  // читаем перемещение частиц
	// ADD_GRAVITY // добавляем гравитацию

	ID3D11ComputeShader* ComputeShaders;

	ConstData constData;

	Particle* injectionParticles = new Particle[MaxParticlesInjectionCount];

	ID3D11VertexShader*   vertShader;
	ID3D11GeometryShader* geomShader;
	ID3D11PixelShader*    pixShader;

	ID3D11RasterizerState*   rastState;
	ID3D11BlendState*        blendState;
	ID3D11DepthStencilState* depthState;

	ParticleSystem();

	static void GetGroupSize(int partCount, int &groupSizeX, int &groupSizeY);

	void Initialize() override;
	void Update(float deltaTime) override;
	void Draw(float deltaTime);

	void DrawDebugBox();

	void LoadShaders(std::string shaderFileName);

	void CreateBuffers();
	void AddParticle(const Particle& p);
	void SwapBuffers();
};

