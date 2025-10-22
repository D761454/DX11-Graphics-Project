#include "DX11Framework.h"
#include <string>

//#define RETURNFAIL(x) if(FAILED(x)) return x;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break; 

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

HRESULT DX11Framework::Initialise(HINSTANCE hInstance, int nShowCmd)
{
    HRESULT hr = S_OK; // check function success

    hr = CreateWindowHandle(hInstance, nShowCmd);
    if (FAILED(hr)) return E_FAIL;

    hr = CreateD3DDevice();
    if (FAILED(hr)) return E_FAIL;

    hr = CreateSwapChainAndFrameBuffer();
    if (FAILED(hr)) return E_FAIL;

    hr = InitShadersAndInputLayout();
    if (FAILED(hr)) return E_FAIL;

    hr = InitVertexIndexBuffers();
    if (FAILED(hr)) return E_FAIL;

    hr = InitPipelineVariables();
    if (FAILED(hr)) return E_FAIL;

    hr = InitRunTimeData();
    if (FAILED(hr)) return E_FAIL;

    return hr;
}

HRESULT DX11Framework::CreateWindowHandle(HINSTANCE hInstance, int nCmdShow)
{
    const wchar_t* windowName  = L"DX11Framework";

    WNDCLASSW wndClass; // defined as function that recieves messages from OS
    wndClass.style = 0;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);;
    wndClass.hbrBackground = 0;
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = windowName;

    RegisterClassW(&wndClass);

    _windowHandle = CreateWindowExW(0, windowName, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 
        _WindowWidth, _WindowHeight, nullptr, nullptr, hInstance, nullptr);

    return S_OK;
}

HRESULT DX11Framework::CreateD3DDevice()
{
    // creates and populaties device and device context
    // device - create resources, context - current state e.g. currently bound resources
    HRESULT hr = S_OK;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, // work for DX 10 and 11
    };

    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;

    DWORD createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // spit out errors
#endif
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    hr = baseDevice->QueryInterface(__uuidof(ID3D11Device), reinterpret_cast<void**>(&_device));
    hr = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext), reinterpret_cast<void**>(&_immediateContext));

    baseDevice->Release();
    baseDeviceContext->Release();

    ///////////////////////////////////////////////////////////////////////////////////////////////

    hr = _device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&_dxgiDevice));
    if (FAILED(hr)) return hr;

    IDXGIAdapter* dxgiAdapter;
    hr = _dxgiDevice->GetAdapter(&dxgiAdapter);
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&_dxgiFactory));
    dxgiAdapter->Release();

    return S_OK;
}

HRESULT DX11Framework::CreateSwapChainAndFrameBuffer()
{
    // initialises render target - back buffer
    // this one makes use of double buffering
    // buffers initialised to screen size
    HRESULT hr = S_OK;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; // Defer to WindowWidth
    swapChainDesc.Height = 0; // Defer to WindowHeight
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //FLIP* modes don't support sRGB backbuffer
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    hr = _dxgiFactory->CreateSwapChainForHwnd(_device, _windowHandle, &swapChainDesc, nullptr, nullptr, &_swapChain);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Texture2D* frameBuffer = nullptr;

    hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));
    if (FAILED(hr)) return hr;

    D3D11_RENDER_TARGET_VIEW_DESC framebufferDesc = {};
    framebufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //sRGB render target enables hardware gamma correction
    framebufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    hr = _device->CreateRenderTargetView(frameBuffer, &framebufferDesc, &_frameBufferView);

    // copy frame buffer desc - account for back buffer being slightly smaller than defined window size
    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    frameBuffer->GetDesc(&depthBufferDesc);

    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // depth 24 bit float - usually 32 bit - normalised 0-1 - final 8 bits - stencil 8 bit unassigned int - whether pixel has been written to.
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    _device->CreateTexture2D(&depthBufferDesc, nullptr, &_depthStencilBuffer);
    _device->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);

    frameBuffer->Release();

    return hr;
}

HRESULT DX11Framework::InitShadersAndInputLayout()
{
    // compile at run time and define part of data they recieve
    HRESULT hr = S_OK;
    ID3DBlob* errorBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
    
    ID3DBlob* vsBlob;
    // compiles shaders at runtime
    hr =  D3DCompileFromFile(L"SimpleShaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_main", "vs_5_0", dwShaderFlags, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_vertexShader);

    if (FAILED(hr)) return hr;
    
    // VVV IMPORTANT
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 }
    };

    hr = _device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_inputLayout);
    if (FAILED(hr)) return hr;

    ID3DBlob* psBlob;

    hr = D3DCompileFromFile(L"SimpleShaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_main", "ps_5_0", dwShaderFlags, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_pixelShader);

    vsBlob->Release();
    psBlob->Release();

    ////////////////////////////////////////////////////////////////////////////////////////////////

    hr = D3DCompileFromFile(L"SkyboxShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_main", "vs_5_0", dwShaderFlags, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_skyboxVertexShader);

    if (FAILED(hr)) return hr;

    hr = D3DCompileFromFile(L"SkyboxShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_main", "ps_5_0", dwShaderFlags, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_skyboxPixelShader);

    vsBlob->Release();
    psBlob->Release();

    ////////////////////////////////////////////////////////////////////////////////////////////////

    hr = D3DCompileFromFile(L"BillboardShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_main", "vs_5_0", dwShaderFlags, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_billboardVertexShader);

    if (FAILED(hr)) return hr;

    hr = D3DCompileFromFile(L"BillboardShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_main", "ps_5_0", dwShaderFlags, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_billboardPixelShader);

    vsBlob->Release();
    psBlob->Release();

    ////////////////////////////////////////////////////////////////////////////////////////////////

    hr = D3DCompileFromFile(L"TerrainShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_main", "vs_5_0", dwShaderFlags, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_terrainVertexShader);

    if (FAILED(hr)) return hr;

    hr = D3DCompileFromFile(L"TerrainShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_main", "ps_5_0", dwShaderFlags, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_terrainPixelShader);

    vsBlob->Release();
    psBlob->Release();

    return hr;
}

