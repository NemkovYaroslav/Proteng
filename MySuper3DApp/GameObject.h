#pragma once
#include "includes.h"

class TransformComponent;
class RenderComponent;
class ModelComponent;
class RenderShadowsComponent;
class Component;
class PointLightComponent;

using namespace DirectX::SimpleMath;

class GameObject
{
public:

	//COMPONENTS
	TransformComponent* transformComponent;
	RenderComponent* renderComponent;
	ModelComponent* modelComponent;
	RenderShadowsComponent* renderShadowsComponent;

	std::vector<Component*> components;

	GameObject(GameObject* parent = nullptr);
	~GameObject();

	virtual void Initialize();
	virtual void Update(float deltaTime);

	void AddComponent(Component* component);

	void CreatePlane(float planeSize, std::string textureFileName);
	void CreateMesh(float scaleRate, std::string textureFileName, std::string objectFileName);
};