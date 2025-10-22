#include "Terrain.h"

Terrain::Terrain() {
	m_terrainInfo.m_layerMapFilenames[0] = "Textures\\lightdirt.dds";
	m_terrainInfo.m_layerMapFilenames[1] = "Textures\\DarkDirt.dds";
	m_terrainInfo.m_layerMapFilenames[2] = "Textures\\grass.dds";
	m_terrainInfo.m_layerMapFilenames[3] = "Textures\\stone.dds";
	m_terrainInfo.m_layerMapFilenames[4] = "Textures\\snow.dds";
	m_terrainInfo.m_blendMapFilename = "Textures\\Blend.dds";
}

Terrain::~Terrain() {
	for (UINT i = 0; i < (sizeof(m_textures) / sizeof(m_textures[0])); i++) {
		m_textures[i]->Release();
	}

	if (m_blend) m_blend->Release();

	if (m_vertexBuffer) m_vertexBuffer->Release();
	if (m_indexBuffer) m_indexBuffer->Release();
}

/// <summary>
/// generates a grid of triangles based on passed in variables, storing in vectors
/// </summary>
/// <param name="width"></param>
/// <param name="depth"></param>
/// <param name="columns"></param>
/// <param name="rows"></param>
void Terrain::GenGrid(float width, float depth, float columns, float rows) {
	float halfWidth = width * 0.5f;
	float halfdepth = depth * 0.5f;

	float dx = width / (columns - 1); // x cell size
	float dz = depth / (rows - 1); // z cell size

	float du = 1.0f / (columns - 1);
	float dv = 1.0f / (rows - 1);

	for (UINT i = 0; i < rows; ++i) {
		float z = halfdepth - (i * dz);
		for (UINT j = 0; j < columns; ++j) {
			float x = -halfWidth + (j * dx);
			// pos, normal, texcoord
			SimpleVertex vertex;
			vertex.m_position = XMFLOAT3(x, 0.0f, z);
			vertex.m_texcoord = XMFLOAT2(j * du, i * dv);
			m_vertices.push_back(vertex);
		}
	}

	for (UINT i = 0; i < m_vertices.size(); ++i) {
		m_vertices[i].m_position.y = m_heightMapData[i];
	}

	// this is likely why holes appear
	for (UINT i = 0; i < rows - 1; ++i) {
		for (UINT j = 0; j < columns - 1; ++j) {
			// top tri
			m_indices.push_back(i * columns + j);
			m_indices.push_back(i * columns + j + 1);
			m_indices.push_back((i + 1) * columns + j);

			// bottom tri
			m_indices.push_back(i * columns + j + 1);
			m_indices.push_back((i + 1) * columns + j + 1);
			m_indices.push_back((i + 1) * columns + j);
		}
	}

	for (UINT i = 0; i < m_indices.size(); i+=3) {
		XMVECTOR vector1 = XMVectorSet(m_vertices[m_indices[i]].m_position.x, m_vertices[m_indices[i]].m_position.y, m_vertices[m_indices[i]].m_position.z, 0.0f);
		XMVECTOR vector2 = XMVectorSet(m_vertices[m_indices[i+1]].m_position.x, m_vertices[m_indices[i + 1]].m_position.y, m_vertices[m_indices[i + 1]].m_position.z, 0.0f);
		XMVECTOR vector3 = XMVectorSet(m_vertices[m_indices[i + 2]].m_position.x, m_vertices[m_indices[i + 2]].m_position.y, m_vertices[m_indices[i + 2]].m_position.z, 0.0f);

		// A1, B2, C3
		XMVECTOR lineAB = XMVectorSubtract(vector2, vector1);
		XMVECTOR lineAC = XMVectorSubtract(vector3, vector1);

		XMVECTOR lineBA = XMVectorSubtract(vector1, vector2);
		XMVECTOR lineBC = XMVectorSubtract(vector3, vector2);

		XMVECTOR lineCA = XMVectorSubtract(vector1, vector3);
		XMVECTOR lineCB = XMVectorSubtract(vector2, vector3);

		XMVECTOR crossA = XMVector3Normalize(XMVector3Cross(lineAB, lineAC));
		XMVECTOR crossB = XMVector3Normalize(XMVector3Cross(lineBA, lineBC));
		XMVECTOR crossC = XMVector3Normalize(XMVector3Cross(lineCA, lineCB));

		XMStoreFloat3(&m_vertices[m_indices[i]].m_normal, crossA);
		XMStoreFloat3(&m_vertices[m_indices[i+1]].m_normal, crossB);
		XMStoreFloat3(&m_vertices[m_indices[i+2]].m_normal, crossC);
	}
}

/// <summary>
/// Builds Vertex and Index Buffers
/// </summary>
/// <param name="device"></param>
/// <returns></returns>
HRESULT Terrain::BuildBuffers(ID3D11Device* device) {
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(SimpleVertex) * m_vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &m_vertices[0];
	hr = device->CreateBuffer(&vbd, &vinitData, &m_vertexBuffer);
	if (FAILED(hr)) return hr;

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(unsigned int) * m_indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &m_indices[0];
	hr = device->CreateBuffer(&ibd, &iinitData, &m_indexBuffer);
	if (FAILED(hr)) return hr;

	return S_OK;
}

/// <summary>
/// loads height map data into a vector, call before generating flat grid
/// </summary>
/// <param name="hmWidth"></param>
/// <param name="hmHeight"></param>
/// <param name="hmFileName"></param>
void Terrain::LoadHeightMap(int hmWidth, int hmHeight, std::string hmFileName) {
	m_terrainInfo.m_heightMapWidth = hmWidth;
	m_terrainInfo.m_heightMapHeight = hmHeight;
	m_terrainInfo.m_heightMapFilename = hmFileName;

	// height per vertex
	std::vector<unsigned char> in(hmWidth * hmHeight);

	std::ifstream inFile;
	inFile.open(hmFileName.c_str(), std::ios_base::binary);

	if (inFile) {
		// read raw bytes
		inFile.read((char*)&in[0], (std::streamsize)in.size());

		inFile.close();
	}

	m_heightMapData.resize(sizeof(float) * hmHeight * hmWidth);

	// copy array data and scale it
	for (UINT i = 0; i < hmHeight * hmWidth; ++i) {
		m_heightMapData[i] = (in[i] / 255.0f) * m_terrainInfo.m_heightScale;
	}
}

/// <summary>
/// set shader resource, buffers and cb Data for textures.
/// </summary>
/// <param name="deviceContext"></param>
/// <param name="cbData"></param>
void Terrain::Draw(ID3D11DeviceContext* deviceContext, ConstantBuffer* cbData) {
	UINT stride = { sizeof(SimpleVertex) };
	UINT offset = 0;

	for (UINT i = 0; i < sizeof(m_textures) / sizeof(m_textures[0]); i++) {
		deviceContext->PSSetShaderResources(i, 1, &m_textures[i]);
	}
	deviceContext->PSSetShaderResources(sizeof(m_textures) / sizeof(m_textures[0]), 1, &m_blend);

	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	cbData->HasTexture = 1;
	cbData->SpecMap = 0;
	cbData->NormMap = 0;
}