HRESULT DX11Framework::InitVertexIndexBuffers()
{
    HRESULT hr = S_OK;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    SimpleVertex pyramidVertexData[] =  // maybe modify to account for sides being at angle
    {
        //Position                     //normal             
        { XMFLOAT3(-1.00f,  -1.00f, 1.0f), XMFLOAT3(0.0f,  -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)}, // bottom
        { XMFLOAT3(1.00f,  -1.00f, 1.0f),  XMFLOAT3(0.0f,  -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f)},
        { XMFLOAT3(-1.00f, -1.00f, -1.0f), XMFLOAT3(0.0f,  -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f)},
        { XMFLOAT3(1.00f, -1.00f, -1.0f),  XMFLOAT3(0.0f,  -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f)},

        { XMFLOAT3(-1.00f,  -1.00f, -1.0f), XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)}, // front
        { XMFLOAT3(1.00f,  -1.00f, -1.0f),  XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)},
        { XMFLOAT3(0.0f, 1.00f, 0.0f), XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT2(0.5f, 1.0f)},

        { XMFLOAT3(-1.00f,  -1.00f, 1.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)}, // back
        { XMFLOAT3(1.00f,  -1.00f, 1.0f),  XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)},
        { XMFLOAT3(0.0f, 1.00f, 0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(0.5f, 1.0f)},

        { XMFLOAT3(-1.00f,  -1.00f, 1.0f), XMFLOAT3(-1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)}, // left
        { XMFLOAT3(-1.00f,  -1.00f, -1.0f),  XMFLOAT3(-1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f)},
        { XMFLOAT3(0.0f, 1.00f, 0.0f), XMFLOAT3(-1.0f,  0.0f, 0.0f), XMFLOAT2(0.5f, 1.0f)},

        { XMFLOAT3(1.00f,  -1.00f, 1.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f)}, // right
        { XMFLOAT3(1.00f,  -1.00f, -1.0f),  XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)},
        { XMFLOAT3(0.0f, 1.00f, 0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(0.5f, 1.0f)},
    };

    D3D11_BUFFER_DESC pyramideVertexBufferDesc = {};
    pyramideVertexBufferDesc.ByteWidth = sizeof(pyramidVertexData);
    pyramideVertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // data wont change
    pyramideVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA pyramidvertexData = { pyramidVertexData };
    // ^^^ DX API pointing to some memory - can handle 2-dimensional data - use to identify data being stored

    hr = _device->CreateBuffer(&pyramideVertexBufferDesc, &pyramidvertexData, &_pyramidVertexBuffer);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    WORD pyramidIndexData[] = // WORD - unsigned short
    {
        //Indices
        0, 2, 3, // bottom
        0, 3, 1,
        4, 6, 5, // front
        8, 9, 7, // back
        10, 12, 11, // left
        14, 15, 13
    };

    D3D11_BUFFER_DESC pyramidIndexBufferDesc = {};
    pyramidIndexBufferDesc.ByteWidth = sizeof(pyramidIndexData); // the amount of indices in bytes, use for size of buffer in bytes
    pyramidIndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    pyramidIndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA pyramidindexData = { pyramidIndexData };

    hr = _device->CreateBuffer(&pyramidIndexBufferDesc, &pyramidindexData, &_pyramidIndexBuffer);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    SimpleVertex billboardVertexData[] =  // maybe modify to account for sides being at angle
    {
        //Position                     //normal             
        { XMFLOAT3(-1.00f,  -1.00f, 0.0f), XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f)},
        { XMFLOAT3(1.00f,  -1.00f, 0.0f),  XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f)},
        { XMFLOAT3(-1.00f, 1.00f, 0.0f), XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
        { XMFLOAT3(1.00f, 1.00f, 0.0f),  XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)},
    };

    D3D11_BUFFER_DESC billboardVertexBufferDesc = {};
    billboardVertexBufferDesc.ByteWidth = sizeof(billboardVertexData);
    billboardVertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // data wont change
    billboardVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA billboardvData = { billboardVertexData };
    // ^^^ DX API pointing to some memory - can handle 2-dimensional data - use to identify data being stored

    hr = _device->CreateBuffer(&billboardVertexBufferDesc, &billboardvData, &_billboardVertexBuffer);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    WORD billboardIndexData[] = // WORD - unsigned short
    {
        //Indices
        0, 2, 3,
        0, 3, 1
    };

    D3D11_BUFFER_DESC billboardIndexBufferDesc = {};
    billboardIndexBufferDesc.ByteWidth = sizeof(billboardIndexData); // the amount of indices in bytes, use for size of buffer in bytes
    billboardIndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    billboardIndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA billboardiData = { billboardIndexData };

    hr = _device->CreateBuffer(&billboardIndexBufferDesc, &billboardiData, &_billboardIndexBuffer);
    if (FAILED(hr)) return hr;


    ///////////////////////////////////////////////////////////////////////////////////////////////

    //SimpleVertex lineVertexData[] = {
    //    {XMFLOAT3(0,3,0), XMFLOAT4(1,1,1,1)},
    //    {XMFLOAT3(0,4,0), XMFLOAT4(1,1,1,1)},
    //};

    //D3D11_BUFFER_DESC lineVertexBufferDesc = {};
    //lineVertexBufferDesc.ByteWidth = sizeof(lineVertexData);
    //lineVertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // data wont change
    //lineVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    //D3D11_SUBRESOURCE_DATA linevertexData = { lineVertexData };
    //// ^^^ DX API pointing to some memory - can handle 2-dimensional data - use to identify data being stored

    //hr = _device->CreateBuffer(&lineVertexBufferDesc, &linevertexData, &_lineVertexBuffer);
    //if (FAILED(hr)) return hr;

    _terrain = new Terrain();

    _terrain->LoadHeightMap(513, 513, "RAW Files\\Heightmap 513x513.raw");
    _terrain->GenGrid(513, 513, 513, 513);
    _terrain->BuildBuffers(_device);

    return S_OK;
}

