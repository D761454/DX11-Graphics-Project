#pragma once

#include <time.h>
#include "DDSTextureLoader.h"
#include "OBJLoader.h"
#include "JSONLoad.h"
#include "FreeCamera.h"
#include "Terrain.h"
//#include <wrl.h>

//using Microsoft::WRL::ComPtr;

class DX11Framework
{
	int _WindowWidth = 1280;
	int _WindowHeight = 768;

	ID3D11DeviceContext* _immediateContext = nullptr;
	ID3D11Device* _device;
	IDXGIDevice* _dxgiDevice = nullptr;
	IDXGIFactory2* _dxgiFactory = nullptr;
	ID3D11RenderTargetView* _frameBufferView = nullptr;
	IDXGISwapChain1* _swapChain;
	D3D11_VIEWPORT _viewport;

	ID3D11RasterizerState* _fillState;
	ID3D11RasterizerState* _wireframeState;
	ID3D11RasterizerState* _cullnoneState;
	bool _wireframe = false;
	bool _fogToggle = false;

	ID3D11VertexShader* _vertexShader;
	ID3D11InputLayout* _inputLayout;
	ID3D11PixelShader* _pixelShader;

	ID3D11VertexShader* _skyboxVertexShader;
	ID3D11PixelShader* _skyboxPixelShader;

	ID3D11VertexShader* _billboardVertexShader;
	ID3D11PixelShader* _billboardPixelShader;

	ID3D11VertexShader* _terrainVertexShader;
	ID3D11PixelShader* _terrainPixelShader;

	ID3D11Buffer* _constantBuffer;

	ID3D11Buffer* _pyramidVertexBuffer;
	ID3D11Buffer* _pyramidIndexBuffer;
	ID3D11Buffer* _lineVertexBuffer;

	ID3D11Buffer* _billboardVertexBuffer;
	ID3D11Buffer* _billboardIndexBuffer;
	ID3D11ShaderResourceView* _billboardTexture;

	HWND _windowHandle;

	XMFLOAT4X4 _cubes[2];
	XMFLOAT4X4 _pyramids[1];
	XMFLOAT4X4 _Line;

	XMFLOAT4X4 _skybox;

	XMFLOAT4X4 _asteroids[50];
	XMFLOAT4X4 _trees[20];

	XMFLOAT4 _diffuseMaterial;
	XMFLOAT4 _ambientMaterial;
	XMFLOAT4 _specularMaterial;
	DirectionalLight _directionalLight;
	PointLight _pointLight;
	Fog _fog;

	ConstantBuffer _cbData;

	ID3D11Texture2D* _depthStencilBuffer;
	ID3D11DepthStencilView* _depthStencilView;

	ID3D11DepthStencilState* _depthStencilSkybox;

	ID3D11SamplerState* _bilinearSamplerState;

	std::vector<GameObject> _gameObjects;
	std::vector<LightTypeInfo> _lightsInfo;

	JSONLoad _jsonLoader;

	std::vector<Camera*> _cameras;
	int currentCam = 0;

	ID3D11BlendState* _blendState;

	Terrain* _terrain = nullptr;
public:
	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);
	HRESULT CreateWindowHandle(HINSTANCE hInstance, int nCmdShow);
	HRESULT CreateD3DDevice();
	HRESULT CreateSwapChainAndFrameBuffer();
	HRESULT InitShadersAndInputLayout();
	HRESULT InitVertexIndexBuffers();
	HRESULT InitPipelineVariables();
	HRESULT InitRunTimeData();
	~DX11Framework();
	void Update();
	void Draw();

	void SetBuffers(
		ID3D11Buffer* vBuffer,
		ID3D11Buffer* pBuffer,
		UINT* stride,
		UINT* offset,
		DXGI_FORMAT format);

	void SetShaders(
		ID3D11VertexShader* vShader,
		ID3D11PixelShader* pShader);

	void SetShaderResources(
		ID3D11ShaderResourceView** C,
		ID3D11ShaderResourceView** S,
		ID3D11ShaderResourceView** N
	);

	void SetRS(ID3D11RasterizerState* state);

	void DrawObjects(
		UINT indices,
		XMFLOAT4X4* position,
		D3D11_MAPPED_SUBRESOURCE* mSubRes);

	void MouseDetection(HWND hWnd);
	void OnMouseMove(int x, int y);
};