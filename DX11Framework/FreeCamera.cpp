#include "FreeCamera.h"

FreeCamera::~FreeCamera() {

}

void FreeCamera::ForwardMove(float dist) {
	// multiple dist by look vector - add to pos
	XMVECTOR s = XMVectorReplicate(dist);
	XMVECTOR l = XMLoadFloat3(&m_at);
	XMVECTOR p = XMLoadFloat3(&m_position);
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, l, p));
}

void FreeCamera::StrafeMove(float dist) {
	// multiple dist by right vector - add to pos
	XMVECTOR s = XMVectorReplicate(dist);
	XMVECTOR r = XMLoadFloat3(&m_right);
	XMVECTOR p = XMLoadFloat3(&m_position);
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, r, p));
}

void FreeCamera::VertMove(float dist) {
	// multiple dist by up vector - add to pos
	XMVECTOR s = XMVectorReplicate(dist);
	XMVECTOR u = XMLoadFloat3(&m_up);
	XMVECTOR p = XMLoadFloat3(&m_position);
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, u, p));
}

void FreeCamera::Pitch(float angle) {
	// Rotate up and look vector around the right vector.
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_right), angle);
	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
	XMStoreFloat3(&m_at, XMVector3TransformNormal(XMLoadFloat3(&m_at), R));
}

void FreeCamera::RotateY(float angle) {
	// Rotate basis vectors about the world y-axis.
	XMMATRIX R = XMMatrixRotationY(angle);
	XMStoreFloat3(&m_right, XMVector3TransformNormal(XMLoadFloat3(&m_right), R));
	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
	XMStoreFloat3(&m_at, XMVector3TransformNormal(XMLoadFloat3(&m_at), R));
}

void FreeCamera::UpdateViewMatrix() {
	XMVECTOR R = XMLoadFloat3(&m_right);
	XMVECTOR U = XMLoadFloat3(&m_up);
	XMVECTOR L = XMLoadFloat3(&m_at);
	XMVECTOR P = XMLoadFloat3(&m_position);

	// orthonormalize r u l - make sure they are mutually orthogonal (right angle) to each other and unit length.
	// prevent errors where coord system is skewed

	L = XMVector3Normalize(L);
	
	U = XMVector3Normalize(XMVector3Cross(L, R));
	
	// u L already orthonormal, dont normalize cross product.
	R = XMVector3Cross(U, L);

	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));
	XMStoreFloat3(&m_right, R);
	XMStoreFloat3(&m_up, U);
	XMStoreFloat3(&m_at, L);

	// store as column major
	m_view(0, 0) = m_right.x;
	m_view(1, 0) = m_right.y;
	m_view(2, 0) = m_right.z;
	m_view(3, 0) = x;
	m_view(0, 1) = m_up.x;
	m_view(1, 1) = m_up.y;
	m_view(2, 1) = m_up.z;
	m_view(3, 1) = y;
	m_view(0, 2) = m_at.x;
	m_view(1, 2) = m_at.y;
	m_view(2, 2) = m_at.z;
	m_view(3, 2) = z;
	m_view(0, 3) = 0.0f;
	m_view(1, 3) = 0.0f;
	m_view(2, 3) = 0.0f;
	m_view(3, 3) = 1.0f;
}

void FreeCamera::Update(float deltaTime) {
	if (GetActiveWindow() == m_handle) {
		if (GetAsyncKeyState(VK_LSHIFT)) {
			if (GetAsyncKeyState(VK_SPACE) & 0x0001) {
				VertMove(-0.2f);
			}
		}
		else {
			if (GetAsyncKeyState(VK_SPACE) & 0x0001) {
				VertMove(0.2f);
			}
		}

		if (GetAsyncKeyState(87) & 0x0001) { // w
			ForwardMove(0.2f);
		}
		if (GetAsyncKeyState(65) & 0x0001) { // a
			StrafeMove(-0.2f);
		}
		if (GetAsyncKeyState(83) & 0x0001) { // s
			ForwardMove(-0.2f);
		}
		if (GetAsyncKeyState(68) & 0x0001) { // d
			StrafeMove(0.2f);
		}
	}

	UpdateViewMatrix();
}