HRESULT DX11Framework::InitPipelineVariables()
{
    // creates constant buffer and sets def options
    HRESULT hr = S_OK;

    //Input Assembler
    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _immediateContext->IASetInputLayout(_inputLayout);

    //Rasterizer fill
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    hr = _device->CreateRasterizerState(&rasterizerDesc, &_fillState);
    if (FAILED(hr)) return hr;

    _immediateContext->RSSetState(_fillState);

    //Rasterizer wireframe
    D3D11_RASTERIZER_DESC wireframeDesc = {};
    wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireframeDesc.CullMode = D3D11_CULL_NONE;

    hr = _device->CreateRasterizerState(&wireframeDesc, &_wireframeState);
    if (FAILED(hr)) return hr;

    //Rasterizer cull none
    D3D11_RASTERIZER_DESC cullnoneDesc = {};
    cullnoneDesc.FillMode = D3D11_FILL_SOLID;
    cullnoneDesc.CullMode = D3D11_CULL_NONE;

    hr = _device->CreateRasterizerState(&cullnoneDesc, &_cullnoneState);
    if (FAILED(hr)) return hr;

    D3D11_DEPTH_STENCIL_DESC dsDescSkybox = { };
    dsDescSkybox.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    dsDescSkybox.DepthEnable = true;
    dsDescSkybox.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

    _device->CreateDepthStencilState(&dsDescSkybox, &_depthStencilSkybox);

    //Viewport Values
    _cameras.push_back(new Camera(XMFLOAT3(0, 30.0f, -10.0f), XMFLOAT3(0, 0, 1.0f), XMFLOAT3(0, 1, 0),
        _WindowWidth, _WindowHeight, 0, 1, _windowHandle));
    _cameras.push_back(new Camera(XMFLOAT3(-5.0f, 30.0f, -10.0f), XMFLOAT3(1.0f, 0, 1.0f), XMFLOAT3(0, 1, 0),
        _WindowWidth, _WindowHeight, 0, 1, _windowHandle));
    _cameras.push_back(new Camera(XMFLOAT3(5.0f, 30.0f, -10.0f), XMFLOAT3(-1.0f, 0, 1.0f), XMFLOAT3(0, 1, 0),
        _WindowWidth, _WindowHeight, 0, 1, _windowHandle));
    _cameras.push_back(new Camera(XMFLOAT3(0, 35.0f, -10.0f), XMFLOAT3(0, -1.0f, 1.0f), XMFLOAT3(0, 1, 0),
        _WindowWidth, _WindowHeight, 0, 1, _windowHandle));
    _cameras.push_back(new Camera(XMFLOAT3(0, 40.0f, 10.0f), XMFLOAT3(0, 0.0f, -1.0f), XMFLOAT3(0, 1, 0),
        _WindowWidth, _WindowHeight, 0, 1, _windowHandle));
    _cameras.push_back(new FreeCamera(XMFLOAT3(0, 30.0f, -10.0f), XMFLOAT3(0, 0, 1), XMFLOAT3(0, 1, 0),
        _WindowWidth, _WindowHeight, 0, 1, _windowHandle));
    
    _immediateContext->RSSetViewports(1, _cameras[0]->GetViewport());

    //Constant Buffer
    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = sizeof(ConstantBuffer);
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = _device->CreateBuffer(&constantBufferDesc, nullptr, &_constantBuffer);
    if (FAILED(hr)) { return hr; }

    _immediateContext->VSSetConstantBuffers(0, 1, &_constantBuffer);
    _immediateContext->PSSetConstantBuffers(0, 1, &_constantBuffer);

    ////////////////////////////

    D3D11_SAMPLER_DESC bilinearSamplerDesc = {};
    bilinearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    bilinearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    bilinearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    bilinearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // minification, magnification, mip levels (mip makes it bilinear)
    bilinearSamplerDesc.MaxLOD = 1;
    bilinearSamplerDesc.MinLOD = 0;

    hr = _device->CreateSamplerState(&bilinearSamplerDesc, &_bilinearSamplerState);
    if (FAILED(hr)) { return hr; }

    _immediateContext->PSSetSamplers(0, 1, &_bilinearSamplerState);

    ////////////////////////////

    D3D11_BLEND_DESC blendDesc = {};

    D3D11_RENDER_TARGET_BLEND_DESC rtbd = {};
    rtbd.BlendEnable = true;
    rtbd.SrcBlend = D3D11_BLEND_SRC_COLOR;
    rtbd.DestBlend = D3D11_BLEND_BLEND_FACTOR;
    rtbd.BlendOp = D3D11_BLEND_OP_ADD;
    rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
    rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
    rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.RenderTarget[0] = rtbd;

    hr = _device->CreateBlendState(&blendDesc, &_blendState);
    if (FAILED(hr)) { return hr; }

    return S_OK;
}

