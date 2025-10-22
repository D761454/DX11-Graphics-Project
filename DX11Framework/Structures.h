#pragma once
#include <windows.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <string>

using namespace DirectX;

struct SimpleVertex
{
	XMFLOAT3 m_position;
	XMFLOAT3 m_normal;
	XMFLOAT2 m_texcoord;
	XMFLOAT3 m_tangent;

	bool operator<(const SimpleVertex other) const
	{
		return memcmp((void*)this, (void*)&other, sizeof(SimpleVertex)) > 0;
	};
};

struct MeshData
{
	ID3D11Buffer* m_vertexBuffer;
	ID3D11Buffer* m_indexBuffer;
	UINT m_vBStride;
	UINT m_vBOffset;
	UINT m_indexCount;

	void Release() {
		if(m_vertexBuffer) m_vertexBuffer->Release();
		if(m_indexBuffer) m_indexBuffer->Release();
	}
};

struct LightTypeInfo {
	std::string m_type;
	XMFLOAT4 m_color;
};

struct DirectionalLight {
	XMFLOAT4 m_ambientColor;
	XMFLOAT4 m_specularColor;
	XMFLOAT4 m_diffuseColor;
	XMFLOAT3 m_direction;
	float padding;
};

struct PointLight {
	XMFLOAT4 m_ambientColor;
	XMFLOAT4 m_specularColor;
	XMFLOAT4 m_diffuseColor;
	float m_maxDistance;
	XMFLOAT3 m_position;
	XMFLOAT3 m_attenuation; // control light intensity falloff
	float padding;
};

struct Fog {
	XMFLOAT4 m_color;
	float m_start;
	float m_range;
};

struct TerrainInfo
{
	std::string m_heightMapFilename;
	std::string m_layerMapFilenames[5];
	std::string m_blendMapFilename;
	float m_heightScale = 50.0f;
	UINT m_heightMapWidth;
	UINT m_heightMapHeight;
};

struct ConstantBuffer
{
	XMMATRIX Projection;
	XMMATRIX View;
	XMMATRIX World;
	XMFLOAT4 DiffuseMaterial;
	XMFLOAT4 AmbientMaterial;
	XMFLOAT4 SpecularMaterial;
	DirectionalLight DirectionalLight;
	PointLight PointLight;
	u_int HasTexture;
	XMFLOAT3 EyePosW;
	Fog FogW;
	UINT SpecMap;
	UINT NormMap;
	float SpecularPower;
	UINT FogEnabled;
};