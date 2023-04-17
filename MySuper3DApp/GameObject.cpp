#include "GameObject.h"

#include "TransformComponent.h"
#include "RenderComponent.h"
#include "RenderShadowsComponent.h"
#include "Component.h"

#include "Game.h"
#include "CameraComponent.h"
#include "CameraControllerComponent.h"

#include "PointLightComponent.h"

GameObject::GameObject(GameObject* parent)
{
	this->transformComponent = new TransformComponent();
	AddComponent(transformComponent);
}

GameObject::~GameObject()
{
	for (auto component : components)
	{
		delete component;
	}
	components.clear();
}

void GameObject::Initialize()
{
	for (const auto& component : components)
	{
		component->Initialize();
	}
}

void GameObject::Update(float deltaTime)
{
	for (const auto& component : components)
	{
		component->Update(deltaTime);
	}
}

void GameObject::AddComponent(Component* component)
{
	components.push_back(component);
	component->gameObject = this;
}

void GameObject::CreatePlane(float planeSize, std::string textureFileName)
{
	modelComponent = new ModelComponent(textureFileName);
	modelComponent->AddPlane(planeSize);
	AddComponent(modelComponent);
	
	//SHADOWS
	renderShadowsComponent = new RenderShadowsComponent(modelComponent);
	AddComponent(renderShadowsComponent);

	//MAIN FRAME
	renderComponent = new RenderComponent(modelComponent);
	AddComponent(renderComponent);
}

void GameObject::CreateMesh(float scaleRate, std::string textureFileName, std::string objectFileName)
{
	modelComponent = new ModelComponent(textureFileName);
	modelComponent->AddMesh(scaleRate, objectFileName);
	AddComponent(modelComponent);
	//SHADOWS
	renderShadowsComponent = new RenderShadowsComponent(modelComponent);
	AddComponent(renderShadowsComponent);
	//MAIN FRAME
	renderComponent = new RenderComponent(modelComponent);
	AddComponent(renderComponent);
}