HRESULT DX11Framework::InitRunTimeData()
{
    HRESULT hr = S_OK;

    _ambientMaterial = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    _diffuseMaterial = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    _specularMaterial = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    // directional
    _directionalLight.m_direction = XMFLOAT3(0.0f, 0.0f, -0.5f);

    // point
    _pointLight.m_maxDistance = 100.0f;
    _pointLight.m_position = XMFLOAT3(0.0f, 40.0f, 2.0f);
    _pointLight.m_attenuation = XMFLOAT3(1.0f, 1.0f, 1.0f);

    _fog.m_color = XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f);
    _fog.m_range = 100.0f;
    _fog.m_start = 20.0f;

    _cbData.FogW = _fog;

    _jsonLoader.JSONSample("test.json", _gameObjects, _lightsInfo);

    // set light data via json
    for (int i = 0; i < _lightsInfo.size(); i++)
    {
        LightTypeInfo current = _lightsInfo[i];
        if (current.m_type == "ambient") {
            _directionalLight.m_ambientColor = current.m_color;
            _pointLight.m_ambientColor = current.m_color;
        }
        else if (current.m_type == "diffuse") {
            _directionalLight.m_diffuseColor = current.m_color;
            _pointLight.m_diffuseColor = current.m_color;
        }
        else if (current.m_type == "specular") {
            _directionalLight.m_specularColor = current.m_color;
            _pointLight.m_specularColor = current.m_color;
        }
    }

    for (int i = 0; i < _gameObjects.size(); i++)
    {
        switch (_gameObjects[i].m_blender) {
        case 0:
            _gameObjects[i].SetMeshData(OBJLoader::Load((_gameObjects[i].m_objFile).c_str(), _device));
            break;
        case 1:
            _gameObjects[i].SetMeshData(OBJLoader::Load((_gameObjects[i].m_objFile).c_str(), _device, false));
            break;
        default:
            _gameObjects[i].SetMeshData(OBJLoader::Load((_gameObjects[i].m_objFile).c_str(), _device));
            break;
        }

        if(_gameObjects[i].m_hasTex == 1) hr = CreateDDSTextureFromFile(_device, _gameObjects[i].m_textureColor.c_str(), nullptr, _gameObjects[i].GetShaderResourceC()); if (FAILED(hr)) { return hr; }

        if (_gameObjects[i].m_hasSpec == 1) hr = CreateDDSTextureFromFile(_device, _gameObjects[i].m_textureSpecular.c_str(), nullptr, _gameObjects[i].GetShaderResourceS()); if (FAILED(hr)) { return hr; }

        if (_gameObjects[i].m_hasNorm == 1) hr = CreateDDSTextureFromFile(_device, _gameObjects[i].m_textureNormal.c_str(), nullptr, _gameObjects[i].GetShaderResourceN()); if (FAILED(hr)) { return hr; }
    }

    std::string name = "Textures\\Pine_Tree.dds";
    std::wstring bbName(name.begin(), name.end());
    hr = CreateDDSTextureFromFile(_device, bbName.c_str(), nullptr, &_billboardTexture); if (FAILED(hr)) { return hr; }

    for (UINT i = 0; i < _terrain->GetAmtTex(); i++) {
        hr = CreateDDSTextureFromFile(_device, _terrain->GetFileName(i).c_str(), nullptr, _terrain->GetShaderResource(i)); if (FAILED(hr)) { return hr; }
    }

    hr = CreateDDSTextureFromFile(_device, _terrain->GetBlendName().c_str(), nullptr, _terrain->GetShaderResourceBlend()); if (FAILED(hr)) { return hr; }

    //World - asteroids
    srand(time(0));

    for (UINT i = 0; i < sizeof(_asteroids) / sizeof(_asteroids[0]); i++)
    {
        if (i < (sizeof(_asteroids) / sizeof(_asteroids[0])) / 4) {
            XMMATRIX position = XMMatrixScaling(0.15f, 0.15f, 0.15f) *
                XMMatrixTranslation(
                    ((rand() % 500) / 100) + 7.0f,     // x
                    (((rand() % 200) / 100) - 1.0f) + 30,     // y
                    ((rand() % 1000) / 100) - 5.0f)   // z
            * XMMatrixRotationY(rand() % 90);
            XMStoreFloat4x4(&_asteroids[i], position);
        }
        else if (i < (sizeof(_asteroids) / sizeof(_asteroids[0])) / 2) {
            XMMATRIX position = XMMatrixScaling(0.15f, 0.15f, 0.15f) *
                XMMatrixTranslation(
                    ((rand() % 500) / 100) - 14.0f,     // x
                    (((rand() % 200) / 100) - 1.0f) + 30,     // y
                    ((rand() % 1000) / 100) - 5.0f)   // z
                * XMMatrixRotationY(rand() % 90);
            XMStoreFloat4x4(&_asteroids[i], position);
        }
        else if (i < ((sizeof(_asteroids) / sizeof(_asteroids[0])) / 4) * 3) {
            XMMATRIX position = XMMatrixScaling(0.15f, 0.15f, 0.15f) *
                XMMatrixTranslation(
                    ((rand() % 1000) / 100) - 5.0f,     // x
                    (((rand() % 200) / 100) - 1.0f) + 30,     // y
                    ((rand() % 500) / 100) + 7.0f)   // z
                * XMMatrixRotationY(rand() % 90);
            XMStoreFloat4x4(&_asteroids[i], position);
        }
        else {
            XMMATRIX position = XMMatrixScaling(0.15f, 0.15f, 0.15f) *
                XMMatrixTranslation(
                    ((rand() % 1000) / 100) - 5.0f,     // x
                    (((rand() % 200) / 100) - 1.0f) + 30,     // y
                    ((rand() % 500) / 100) - 14.0f)   // z
                * XMMatrixRotationY(rand() % 90);
            XMStoreFloat4x4(&_asteroids[i], position);
        }
    }

    for (UINT i = 0; i < sizeof(_trees) / sizeof(_trees[0]); i++)
    {
        XMMATRIX position = XMMatrixScaling(10.0f, 10.0f, 10.0f) *
            XMMatrixTranslation(
                ((rand() % 10000) / 100),     // x
                (((rand() % 1000) / 100)) + 30,     // y
                ((rand() % 1000) / 100) + 50.0f);   // z
        XMStoreFloat4x4(&_trees[i], position);
    }

    bool changed = false;

    do {
        changed = false;
        for (UINT i = 0; i < sizeof(_trees) / sizeof(_trees[0]); i++)
        {
            for (UINT j = 0; j < sizeof(_trees) / sizeof(_trees[0]); j++)
            {
                if (j != i) {
                    if (_trees[i]._41 == _trees[j]._41 && 
                        (_trees[i]._43 > (_trees[j]._43 - 10) && _trees[i]._43 < (_trees[j]._43 + 10))) // x
                    {
                        _trees[i]._41 += 1.0f;
                        changed = true;
                    }
                    if (_trees[i]._43 == _trees[j]._43 && 
                        (_trees[i]._41 > (_trees[j]._41 - 10) && _trees[i]._41 < (_trees[j]._41 + 10))) // z
                    {
                        _trees[i]._43 += 1.0f;
                        changed = true;
                    }
                }
            }
        }
    } while (changed);

    XMStoreFloat4x4(_terrain->getPosition(), XMMatrixIdentity() * XMMatrixTranslation(0.0f, 0.0f, 0.0f));

    return S_OK;
}

