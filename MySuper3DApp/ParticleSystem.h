#pragma once
#include "Component.h"

#include "magic_enum.hpp"
#include <map>

using namespace DirectX::SimpleMath;

using namespace magic_enum::bitwise_operators;

class ParticleSystem : public Component
{
public:

	ID3D11Buffer *bufFirst, *bufSecond, *countBuf, *injectionBuf, *constBuf;
	// countBuf - используется чтобы получать количество живых или мертвых частиц
	// injectionBuf - с помощью его можно добавить новые частицы

	ID3D11ShaderResourceView  *srvFirst, *srvSecond, *srvSrc, *srvDst;
	ID3D11UnorderedAccessView *uavFirst, *uavSecond, *uavSrc, *uavDst, *injUav;
	// srvFirst и srvSecond - используются для переливания

	// КУБ - ОБЛАСТЬ генерации частиц
	Vector3 Position;
	float Width, Height, Length;

	const unsigned int MaxParticlesCount = 1024;          // максимальное количество частиц
	const unsigned int MaxParticlesInjectionCount = 256; // максимальное количество частиц, которое можно добавить за 1 кадр, не больше 100
	UINT injectionCount = 0;                             // количество частиц, которое мы добавляем на текущем кадре
	int particlesCount = MaxParticlesCount;              // текущее количество частиц

	struct Particle         // структура, описывающая частицу
	{
		Vector4 Position;   // позиция частицы
		Vector4 Velocity;   // скорость частицы
		Vector4 Color0;     // цвет частицы
		Vector2 Size0Size1; // два размера частицы (размер может меняться со временем)
		float LifeTime;     // время жизни частицы
	};

	struct ConstData
	{
		Matrix World;                          // матрица мировой позиции
		Matrix View;                           // матрица вида камеры
		Matrix Projection;                     // матрица проекции камеры
		Vector4 DeltaTimeMaxParticlesGroupdim; // tick, максимальное количество частиц, группа
	};

	enum class ComputeFlags
	{
		INJECTION   = 1 << 0, // добавляем новые частицы
		SIMULATION  = 1 << 1, // считаем перемещение частиц
		ADD_GRAVITY = 1 << 2, // добавляем гравитацию
	};

	std::map<ComputeFlags, ID3D11ComputeShader*> ComputeShaders;

	ConstData constData;

	Particle* injectionParticles = new Particle[MaxParticlesInjectionCount]; // банк добавляемых частиц

	ID3D11VertexShader*   vertShader; // для отрисовки частиц
	ID3D11GeometryShader* geomShader; // для отрисовки частиц
	ID3D11PixelShader*    pixShader;  // для отрисовки частиц

	ID3D11RasterizerState*   rastState;
	ID3D11BlendState*        blendState;
	ID3D11DepthStencilState* depthState;

	ParticleSystem();

	static void GetGroupSize(int partCount, int& groupSizeX, int& groupSizeY);
	void SwapBuffers();

	void Initialize() override;
	void Update(float deltaTime) override;
	void Draw(float deltaTime);

	void LoadShaders(std::string shaderFileName);
	void CreateBuffers();
};

