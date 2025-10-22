#pragma once

#include "Structures.h"

class GameObject
{
private:
	ID3D11ShaderResourceView* m_textureC = nullptr;
	ID3D11ShaderResourceView* m_textureS = nullptr;
	ID3D11ShaderResourceView* m_textureN = nullptr;
	MeshData m_meshData;
	XMFLOAT4X4 m_world;

public:
	GameObject();
	~GameObject();

	std::string m_objFile;
	UINT m_blender;
	UINT m_hasTex;
	UINT m_hasSpec;
	UINT m_hasNorm;
	std::wstring m_textureColor;
	std::wstring m_textureSpecular;
	std::wstring m_textureNormal;

	void SetMeshData(MeshData in) { m_meshData = in; }

	ID3D11ShaderResourceView** GetShaderResourceC() { return &m_textureC; }
	ID3D11ShaderResourceView** GetShaderResourceS() { return &m_textureS; }
	ID3D11ShaderResourceView** GetShaderResourceN() { return &m_textureN; }
	MeshData* GetMeshData() { return &m_meshData; }

	void SetPosition(XMFLOAT4X4 newWorld) { m_world = newWorld; }
	XMFLOAT4X4* getPosition() { return &m_world; }

	void Draw(ID3D11DeviceContext* deviceContext, ConstantBuffer* cbData);
};

