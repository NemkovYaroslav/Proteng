#include "PointLightComponent.h"
#include "GameObject.h"
#include "Game.h"
#include "RenderSystem.h"
#include "DisplayWin32.h"
#include "CameraComponent.h"
#include "GBuffer.h"

struct alignas(16) CameraData
{
	Matrix  camView;
	Matrix  camProjection;
	Vector4 camPosition;
};

struct PointLightData
{
	Matrix  poiModel;
	Vector4 poiLightColor;
	Vector4 poiConstLinearQuadCount;
	Vector4 poiPosition;
};
struct alignas(16) LightData
{
	PointLightData PoiLight;
};

PointLightComponent::PointLightComponent(float constant, float linear, float quadratic)
{
	this->constant  = constant;
	this->linear    = linear;
	this->quadratic = quadratic;
}

void PointLightComponent::Initialize()
{
	D3D11_BUFFER_DESC vertexBufDesc = {};
	vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufDesc.CPUAccessFlags = 0;
	vertexBufDesc.MiscFlags = 0;
	vertexBufDesc.StructureByteStride = 0;
	vertexBufDesc.ByteWidth = sizeof(DirectX::XMFLOAT4) * std::size(poiPoints);
	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = poiPoints.data();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	auto result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&vertexBufDesc, &vertexData, poiVertexBuffer.GetAddressOf());
	assert(SUCCEEDED(result));

	D3D11_BUFFER_DESC indexBufDesc = {};
	indexBufDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufDesc.CPUAccessFlags = 0;
	indexBufDesc.MiscFlags = 0;
	indexBufDesc.StructureByteStride = 0;
	indexBufDesc.ByteWidth = sizeof(int) * std::size(poiIndices);
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = poiIndices.data();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&indexBufDesc, &indexData, poiIndexBuffer.GetAddressOf());
	assert(SUCCEEDED(result));
	
	std::cout << "!!!!!!!!!!!!!!!!" << std::endl;

	constBuffer = new ID3D11Buffer * [2];

	D3D11_BUFFER_DESC firstConstBufferDesc = {};
	firstConstBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	firstConstBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	firstConstBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	firstConstBufferDesc.MiscFlags = 0;
	firstConstBufferDesc.StructureByteStride = 0;
	firstConstBufferDesc.ByteWidth = sizeof(CameraData);
	result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&firstConstBufferDesc, nullptr, &constBuffer[0]);
	assert(SUCCEEDED(result));

	D3D11_BUFFER_DESC secondConstBufferDesc = {};
	secondConstBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	secondConstBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	secondConstBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	secondConstBufferDesc.MiscFlags = 0;
	secondConstBufferDesc.StructureByteStride = 0;
	secondConstBufferDesc.ByteWidth = sizeof(LightData);
	result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&secondConstBufferDesc, nullptr, &constBuffer[1]);
	assert(SUCCEEDED(result));
}

void PointLightComponent::Draw()
{
	const CameraData cameraData
	{
		Game::GetInstance()->currentCamera->gameObject->transformComponent->GetView(),
		Game::GetInstance()->currentCamera->GetProjection(),
		Vector4(Game::GetInstance()->currentCamera->gameObject->transformComponent->GetPosition()),
	};
	D3D11_MAPPED_SUBRESOURCE firstMappedResource;
	Game::GetInstance()->GetRenderSystem()->context->Map(constBuffer[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &firstMappedResource);
	memcpy(firstMappedResource.pData, &cameraData, sizeof(CameraData));
	Game::GetInstance()->GetRenderSystem()->context->Unmap(constBuffer[0], 0);

	LightData lightData
	{
		gameObject->transformComponent->GetModel(),
		lightColor,
		Vector4(1.0f, 0.09f, 0.032f, 1.0f),
		Vector4(gameObject->transformComponent->GetPosition()),
	};
	D3D11_MAPPED_SUBRESOURCE secondMappedResource;
	Game::GetInstance()->GetRenderSystem()->context->Map(constBuffer[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &secondMappedResource);
	memcpy(secondMappedResource.pData, &lightData, sizeof(LightData));
	Game::GetInstance()->GetRenderSystem()->context->Unmap(constBuffer[1], 0);

	///

	ID3D11ShaderResourceView* resources[] = {
		Game::GetInstance()->GetRenderSystem()->gBuffer->diffuseSRV,
		Game::GetInstance()->GetRenderSystem()->gBuffer->normalSRV,
		Game::GetInstance()->GetRenderSystem()->gBuffer->worldPositionSRV
	};
	Game::GetInstance()->GetRenderSystem()->context->PSSetShaderResources(0, 3, resources);

	//POINT
	Game::GetInstance()->GetRenderSystem()->context->RSSetState(Game::GetInstance()->GetRenderSystem()->rastCullFront);                     //-//
	Game::GetInstance()->GetRenderSystem()->context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);                          //-//
	Game::GetInstance()->GetRenderSystem()->context->OMSetDepthStencilState(Game::GetInstance()->GetRenderSystem()->dsLightingGreater, 0); //-//

	UINT strides[] { 16 };
	UINT offsets[] { 0 };
	Game::GetInstance()->GetRenderSystem()->context->IASetVertexBuffers(0, 1, poiVertexBuffer.GetAddressOf(), strides, offsets); //-//
	Game::GetInstance()->GetRenderSystem()->context->IASetInputLayout(Game::GetInstance()->GetRenderSystem()->layoutLightingPoi); //-//
	Game::GetInstance()->GetRenderSystem()->context->IASetIndexBuffer(poiIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0); //-//

	Game::GetInstance()->GetRenderSystem()->context->VSSetShader(Game::GetInstance()->GetRenderSystem()->vsLightingPoi, nullptr, 0); //-//
	Game::GetInstance()->GetRenderSystem()->context->PSSetShader(Game::GetInstance()->GetRenderSystem()->psLightingPoi, nullptr, 0); //-//
	//Game::GetInstance()->GetRenderSystem()->context->GSSetShader(nullptr, nullptr, 0);

	Game::GetInstance()->GetRenderSystem()->context->VSSetConstantBuffers(0, 2, constBuffer); //-//
	Game::GetInstance()->GetRenderSystem()->context->PSSetConstantBuffers(0, 2, constBuffer); //-//

	Game::GetInstance()->GetRenderSystem()->context->DrawIndexed(poiIndices.size(), 0, 0); //-//
}

void PointLightComponent::PoiAddMesh(float sphereRadius, std::string objectFileName)
{
	this->sphereRadius = sphereRadius;

	Assimp::Importer importer;
	const aiScene* pScene = importer.ReadFile(objectFileName, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	if (!pScene) { return; }

	ProcessNode(pScene->mRootNode, pScene, sphereRadius);
}
void PointLightComponent::ProcessNode(aiNode* node, const aiScene* scene, float sphereRadius)
{
	for (UINT i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, sphereRadius);
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, sphereRadius);
	}
}
void PointLightComponent::ProcessMesh(aiMesh* mesh, const aiScene* scene, float sphereRadius)
{
	for (UINT i = 0; i < mesh->mNumVertices; i++) {
		poiPoints.push_back({ mesh->mVertices[i].x * sphereRadius, mesh->mVertices[i].y * sphereRadius, mesh->mVertices[i].z * sphereRadius, 1.0f });
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++) {		
		aiFace face = mesh->mFaces[i];
		for (INT j = face.mNumIndices - 1; j >= 0; j--)
		{
			poiIndices.push_back(face.mIndices[j]);
		}
	}
}