DX11Framework::~DX11Framework()
{
    if (_immediateContext)_immediateContext->Release();
    if (_device)_device->Release();
    if (_dxgiDevice)_dxgiDevice->Release();
    if (_dxgiFactory)_dxgiFactory->Release();
    if (_frameBufferView)_frameBufferView->Release();
    if (_swapChain)_swapChain->Release();

    if (_fillState)_fillState->Release();
    if (_wireframeState)_wireframeState->Release();
    if (_cullnoneState) _cullnoneState->Release();

    if (_vertexShader)_vertexShader->Release();
    if (_inputLayout)_inputLayout->Release();
    if (_pixelShader)_pixelShader->Release();
    if (_constantBuffer)_constantBuffer->Release();
    if (_pyramidVertexBuffer)_pyramidVertexBuffer->Release();
    if (_pyramidIndexBuffer)_pyramidIndexBuffer->Release();
    if (_lineVertexBuffer)_lineVertexBuffer->Release();
    if (_depthStencilBuffer)_depthStencilBuffer->Release();
    if (_depthStencilView)_depthStencilView->Release();
    if (_bilinearSamplerState)_bilinearSamplerState->Release();

    if (_skyboxVertexShader) _skyboxVertexShader->Release();
    if (_skyboxPixelShader) _skyboxPixelShader->Release();
    if (_depthStencilSkybox) _depthStencilSkybox->Release();

    if (_billboardVertexShader) _billboardVertexShader->Release();
    if (_billboardPixelShader) _billboardPixelShader->Release();

    if (_terrainVertexShader) _terrainVertexShader->Release();
    if (_terrainPixelShader) _terrainPixelShader->Release();

    if (_billboardVertexBuffer) _billboardVertexBuffer->Release();
    if (_billboardIndexBuffer) _billboardIndexBuffer->Release();

    if (_billboardTexture) _billboardTexture->Release();

    if (_terrain) delete _terrain; _terrain = nullptr;

    if (_blendState) _blendState->Release();
}


