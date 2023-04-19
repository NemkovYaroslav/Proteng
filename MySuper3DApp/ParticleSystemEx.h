#pragma once
#include "Component.h"

#include "magic_enum.hpp"
#include <map>

using namespace DirectX::SimpleMath;

using namespace magic_enum::bitwise_operators;

class ParticleSystemEx : public Component
{
	ID3D11Buffer *particlesPool, *sortBuffer, *deadBuf, *injectionBuf, *constBuf, *countBuf, *debugBuf;

	ID3D11ShaderResourceView *srvSorted, *srvPool;
	ID3D11UnorderedAccessView *uavDead, *uavSorted, *uavSortedRW, *injUav, *uavPool;

public:

	Vector3 Position;
	float Width, Height, Length;

	const unsigned int MaxParticlesCount = 256 * 256 * 16;
	const unsigned int MaxParticlesInjectionCount = 10000;
	UINT ParticlesCount = 0;
	UINT injectionCount = 0;

	struct Particle
	{
		Vector4 Position;  
		Vector4 Velocity;  
		Vector4 Color0;    
		Vector2 Size0Size1;
		float LifeTime;    
	};

	struct ConstData
	{
		Matrix World;                         
		Matrix View;                          
		Matrix Projection;                    
		Vector4 DeltaTimeMaxParticlesGroupdim;
		Vector4 CameraPosX;
	};

	enum class ComputeFlags
	{
		INJECTION   = 1 << 0,
		SIMULATION  = 1 << 1,
		ADD_GRAVITY = 1 << 2,
		MOVE        = 1 << 3,
	};

	std::map<ComputeFlags, ID3D11ComputeShader*> ComputeShaders;

	ConstData constData;

	Particle* injectionParticles = new Particle[MaxParticlesInjectionCount];

	ID3D11VertexShader*   vertShader;
	ID3D11GeometryShader* geomShader;
	ID3D11PixelShader*    pixShader;

	ID3D11RasterizerState*   rastState;
	ID3D11BlendState*        blendState;
	ID3D11DepthStencilState* depthState;

	BitonicSort* sort;



};

class BitonicSort
{

	Game* game;

public:

	struct Params
	{
		unsigned int Level;
		unsigned int LevelMask;
		unsigned int Width;
		unsigned int Height;
	};

	const int NumberOfElements = 256 * 256 * 16;
	const int BitonicBlockSize = 1024;
	const int TransposeBlockSize = 16;
	const int MatrixWidth = BitonicBlockSize;
	const int MatrixHeight = NumberOfElements / BitonicBlockSize;

	ID3D11Buffer* paramsCB;
	ID3D11Buffer* buffer2;
	ID3D11UnorderedAccessView* uavBuf;
	ID3D11ShaderResourceView* srvBuf;

	enum class ComputeFlags
	{
		BITONIC_SORT = 1 << 0,
		TRANSPOSE    = 1 << 1,
	};

	std::map<ComputeFlags, ID3D11ComputeShader*> ComputeShaders;

	BitonicSort(Game* game);

	void Initialize();

	void LoadContent();

	void Dispose();

	void SetConstants(unsigned int iLevel, unsigned int iLevelMask, unsigned int iWidth, unsigned int iHeight);

	void Sort(ID3D11UnorderedAccessView* uav, ID3D11ShaderResourceView* srv);
};

