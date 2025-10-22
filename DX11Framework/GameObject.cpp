#include "GameObject.h"

GameObject::GameObject() {

}

GameObject::~GameObject() {
	m_meshData.Release();
	if (m_textureC) m_textureC->Release();
	if (m_textureS) m_textureS->Release();
	if (m_textureN) m_textureN->Release();
}

/// <summary>
/// Takes in all required parameters to complete all draw functionality within the game object
/// </summary>
/// <param name="deviceContext"></param>
/// <param name="cbData"></param>
void GameObject::Draw(ID3D11DeviceContext* deviceContext, ConstantBuffer* cbData) {
	deviceContext->IASetVertexBuffers(0, 1, &GetMeshData()->m_vertexBuffer, &GetMeshData()->m_vBStride, &GetMeshData()->m_vBOffset);
	deviceContext->IASetIndexBuffer(GetMeshData()->m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

	cbData->HasTexture = m_hasTex;
	cbData->SpecMap = m_hasSpec;
	cbData->NormMap = m_hasNorm;
}