void DX11Framework::Update()
{
    //Static initializes this value only once    
    static ULONGLONG frameStart = GetTickCount64();

    ULONGLONG frameNow = GetTickCount64();
    float deltaTime = (frameNow - frameStart) / 1000.0f;
    frameStart = frameNow;

    static float simpleCount = 0.0f;
    simpleCount += deltaTime;

    if (GetActiveWindow() == _windowHandle) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x0001) {
            if (_wireframe == false) {
                _wireframe = true;
            }
            else {
                _wireframe = false;
            }
        }

        if (GetAsyncKeyState(VK_SUBTRACT) & 0x0001) {
            if (_fogToggle == false) {
                _fogToggle = true;
                _cbData.FogEnabled = 1;
            }
            else {
                _fogToggle = false;
                _cbData.FogEnabled = 0;
            }
        }

        if (GetAsyncKeyState(48) & 0x0001) { // 0
            if (currentCam != 0) {
                currentCam = 0;
            }
        }
        if (GetAsyncKeyState(49) & 0x0001) { // 1
            if (currentCam != 1) {
                currentCam = 1;
            }
        }
        if (GetAsyncKeyState(50) & 0x0001) { // 2
            if (currentCam != 2) {
                currentCam = 2;
            }
        }
        if (GetAsyncKeyState(51) & 0x0001) { // 3
            if (currentCam != 3) {
                currentCam = 3;
            }
        }
        if (GetAsyncKeyState(52) & 0x0001) { // 4
            if (currentCam != 4) {
                currentCam = 4;
            }
        }
        if (GetAsyncKeyState(53) & 0x0001) { // 5
            if (currentCam != 5) {
                _cameras[5]->SetPosition(_cameras[currentCam]->GetPosition());
                currentCam = 5;
            }
        }
    }

    _cameras[currentCam]->Update(deltaTime);

    MouseDetection(_windowHandle);

    _cbData.DiffuseMaterial = _diffuseMaterial;
    _cbData.AmbientMaterial = _ambientMaterial;
    _cbData.SpecularMaterial = _specularMaterial;
    _cbData.SpecularPower = 10.0f;
    _cbData.PointLight = _pointLight;
    _cbData.DirectionalLight = _directionalLight;
    _cbData.EyePosW = _cameras[currentCam]->GetPosition();

    XMStoreFloat4x4(&_cubes[0], XMMatrixIdentity() * XMMatrixTranslation(0, 30, 0) * XMMatrixRotationY(simpleCount)); // axis rotation
    //XMStoreFloat4x4(&_cubes[0], XMMatrixIdentity());
    // XMMatrixTranslation(0.5f, 0.5f, 0.5f)
    // if matrix translation 2nd - moved position of shape relative to camera, 
    // if matrix translation 1st - moves rotation but not shape, so on rotation, we get closer and further away from cube depending on the rotation

    // XMMatrixScaling - scales size of an object in 3D space
    // XMMatrixTranslationFromVector - take vector and makes a translation matrix from it - use for movement
    XMStoreFloat4x4(&_cubes[1], XMMatrixRotationY(simpleCount * 2.0f) * XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(4.0f, 0.0f, 2.0f) * XMMatrixRotationY(-simpleCount * 0.25f) * XMLoadFloat4x4(&_cubes[0]));
    //XMStoreFloat4x4(&_cubes[1], XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(4.0f, 0.0f, 2.0f) * XMLoadFloat4x4(&_cubes[0]));
    XMStoreFloat4x4(&_pyramids[0], XMMatrixScaling(0.25f, 0.25f, 0.25f) * XMMatrixTranslation(1.5, 0, 4) * XMMatrixRotationY(-simpleCount) * XMLoadFloat4x4(&_cubes[1]));
    //XMStoreFloat4x4(&_pyramids[0], XMMatrixScaling(0.25f, 0.25f, 0.25f) * XMMatrixTranslation(1.5, 0, 4) * XMLoadFloat4x4(&_cubes[1]));

    for (UINT i = 0; i < sizeof(_asteroids) / sizeof(_asteroids[0]); i++)
    {
        XMMATRIX position = XMLoadFloat4x4(&_asteroids[i]);
        //position *= XMMatrixRotationY(1 * deltaTime);
        XMStoreFloat4x4(&_asteroids[i], position);
    }

    XMFLOAT3 camPos = _cameras[currentCam]->GetPosition();
    XMStoreFloat4x4(&_skybox, XMMatrixScaling(100.0f, 100.0f, 100.0f) * XMMatrixTranslation(camPos.x, camPos.y, camPos.z));
}

