#pragma once
#include <windows.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <fstream>		
#include <vector>			
#include "Structures.h"

class Terrain
{
private:
	// used as arrays for Vertex/Index Buffer creation
	std::vector<SimpleVertex> m_vertices;
	std::vector<unsigned int> m_indices; // DXGI format of R32 rather than R16
	 
	ID3D11Buffer* m_vertexBuffer;
	ID3D11Buffer* m_indexBuffer;

	ID3D11ShaderResourceView* m_textures[5];
	ID3D11ShaderResourceView* m_blend;

	std::vector<float> m_heightMapData;

	TerrainInfo m_terrainInfo;

	XMFLOAT4X4 m_world;

public:
	Terrain();
	~Terrain();

	int GetIndexCount() { return m_indices.size(); }

	void GenGrid(float width, float depth, float columns, float rows);
	HRESULT BuildBuffers(ID3D11Device* device);

	void LoadHeightMap(int hmWidth, int hmHeight, std::string hmFileName);

	void SetPosition(XMFLOAT4X4 newWorld) { m_world = newWorld; }
	XMFLOAT4X4* getPosition() { return &m_world; }

	void Draw(ID3D11DeviceContext* deviceContext, ConstantBuffer* cbData);

	std::wstring GetFileName(UINT index) { std::wstring bbName(m_terrainInfo.m_layerMapFilenames[index].begin(), m_terrainInfo.m_layerMapFilenames[index].end()); return bbName; }
	std::wstring GetBlendName() { std::wstring bbName(m_terrainInfo.m_blendMapFilename.begin(), m_terrainInfo.m_blendMapFilename.end()); return bbName; }

	ID3D11ShaderResourceView** GetShaderResource(UINT index) { return &m_textures[index]; }
	ID3D11ShaderResourceView** GetShaderResourceBlend() { return &m_blend; }

	int GetAmtTex() { return sizeof(m_textures) / sizeof(m_textures[0]); }
};

