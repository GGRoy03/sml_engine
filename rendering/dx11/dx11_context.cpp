#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

// ===================================
// Type Definitions
// ===================================

struct dx11_context
{
    ID3D11Device           *Device;
    ID3D11DeviceContext    *Context;
    IDXGISwapChain         *SwapChain;
    ID3D11RenderTargetView *RenderView;

    ID3D11Texture2D        *DepthTexture;
    ID3D11DepthStencilView *DepthView;

    ID3D11Buffer *FrameBuffer;

    D3D11_VIEWPORT Viewport;
};

// ===================================
//  Global variables
// ===================================

// NOTE: Move this to backend?
static dx11_context Dx11;

// ===================================
// Helper Functions
// ===================================

template<typename T>
static inline void
Dx11_Release(T *&Resource)
{
    if(Resource)
    {
        Resource->Release();
        Resource = nullptr;
    }
}

// ===================================
// Directx11 Files
// ===================================

#include "dx11_resource.cpp"
#include "dx11_playback.cpp"

// ===================================
// API Implementation
// ===================================

// WARN: 
// 1) This needs a huge cleanup
// 2) Uses malloc for the renderer allocation

static void
Dx11_Initialize(sml_window Window)
{
    DXGI_SWAP_CHAIN_DESC SDesc = {};
    SDesc.BufferCount          = 2;
    SDesc.BufferDesc.Width     = Window.Width;
    SDesc.BufferDesc.Height    = Window.Height;
    SDesc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    SDesc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SDesc.OutputWindow         = (HWND)Window.Handle;
    SDesc.SampleDesc.Count     = 1;
    SDesc.Windowed             = TRUE;
    SDesc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT CreateFlags = D3D11_CREATE_DEVICE_DEBUG;
#if _DEBUG
    CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT Status = 
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                      CreateFlags, nullptr, 0, D3D11_SDK_VERSION,
                                      &SDesc, &Dx11.SwapChain, &Dx11.Device,
                                      nullptr, &Dx11.Context);
    Sml_Assert(SUCCEEDED(Status));

    ID3D11Texture2D* BackBuffer = nullptr;
    Status = Dx11.SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
    Sml_Assert(SUCCEEDED(Status));

    Status = Dx11.Device->CreateRenderTargetView(BackBuffer, nullptr,
                                                 &Dx11.RenderView);
    Sml_Assert(SUCCEEDED(Status));

    BackBuffer->Release();

    Dx11.Context->OMSetRenderTargets(1, &Dx11.RenderView, nullptr);

    D3D11_VIEWPORT *Viewport = &Dx11.Viewport;
    Viewport->TopLeftX       = 0.0f;
    Viewport->TopLeftY       = 0.0f;
    Viewport->Width          = (FLOAT)Window.Width;
    Viewport->Height         = (FLOAT)Window.Height;
    Viewport->MinDepth       = 0.0f;
    Viewport->MaxDepth       = 1.0f;

    // Depth

    D3D11_TEXTURE2D_DESC DepthTexDesc = {};
    DepthTexDesc.Width                = Window.Width;
    DepthTexDesc.Height               = Window.Height;
    DepthTexDesc.MipLevels            = 1;
    DepthTexDesc.ArraySize            = 1;
    DepthTexDesc.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DepthTexDesc.SampleDesc.Count     = 1;
    DepthTexDesc.SampleDesc.Quality   = 0;
    DepthTexDesc.Usage                = D3D11_USAGE_DEFAULT;
    DepthTexDesc.BindFlags            = D3D11_BIND_DEPTH_STENCIL;
    DepthTexDesc.CPUAccessFlags       = 0;
    DepthTexDesc.MiscFlags            = 0;

    Status = Dx11.Device->CreateTexture2D(&DepthTexDesc, nullptr, &Dx11.DepthTexture);

    Sml_Assert(SUCCEEDED(Status));

    D3D11_DEPTH_STENCIL_VIEW_DESC DepthTexDescV = {};
    DepthTexDescV.Format                        = DepthTexDesc.Format;
    DepthTexDescV.ViewDimension                 = D3D11_DSV_DIMENSION_TEXTURE2D;
    DepthTexDescV.Texture2D.MipSlice            = 0;

    Status = Dx11.Device->CreateDepthStencilView(Dx11.DepthTexture, &DepthTexDescV,
                                                 &Dx11.DepthView);
    Sml_Assert(SUCCEEDED(Status));

    auto *Dx11_Renderer = (renderer*)SmlMemory.Allocate(sizeof(renderer)).Data;
    auto *Dx11_Backend  = (dx11_backend*)SmlMemory.Allocate(sizeof(dx11_backend)).Data;

    Dx11_Backend->Materials = slot_map<dx11_material, material>(10);
    Dx11_Backend->Instances = slot_map<dx11_instance, instance>(10);

    Dx11_Renderer->Backend = Dx11_Backend;

    Renderer = Dx11_Renderer;
    Playback = Dx11_Playback;

    ImGui_ImplDX11_Init(Dx11.Device, Dx11.Context);
}
