#pragma once
#include <DirectXMath.h>
#include <d3d11_1.h>

using namespace DirectX;

class Camera
{
protected:
	XMFLOAT3 m_position;
	XMFLOAT3 m_at;
	XMFLOAT3 m_up;

	float m_windowWidth;
	float m_windowHeight;
	D3D11_VIEWPORT m_viewport;
	float m_nearDepth;
	float m_farDepth;

	XMFLOAT4X4 m_view;
	XMFLOAT4X4 m_projection;
	XMFLOAT4X4 m_viewProj;

	HWND m_handle;

public:
	Camera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up, float windowWidth, 
		float windowHeight, float nearDepth, float farDepth, HWND handle);
	~Camera();

	virtual void Update(float deltaTime);

	void SetPosition(XMFLOAT3 position) { m_position = position; } // used to keep position same as cam we just swapped from

	XMFLOAT3 GetPosition() { return m_position; }
	XMFLOAT3 GetAt() { return m_at; }
	XMFLOAT3 GetUp() { return m_up; }

	XMFLOAT4X4* GetView() { return &m_view; }

	D3D11_VIEWPORT* GetViewport() { return &m_viewport; }

	void UpdateProjection();
	XMFLOAT4X4* GetProjection() { return &m_projection; }

	XMFLOAT4X4* GetViewProjection();
};

