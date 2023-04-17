#include "ModelComponent.h"

#include "Game.h"
#include "RenderSystem.h"
#include <WICTextureLoader.h>

#include "PointLightComponent.h"

ModelComponent::ModelComponent(std::string textureFileName)
{
	this->textureFileName = textureFileName;
}

void ModelComponent::Initialize()
{
	D3D11_BUFFER_DESC vertexBufDesc = {};
	vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufDesc.CPUAccessFlags = 0;
	vertexBufDesc.MiscFlags = 0;
	vertexBufDesc.StructureByteStride = 0;
	vertexBufDesc.ByteWidth = sizeof(DirectX::XMFLOAT4) * std::size(points);
	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = points.data();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	auto result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&vertexBufDesc, &vertexData, vertexBuffer.GetAddressOf());
	assert(SUCCEEDED(result));

	D3D11_BUFFER_DESC indexBufDesc = {};
	indexBufDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufDesc.CPUAccessFlags = 0;
	indexBufDesc.MiscFlags = 0;
	indexBufDesc.StructureByteStride = 0;
	indexBufDesc.ByteWidth = sizeof(int) * std::size(indices);
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices.data();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	result = Game::GetInstance()->GetRenderSystem()->device->CreateBuffer(&indexBufDesc, &indexData, indexBuffer.GetAddressOf());
	assert(SUCCEEDED(result));

	std::wstring textureName(textureFileName.begin(), textureFileName.end());
	result = DirectX::CreateWICTextureFromFile(
		Game::GetInstance()->GetRenderSystem()->device.Get(),
		Game::GetInstance()->GetRenderSystem()->context.Get(),
		textureName.c_str(),
		texture.GetAddressOf(),
		textureView.GetAddressOf()
	);
	assert(SUCCEEDED(result));
}

void ModelComponent::AddPlane(float planeSize)
{
	points = {
		Vector4(planeSize,   planeSize, 0.0f, 1.0f), Vector4(planeSize, planeSize, 0.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 0.0f),
		Vector4(-planeSize, -planeSize, 0.0f, 1.0f), Vector4(0.0f,       0.0f,    0.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 0.0f),
		Vector4(planeSize, -planeSize, 0.0f, 1.0f), Vector4(planeSize,   0.0f,    0.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 0.0f),
		Vector4(-planeSize,   planeSize, 0.0f, 1.0f), Vector4(0.0f,     planeSize, 0.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 0.0f)
	};
	indices = { 0, 1, 2, 1, 0, 3 };
}
void ModelComponent::AddMesh(float scaleRate, std::string objectFileName)
{
	Assimp::Importer importer;
	const aiScene* pScene = importer.ReadFile(objectFileName, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	if (!pScene) { return; }

	ProcessNode(pScene->mRootNode, pScene, scaleRate);
}
void ModelComponent::ProcessNode(aiNode* node, const aiScene* scene, float scaleRate)
{
	for (UINT i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, scaleRate);
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, scaleRate);
	}
}
void ModelComponent::ProcessMesh(aiMesh* mesh, const aiScene* scene, float scaleRate)
{
	for (UINT i = 0; i < mesh->mNumVertices; i++) {
		DirectX::SimpleMath::Vector4 textureCoordinate = {};

		if (mesh->mTextureCoords[0])
		{
			textureCoordinate.x = (float)mesh->mTextureCoords[0][i].x;
			textureCoordinate.y = (float)mesh->mTextureCoords[0][i].y;
			textureCoordinate.z = 0;
			textureCoordinate.w = 0;
		}

		DirectX::XMFLOAT4 normal;
		normal.x = mesh->mNormals[i].x;
		normal.y = mesh->mNormals[i].y;
		normal.z = mesh->mNormals[i].z;
		normal.w = 0;

		points.push_back({ mesh->mVertices[i].x * scaleRate, mesh->mVertices[i].y * scaleRate, mesh->mVertices[i].z * scaleRate, 1.0f });
		points.push_back(textureCoordinate);
		points.push_back(normal);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}
}