void DX11Framework::Draw()
{    
    //Present unbinds render target, so rebind and clear at start of each frame
    float backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };  
    _immediateContext->OMSetRenderTargets(1, &_frameBufferView, _depthStencilView);
    _immediateContext->ClearRenderTargetView(_frameBufferView, backgroundColor);
    _immediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // bind as secondary render target then clear
    //Store this frames data in constant buffer struct
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(&_cubes[0]));
    _cbData.View = XMMatrixTranspose(XMLoadFloat4x4(_cameras[currentCam]->GetView()));
    _cbData.Projection = XMMatrixTranspose(XMLoadFloat4x4(_cameras[currentCam]->GetProjection()));

    //Write constant buffer data onto GPU
    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);

    //Set object variables and draw
    UINT stride = {sizeof(SimpleVertex)};
    UINT offset =  0 ;

    // set shaders
    SetShaders(_vertexShader, _pixelShader);

    FLOAT blendFactor[4] = {0.75f, 0.75f, 0.75f, 1.0f};
    _immediateContext->OMSetBlendState(0, 0, 0xffffffff); // no blend

    /////////////////////////
    // DRAW OPAQUE OBJECTS //
    /////////////////////////

    SetShaderResources(_gameObjects[2].GetShaderResourceC(), _gameObjects[2].GetShaderResourceS(), _gameObjects[2].GetShaderResourceN());

    SetRS(_fillState);

    _gameObjects[2].Draw(_immediateContext, &_cbData);

    _gameObjects[2].SetPosition(_cubes[0]);

    DrawObjects(_gameObjects[2].GetMeshData()->m_indexCount, _gameObjects[2].getPosition(), &mappedSubresource);

    ////////    non norm mapped cube - show working spec map and specular lighting
    // needed as when using norm map, specular light can slightly bleed onto back

    SetShaderResources(_gameObjects[3].GetShaderResourceC(), _gameObjects[3].GetShaderResourceS(), _gameObjects[3].GetShaderResourceN());

    _gameObjects[3].Draw(_immediateContext, &_cbData);

    XMFLOAT4X4 nonNormMap;
    XMStoreFloat4x4(&nonNormMap, XMMatrixTranslation(0, 10.0f, 0) * XMLoadFloat4x4(&_cubes[0]));

    _gameObjects[3].SetPosition(nonNormMap);

    DrawObjects(_gameObjects[3].GetMeshData()->m_indexCount, _gameObjects[3].getPosition(), &mappedSubresource);

    /////////

    SetShaderResources(_gameObjects[_gameObjects.size() - 2].GetShaderResourceC(), _gameObjects[_gameObjects.size() - 2].GetShaderResourceS(), _gameObjects[_gameObjects.size() - 2].GetShaderResourceN());
    
    SetRS(_cullnoneState);

    _gameObjects[_gameObjects.size() - 2].Draw(_immediateContext, &_cbData);

    for (UINT i = 1; i < sizeof(_cubes) / sizeof(_cubes[0]); i++)
    {
        _gameObjects[4].SetPosition(_cubes[i]);

        DrawObjects(_gameObjects[_gameObjects.size() - 2].GetMeshData()->m_indexCount, _gameObjects[_gameObjects.size() - 2].getPosition(), &mappedSubresource);
    }

    //////////////////////////////////////////////////////////////////

    SetRS(_fillState);

    SetShaderResources(_gameObjects[2].GetShaderResourceC(), _gameObjects[2].GetShaderResourceS(), _gameObjects[2].GetShaderResourceN());

    SetBuffers(_pyramidVertexBuffer, _pyramidIndexBuffer, &stride, &offset, DXGI_FORMAT_R16_UINT);

    _cbData.HasTexture = 1;
    _cbData.SpecMap = 1;
    _cbData.NormMap = 0;

    for (UINT i = 0; i < sizeof(_pyramids) / sizeof(_pyramids[0]); i++)
    {
        DrawObjects(18, &_pyramids[i], &mappedSubresource);
    }

    ///////////////////////////////////////////////////////////////////////////////////////

    // billboards

    SetShaders(_billboardVertexShader, _billboardPixelShader);

    _immediateContext->PSSetShaderResources(0, 1, &_billboardTexture);

    SetBuffers(_billboardVertexBuffer, _billboardIndexBuffer, &stride, &offset, DXGI_FORMAT_R16_UINT);

    _cbData.HasTexture = 1;
    _cbData.SpecMap = 0;
    _cbData.NormMap = 0;

    for (UINT i = 0; i < sizeof(_trees) / sizeof(_trees[0]); i++)
    {
        DrawObjects(6, &_trees[i], &mappedSubresource);
    }

    //terrain
    SetShaders(_terrainVertexShader, _terrainPixelShader);

    _terrain->Draw(_immediateContext, &_cbData);

    DrawObjects(_terrain->GetIndexCount(), _terrain->getPosition(), &mappedSubresource);

    /////////////////////////
    //     DRAW SKYBOX     //
    /////////////////////////

    SetRS(_cullnoneState);

    // set shaders to skybox ver
    SetShaders(_skyboxVertexShader, _skyboxPixelShader);

    // set stencil state to new one
    _immediateContext->OMSetDepthStencilState(_depthStencilSkybox, 1);

    // call draw to set buffers
    _gameObjects[_gameObjects.size() - 1].Draw(_immediateContext, &_cbData);

    // set position (will update with cam) 
    _gameObjects[_gameObjects.size() - 1].SetPosition(_skybox);

    // set shader resource to the texture for skybox
    _immediateContext->PSSetShaderResources(0, 1, _gameObjects[_gameObjects.size() - 1].GetShaderResourceC());

    DrawObjects(_gameObjects[_gameObjects.size() - 1].GetMeshData()->m_indexCount, _gameObjects[_gameObjects.size() - 1].getPosition(), &mappedSubresource);

    //////////////////////////////
    // DRAW TRANSPARENT OBJECTS //
    //////////////////////////////

    SetRS(_fillState);

    SetShaders(_vertexShader, _pixelShader);

    SetShaderResources(_gameObjects[2].GetShaderResourceC(), _gameObjects[2].GetShaderResourceS(), _gameObjects[2].GetShaderResourceN());

    _immediateContext->OMSetBlendState(_blendState, blendFactor, 0xffffffff); // blending / transparency

    _gameObjects[2].Draw(_immediateContext, &_cbData);

    for (UINT i = 0; i < sizeof(_asteroids) / sizeof(_asteroids[0]); i++)
    {
        _gameObjects[2].SetPosition(_asteroids[i]);
        DrawObjects(_gameObjects[2].GetMeshData()->m_indexCount, _gameObjects[2].getPosition(), &mappedSubresource);
    }

    _immediateContext->OMSetBlendState(0, 0, 0xffffffff);

    //Present Backbuffer to screen
    _swapChain->Present(0, 0);
}

