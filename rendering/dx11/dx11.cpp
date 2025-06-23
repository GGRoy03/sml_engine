// ===================================
//  Type Definitions
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

static const char* SmlDefaultShader = R"(

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 View;
    float4x4 Projection;
};

cbuffer MaterialBuffer : register(b1)
{
    float3 AlbedoFactor;
    float  MetallicFactor;
    float  RoughnessFactor;
};

//--------------------------------------------------------------------------------------
// Textures & Sampler
//--------------------------------------------------------------------------------------
Texture2D    AlbedoTexture            : register(t0);
Texture2D    NormalTexture            : register(t1);
Texture2D    MetallicRoughnessTexture : register(t2);
Texture2D    AmbientOcclusionTexture  : register(t3);

SamplerState MaterialSampler : register(s0);

//--------------------------------------------------------------------------------------
// Vertex input / output
//--------------------------------------------------------------------------------------
struct VSInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal   : TEXCOORD1;
    float2 UV       : TEXCOORD2;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VSOutput VSMain(VSInput IN)
{
    VSOutput OUT;

    OUT.Position = mul(Projection, mul(float4(IN.Position, 1.0f), View));
    OUT.Normal   = IN.Normal;
    OUT.UV       = IN.UV;

    return OUT;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_Target
{
    // Sample all maps
    float3 albedo   = AlbedoTexture.Sample(MaterialSampler, IN.UV).rgb * AlbedoFactor;
    float  metallic = MetallicRoughnessTexture.Sample(MaterialSampler, IN.UV).b * MetallicFactor;
    float  roughness= MetallicRoughnessTexture.Sample(MaterialSampler, IN.UV).g * RoughnessFactor;
    float  ao       = AmbientOcclusionTexture.Sample(MaterialSampler, IN.UV).r;

    float3 color = albedo;

    return float4(color, 1.0f);
}

//--------------------------------------------------------------------------------------
// Technique / Pass (for FX-based systems, optionally)
//--------------------------------------------------------------------------------------
)";

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

#include "dx11_pipeline.cpp"

// ===================================
// Functions declaration
// ===================================

static void SmlDx11_Render(sml_render_command_header* Header);

// ===================================
// Internal functions
// ===================================

static sml_render_playback_function
SmlDx11_Initialize(sml_window Window)
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

    Status = Dx11.Device->CreateRenderTargetView(BackBuffer, nullptr, &Dx11.RenderView);
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

    // Default pipeline init
    {
        sml_shader_desc ShaderDesc[2] = {};

        sml_shader_desc *VShaderDesc        = &ShaderDesc[0];
        VShaderDesc->Type                   = SmlShader_Vertex;
        VShaderDesc->Flags                  = SmlShader_CompileFromBuffer;
        VShaderDesc->Info.Buffer.EntryPoint = "VSMain";
        VShaderDesc->Info.Buffer.ByteCode   = SmlDefaultShader;
        VShaderDesc->Info.Buffer.Size       = strlen(SmlDefaultShader);

        sml_shader_desc *PShaderDesc        = &ShaderDesc[1];
        PShaderDesc->Type                   = SmlShader_Pixel;
        PShaderDesc->Flags                  = SmlShader_CompileFromBuffer;
        PShaderDesc->Info.Buffer.EntryPoint = "PSMain";
        PShaderDesc->Info.Buffer.ByteCode   = SmlDefaultShader;
        PShaderDesc->Info.Buffer.Size       = strlen(SmlDefaultShader);

        sml_pipeline_layout Layout[3] = 
        {
            {"POSITION", 0, SmlData_Vector3Float},
            {"NORMAL"  , 0, SmlData_Vector3Float},
            {"TEXCOORD", 0, SmlData_Vector2Float},
        };

        sml_pipeline_desc PipelineDesc = {};
        PipelineDesc.Shaders           = ShaderDesc;
        PipelineDesc.ShaderCount       = 2;
        PipelineDesc.Layout            = Layout;
        PipelineDesc.LayoutCount       = 3;

        SmlPipeline = SmlDx11_CreatePipeline(PipelineDesc);

        CubeMesh = GetCubeMesh();

        D3D11_MAPPED_SUBRESOURCE VMapped;
        Status = Dx11.Context->Map(SmlPipeline.VertexBuffer.Handle, 0, D3D11_MAP_WRITE_DISCARD,
                                   0, &VMapped);

        memcpy(VMapped.pData, CubeMesh.VertexData, CubeMesh.VertexDataSize);

        Dx11.Context->Unmap(SmlPipeline.VertexBuffer.Handle, 0);

        D3D11_MAPPED_SUBRESOURCE IMapped;
        Status = Dx11.Context->Map(SmlPipeline.IndexBuffer.Handle, 0, D3D11_MAP_WRITE_DISCARD,
                                   0, &IMapped);

        memcpy(IMapped.pData, CubeMesh.IndexData, CubeMesh.IndexDataSize);

        Dx11.Context->Unmap(SmlPipeline.IndexBuffer.Handle, 0);
    }

    // Default material init
    {
        sml_material_desc MaterialDesc = {};

        sml_texture_desc *Albedo = MaterialDesc.Textures + SmlMaterial_Albedo;
        Albedo->Info.Path        = "../../small_engine/data/textures/brick_wall/brick_wall_albedo.png";
        Albedo->MaterialType     = SmlMaterial_Albedo;
        Albedo->BindSlot         = 0;

        sml_texture_desc *Normal = MaterialDesc.Textures + SmlMaterial_Normal;
        Normal->Info.Path        = "../../small_engine/data/textures/brick_wall/brick_wall_normal.png";
        Normal->MaterialType     = SmlMaterial_Normal;
        Normal->BindSlot         = 1;

        sml_texture_desc *Metallic = MaterialDesc.Textures + SmlMaterial_Metallic;
        Metallic->Info.Path        = "../../small_engine/data/textures/brick_wall/brick_wall_metallic.png";
        Metallic->MaterialType     = SmlMaterial_Metallic;
        Metallic->BindSlot         = 2;

        sml_texture_desc *Ambient = MaterialDesc.Textures + SmlMaterial_AmbientOcc;
        Ambient->Info.Path        = "../../small_engine/data/textures/brick_wall/brick_wall_ao.png";
        Ambient->MaterialType     = SmlMaterial_AmbientOcc;
        Ambient->BindSlot         = 3;

        sml_pbr_material_constant *Constants = &MaterialDesc.Constants;
        Constants->AlbedoFactor              = sml_vector3(1.0f, 1.0f, 1.0f);
        Constants->MetallicFactor            = 1.0f;
        Constants->RoughnessFactor           = 1.0f;

        SmlPipeline.Material = SmlDx11_CreateMaterial(MaterialDesc);
    }

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

    SmlDx11_BindPipeline(&SmlPipeline);

    Context->DrawIndexed(CubeMesh.IndexCount, 0, 0);
    Dx11.SwapChain->Present(1, 0);
}
