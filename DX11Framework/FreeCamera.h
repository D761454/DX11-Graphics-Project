#pragma once
#include "Camera.h"

class FreeCamera : public Camera
{
private:
	float m_angle;
	XMFLOAT3 m_right;

	XMVECTOR GetPositionV()const { return XMLoadFloat3(&m_position); }

	XMVECTOR GetUpV()const { return XMLoadFloat3(&m_up); }

	XMVECTOR GetLookV()const { return XMLoadFloat3(&m_at); }
	XMFLOAT3 GetLook()const { return m_at; } // rename for ease

	XMVECTOR GetRightV()const { return XMLoadFloat3(&m_right); }
	XMFLOAT3 GetRight()const { return m_right; }

public:
	FreeCamera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up, float windowWidth,
		float windowHeight, float nearDepth, float farDepth, HWND handle)
		: Camera(position, at, up, windowWidth,
			windowHeight, nearDepth, farDepth, handle) {
		XMStoreFloat3(&m_right, XMVector3Cross(GetUpV(), GetLookV()));
	}

	~FreeCamera();

	void Update(float deltaTime) override;

	void UpdateViewMatrix();

	void ForwardMove(float dist);
	void StrafeMove(float dist);
	void VertMove(float dist);

	void Pitch(float angle);
	void RotateY(float angle);
};