/// <summary>
/// set buffers using specified format
/// </summary>
/// <param name="vBuffer"></param>
/// <param name="pBuffer"></param>
/// <param name="stride"></param>
/// <param name="offset"></param>
/// <param name="format"></param>
void DX11Framework::SetBuffers(
    ID3D11Buffer* vBuffer,
    ID3D11Buffer* pBuffer,
    UINT* stride,
    UINT* offset,
    DXGI_FORMAT format) {
    _immediateContext->IASetVertexBuffers(0, 1, &vBuffer, stride, offset);
    _immediateContext->IASetIndexBuffer(pBuffer, format, 0);
}

/// <summary>
/// set shaders using specified shaders
/// </summary>
/// <param name="vShader"></param>
/// <param name="pShader"></param>
void DX11Framework::SetShaders(
    ID3D11VertexShader* vShader,
    ID3D11PixelShader* pShader) {
    _immediateContext->VSSetShader(vShader, nullptr, 0);
    _immediateContext->PSSetShader(pShader, nullptr, 0);
}

/// <summary>
/// set shader resources using specified SRVs
/// </summary>
/// <param name="C"></param>
/// <param name="S"></param>
/// <param name="N"></param>
void DX11Framework::SetShaderResources(
    ID3D11ShaderResourceView** C,
    ID3D11ShaderResourceView** S,
    ID3D11ShaderResourceView** N) {
    if (C != nullptr) {
        _immediateContext->PSSetShaderResources(0, 1, C);
    }
    if (S != nullptr) {
        _immediateContext->PSSetShaderResources(1, 1, S);
    }
    if (N != nullptr) {
        _immediateContext->PSSetShaderResources(2, 1, N);
    }
}

/// <summary>
/// set rasterizer state when not using wireframe mode
/// </summary>
/// <param name="state"></param>
void DX11Framework::SetRS(ID3D11RasterizerState* state) {
    if (!_wireframe) {
        _immediateContext->RSSetState(state);
    }
    else {
        _immediateContext->RSSetState(_wireframeState);
    }
}

/// <summary>
/// draw objects at specified position
/// </summary>
/// <param name="indices"></param>
/// <param name="position"></param>
/// <param name="mSubRes"></param>
void DX11Framework::DrawObjects(
    UINT indices,
    XMFLOAT4X4* position,
    D3D11_MAPPED_SUBRESOURCE* mSubRes) {
    // remap - update data
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, mSubRes);
    // load new world info
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(position));

    memcpy(mSubRes->pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);

    _immediateContext->DrawIndexed(indices, 0, 0);
}

/// <summary>
/// register and capture mouse when the program is the main window
/// </summary>
/// <param name="hWnd"></param>
void DX11Framework::MouseDetection(HWND hWnd) {
    POINT cursorPos;
    RECT rect; GetWindowRect(hWnd, &rect);

    if (GetAsyncKeyState(VK_LBUTTON) && GetActiveWindow() == _windowHandle) {
        
        SetCapture(hWnd);
        ClipCursor(&rect);
        GetCursorPos(&cursorPos);

        ScreenToClient(hWnd, &cursorPos);
        OnMouseMove(cursorPos.x - (_WindowWidth / 2), cursorPos.y - (_WindowHeight / 2));
    }
    else {
        ClipCursor(NULL);
        ReleaseCapture();
    }
}
    
/// <summary>
/// when using freecam, get normalized vector from centre of window to the cursor to calculate angles for camera rotation
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
void DX11Framework::OnMouseMove(int x, int y)
{
    if (currentCam == 5) {
        FreeCamera* fc = dynamic_cast<FreeCamera*>(_cameras[5]);

        // move via vector from centre screen to mouse
        XMFLOAT2 dir(x, y);

        XMStoreFloat2(&dir, XMVector2Normalize(XMLoadFloat2(&dir)));

        // static cast is safer for conversion
        fc->Pitch(XMConvertToRadians(0.25f * static_cast<float>(dir.y * 3)));
        fc->RotateY(XMConvertToRadians(0.25f * static_cast<float>(dir.x * 3)));
    }
}