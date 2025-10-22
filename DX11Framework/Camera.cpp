#include "Camera.h"

Camera::Camera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up, float windowWidth,
	float windowHeight, float nearDepth, float farDepth, HWND handle) {
	m_position = position;
	m_at = at;
	m_up = up;
	m_windowWidth = windowWidth;
	m_windowHeight = windowHeight;
	m_nearDepth = nearDepth;
	m_farDepth = farDepth;
	m_viewport = { 0.0f, 0.0f, m_windowWidth, m_windowHeight, nearDepth, farDepth };
	m_handle = handle;
	UpdateProjection();
}

Camera::~Camera() {
	
}

void Camera::Update(float deltaTime) {
	XMStoreFloat4x4(&m_view, XMMatrixLookToLH(XMLoadFloat3(&m_position), XMLoadFloat3(&m_at), XMLoadFloat3(&m_up)));
}

XMFLOAT4X4* Camera::GetViewProjection() {
	XMStoreFloat4x4(&m_viewProj, XMMatrixMultiply(XMLoadFloat4x4(&m_view), XMLoadFloat4x4(&m_projection)));
	return &m_viewProj;
}

void Camera::UpdateProjection() {
	float aspect = m_viewport.Width / m_viewport.Height;

	XMMATRIX perspective = XMMatrixPerspectiveFovLH(XMConvertToRadians(90), aspect, 0.01f, 1000.0f);
	XMStoreFloat4x4(&m_projection, perspective);
}