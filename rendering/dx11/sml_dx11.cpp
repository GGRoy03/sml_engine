// ===================================
// Type Definitions
// ===================================

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

struct sml_dx11_context
{
    ID3D11Device           *Device;
    ID3D11DeviceContext    *Context;
    IDXGISwapChain         *SwapChain;
    ID3D11RenderTargetView *RenderView;

    ID3D11Texture2D        *DepthTexture;
    ID3D11DepthStencilView *DepthView;

    ID3D11Buffer *CameraBuffer;

    D3D11_VIEWPORT Viewport;
};

// ===================================
//  Global variables
// ===================================

static sml_dx11_context Dx11;
static sml_tri_mesh     CubeMesh;

// ===================================
// Internal functions
// ===================================

static DXGI_FORMAT
SmlDx11_GetDXGIFormat(SmlData_Type Format)
{
    switch(Format)
    {

    case SmlData_Vector2Float: return DXGI_FORMAT_R32G32_FLOAT;
    case SmlData_Vector3Float: return DXGI_FORMAT_R32G32B32_FLOAT;

    default: return DXGI_FORMAT_UNKNOWN;
    }
}

// ===================================
// Directx11 Files
// ===================================

#include "sml_dx11_pipeline.cpp"

// ===================================
// Functions declaration
// ===================================

static void SmlDx11_Render(sml_render_command_header* Header);

// ===================================
// Internal functions
// ===================================

static sml_render_playback_function
SmlDx11_Initialize(sml_window Window, sml_material_desc MaterialDesc,
                   sml_pipeline_desc PipelineDesc)
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

    Status = Dx11.Device->CreateDepthStencilView(Dx11.DepthTexture, &DepthTexDescV, &Dx11.DepthView);

    Sml_Assert(SUCCEEDED(Status));

    SmlPipeline.BackendData         = SmlDx11_CreatePipeline(PipelineDesc);
    sml_dx11_pipeline *Dx11Pipeline = (sml_dx11_pipeline*)SmlPipeline.BackendData;

    // Upload a simple cube
    {
        CubeMesh = GetCubeMesh();

        D3D11_MAPPED_SUBRESOURCE VMapped;
        Status = Dx11.Context->Map(Dx11Pipeline->VertexBuffer.Handle, 0,
                                   D3D11_MAP_WRITE_DISCARD, 0, &VMapped);

        memcpy(VMapped.pData, CubeMesh.VertexData, CubeMesh.VertexDataSize);

        Dx11.Context->Unmap(Dx11Pipeline->VertexBuffer.Handle, 0);

        D3D11_MAPPED_SUBRESOURCE IMapped;
        Status = Dx11.Context->Map(Dx11Pipeline->IndexBuffer.Handle, 0,
                                   D3D11_MAP_WRITE_DISCARD, 0, &IMapped);

        memcpy(IMapped.pData, CubeMesh.IndexData, CubeMesh.IndexDataSize);

        Dx11.Context->Unmap(Dx11Pipeline->IndexBuffer.Handle, 0);
    }

    Dx11Pipeline->Material = SmlDx11_CreateMaterial(MaterialDesc);

    // Camera init
    {
        struct
        {
            sml_matrix4 View;
            sml_matrix4 Projection;
        } Camera;

        D3D11_BUFFER_DESC CBufferDesc = {};
        CBufferDesc.ByteWidth         = sizeof(Camera);
        CBufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        CBufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
        CBufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

        Status = Dx11.Device->CreateBuffer(&CBufferDesc, nullptr, &Dx11.CameraBuffer);
        Sml_Assert(SUCCEEDED(Status));

        D3D11_MAPPED_SUBRESOURCE CMapped;
        Status = Dx11.Context->Map(Dx11.CameraBuffer, 0,
                                   D3D11_MAP_WRITE_DISCARD, 0, &CMapped);
        Sml_Assert(SUCCEEDED(Status));

        sml_vector3 Eye     = sml_vector3(1.0f, 0.0f, -1.0f);
        sml_vector3 WorldUp = sml_vector3(0.0f, 1.0f, 0.0f);
        sml_vector3 Target  = sml_vector3(0.0f, 0.0f, 0.0f);
        Camera.View         = SmlMat4_LookAt(Eye, Target, WorldUp);
        Camera.Projection   = SmlMat4_Perspective(90.0f, (f32)Window.Width/(f32)Window.Height);

        memcpy(CMapped.pData, &Camera, sizeof(Camera));
        Dx11.Context->Unmap(Dx11.CameraBuffer, 0);
    }

    return SmlDx11_Render;
}

// ===================================
// Commands Implementations
// ===================================

static void
SmlDx11_Render(sml_render_command_header *Header)
{
    Sml_Unused(Header);

    ID3D11DeviceContext *Context = Dx11.Context;

    const float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    Context->ClearRenderTargetView(Dx11.RenderView, ClearColor);
    Context->ClearDepthStencilView(Dx11.DepthView,
                                   D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                   1.0f, 0);

    Context->OMSetRenderTargets(
        1,
        &Dx11.RenderView,
        Dx11.DepthView
    );

    Context->RSSetViewports(1, &Dx11.Viewport);

    Context->VSSetConstantBuffers(0, 1, &Dx11.CameraBuffer);

    // TODO: Add topology in the pipeline
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    sml_dx11_pipeline *Pipeline = (sml_dx11_pipeline*)SmlPipeline.BackendData;
    SmlDx11_BindPipeline(Pipeline);

    Context->DrawIndexed(CubeMesh.IndexCount, 0, 0);
    Dx11.SwapChain->Present(1, 0);
}
