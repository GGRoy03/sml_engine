// ===================================
// Type Definitions
// ===================================

// NOTE: Temporary include
#include <unordered_map>

// WARN:
// 1) Should probably hold an optimized draw-list of some sort
// when we add materials.

struct sml_dx11_static_group_cached_state
{
    ID3D11Buffer *VertexBuffer;
    ID3D11Buffer *IndexBuffer;

    ID3D11VertexShader *VShader;
    ID3D11PixelShader  *PShader;
    ID3D11InputLayout  *InputLayout;

    UINT    Stride;
    sml_u32 IndexCount;
};

struct sml_dx11_sampled_texture
{
    ID3D11ShaderResourceView *View;
    sml_u32                   Slot;
};

struct sml_dx11_sampled_material
{
    sml_dx11_sampled_texture Texture;
    SmlMaterial_Type         Type;
};

struct sml_dx11_pbr_material
{
    sml_dx11_sampled_material Sampled[SmlMaterial_Count];
    ID3D11SamplerState       *SamplerState;
    ID3D11Buffer             *GPUHandle;
};

// ===================================
//  Global Variables
// ===================================

// NOTE: For now we simply use a std::map for our cache.
// This will probably change, but for simplicity the map should
// be more than fine.
std::unordered_map<sml_u32, sml_dx11_static_group_cached_state> StaticCache;

// NOTE: We also hold a map of materials for now. It's not that great.
// But it should do the work for prototyping.
std::unordered_map<sml_material_handle, sml_dx11_pbr_material> MaterialCache;

// ===================================
// Internal Helpers
// ===================================

static ID3D11ShaderResourceView*
SmlDx11_LoadTextureFromPath(const char *FileName)
{
    sml_i32 Width, Height, Channels;

    sml_u8 *Data = stbi_load(FileName, &Width, &Height, &Channels, 4);
    Sml_Assert(Data);

    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width                = Width;
    TexDesc.Height               = Height;
    TexDesc.MipLevels            = 1;
    TexDesc.ArraySize            = 1;
    TexDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    TexDesc.SampleDesc.Count     = 1;
    TexDesc.Usage                = D3D11_USAGE_DEFAULT;
    TexDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
    TexDesc.CPUAccessFlags       = 0;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem                = Data;
    InitData.SysMemPitch            = Width * 4;

    ID3D11Texture2D *Texture = nullptr;
    HRESULT          Status  = Dx11.Device->CreateTexture2D(&TexDesc, &InitData,
                                                            &Texture);
    Sml_Assert(SUCCEEDED(Status));

    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format                          = TexDesc.Format;
    SrvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
    SrvDesc.Texture2D.MostDetailedMip       = 0;
    SrvDesc.Texture2D.MipLevels             = 1;

    ID3D11ShaderResourceView *Srv = nullptr;
    Status = Dx11.Device->CreateShaderResourceView(Texture, &SrvDesc, &Srv);
    Sml_Assert(SUCCEEDED(Status));

    Texture->Release();
    stbi_image_free(Data);

    return Srv;
}

// WARN: 
// 1) Does not allow materials to be changed after creation (Immutable buffer)
// 2) Does not allow different sampler state parameters
// 3) There are numerous naming issues.

static sml_dx11_pbr_material
SmlDx11_CreateMaterial(sml_material_desc Desc)
{
    sml_dx11_pbr_material Material = {};
    HRESULT               Status   = S_OK;

    for(u32 Index = 1; Index < SmlMaterial_Count; ++Index) // Skips unknown
    {
        sml_texture_desc *TextureDesc = Desc.Textures + Index;

        if (TextureDesc->Flags & SmlTexture_CompileFromBuffer &&
            TextureDesc->Info.Buffer.Size > 0                 &&
            TextureDesc->Info.Buffer.Pixels)
        {

        }
        else if (TextureDesc->Info.Path)
        {
            const char* Path                   = TextureDesc->Info.Path;
            sml_dx11_sampled_material *Sampled = 
                &Material.Sampled[TextureDesc->MaterialType];

            Sampled->Texture.View = SmlDx11_LoadTextureFromPath(Path);
            Sampled->Texture.Slot = TextureDesc->BindSlot;
            Sampled->Type         = TextureDesc->MaterialType;
        }
    }

    D3D11_BUFFER_DESC CDesc = {};
    CDesc.ByteWidth         = sizeof(sml_pbr_material_constant);
    CDesc.Usage             = D3D11_USAGE_IMMUTABLE;
    CDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA ConstantData = {};
    ConstantData.pSysMem = &Desc.Constants;

    Status = Dx11.Device->CreateBuffer(&CDesc, &ConstantData, &Material.GPUHandle);
    Sml_Assert(SUCCEEDED(Status));

    D3D11_SAMPLER_DESC SampDesc = {};
    SampDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SampDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
    SampDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
    SampDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
    SampDesc.MipLODBias         = 0.0f;
    SampDesc.MaxAnisotropy      = 1;
    SampDesc.ComparisonFunc     = D3D11_COMPARISON_ALWAYS;
    SampDesc.BorderColor[0]     = SampDesc.BorderColor[1] = 0.0f;
    SampDesc.BorderColor[2]     = SampDesc.BorderColor[3] = 0.0f;
    SampDesc.MinLOD             = 0;
    SampDesc.MaxLOD             = D3D11_FLOAT32_MAX;

    Status = Dx11.Device->CreateSamplerState(&SampDesc, &Material.SamplerState);
    Sml_Assert(SUCCEEDED(Status));

    return Material;
}

// WARN: 
// 1) We probably want to crash even on release.

static ID3DBlob*
SmlDx11_CompileShader(const char *ByteCode, size_t ByteCodeSize,
                      const char* EntryPoint, const char *Profile,
                      UINT Flags)
{
    ID3DBlob *ShaderBlob = nullptr;
    ID3DBlob *ErrorBlob  = nullptr;

    HRESULT Status = D3DCompile(ByteCode, ByteCodeSize,
                                nullptr, nullptr, nullptr,
                                EntryPoint, Profile,
                                Flags, 0, &ShaderBlob, &ErrorBlob);

    if (FAILED(Status) || !ShaderBlob)
    {
        if (ErrorBlob)
        {
            OutputDebugStringA("Shader compiler error: ");
            OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
            OutputDebugStringA("\n");
            ErrorBlob->Release();
        }

        Sml_Assert(!"Failed to compile shader.");
    }

    return ShaderBlob;
}

// WARN:
// 1) Need to handle the forced crash better.

static ID3D11VertexShader*
SmlDx11_CreateVertexShader(sml_shader_desc *Shader, ID3DBlob **OutShaderBlob)
{
    Sml_Assert(Shader->Type == SmlShader_Vertex);

    HRESULT Status = S_OK;

    const char *EntryPoint = Shader->EntryPoint;
    const char *Profile    = "vs_5_0";

    UINT CompileFlagsConstants = Shader->Flags & SmlShader_Debug
                                     ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
                                     : D3DCOMPILE_OPTIMIZATION_LEVEL3;

    if (Shader->Flags & SmlShader_CompileFromBuffer)
    {
        const char *ByteCode     = Shader->Info.Buffer.ByteCode;
        SIZE_T      ByteCodeSize = (SIZE_T)Shader->Info.Buffer.Size;

        *OutShaderBlob = SmlDx11_CompileShader(ByteCode, ByteCodeSize, EntryPoint,
                                               Profile, CompileFlagsConstants);
    }
    else
    {
        Sml_Assert(!"SML_DX11: Compiling shader from path is not possible.");
    }

    ID3D11VertexShader *VertexShader = nullptr;
    Status = Dx11.Device->CreateVertexShader((*OutShaderBlob)->GetBufferPointer(),
                                             (*OutShaderBlob)->GetBufferSize(),
                                             nullptr, &VertexShader);
    if(FAILED(Status))
    {
        Sml_Assert(!"SML_DX11: Failed to compile pixel shader.");
        return nullptr;
    }

    return VertexShader;
}

// WARN:
// 1) Need to handle the forced crash better.

static ID3D11PixelShader*
SmlDx11_CreatePixelShader(sml_shader_desc *Shader)
{
    Sml_Assert(Shader->Type == SmlShader_Pixel);

    HRESULT   Status     = S_OK;
    ID3DBlob *ShaderBlob = nullptr;

    const char *EntryPoint = Shader->EntryPoint;
    const char *Profile    = "ps_5_0";

    UINT CompileFlagsConstants = Shader->Flags & SmlShader_Debug
                                     ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
                                     : D3DCOMPILE_OPTIMIZATION_LEVEL3;

    if (Shader->Flags & SmlShader_CompileFromBuffer)
    {
        const char *ByteCode     = Shader->Info.Buffer.ByteCode;
        SIZE_T      ByteCodeSize = (SIZE_T)Shader->Info.Buffer.Size;

        ShaderBlob = SmlDx11_CompileShader(ByteCode, ByteCodeSize, EntryPoint,
                                           Profile, CompileFlagsConstants);
    }
    else
    {
        Sml_Assert(!"SML_DX11: Compiling shader from path is not possible.");
    }

    ID3D11PixelShader *PixelShader = nullptr;
    Status = Dx11.Device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
                                            ShaderBlob->GetBufferSize(),
                                            nullptr, &PixelShader);
    ShaderBlob->Release();

    if(FAILED(Status))
    {
        Sml_Assert(!"SML_DX11: Failed to compile pixel shader.");
        return nullptr;
    }

    return PixelShader;
}

static ID3D11InputLayout*
SmlDx11_CreateInputLayout(sml_pipeline_layout *Layout, sml_u32 Count,
                          ID3DBlob *VertexBlob)
{
    HRESULT Status = S_OK;
    UINT    Stride = 0;

    D3D11_INPUT_ELEMENT_DESC Elements[16] = {};
    for (u32 Input = 0; Input < Count; ++Input)
    {
        auto                     *Desc    = Layout   + Input;
        D3D11_INPUT_ELEMENT_DESC *Element = Elements + Input;

        Element->SemanticName      = Desc->Name;
        Element->SemanticIndex     = 0;
        Element->Format            = SmlDx11_GetDXGIFormat(Desc->Format);
        Element->InputSlot         = Desc->Slot;
        Element->AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        Element->InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;

        size_t ElementSize = Sml_GetSizeOfDataType(Desc->Format);
        Stride            += (UINT)ElementSize;
    }

    ID3D11InputLayout *InputLayout = nullptr;
    Status = Dx11.Device->CreateInputLayout(Elements, Count,
                                            VertexBlob->GetBufferPointer(),
                                            VertexBlob->GetBufferSize(),
                                            &InputLayout);
    if(FAILED(Status))
    {
        Sml_Assert(!"Failed to create input layout.");
        return nullptr;
    }

    return InputLayout;
}

// ===================================
// API Implementation
// ===================================

// WARN:
// This should probably sort the meshes by material.

static inline void
SmlDx11_SetupStaticGroup(sml_setup_command_static_group *Payload)
{
    sml_dx11_static_group_cached_state CachedState = {};

    sml_pipeline_desc *Pipeline = Payload->Pipeline;

     for(u32 Index = 0; Index < Pipeline->ShaderCount; Index++)
     {
         sml_shader_desc *Shader = Pipeline->Shaders + Index;

         switch(Shader->Type)
         {

         case SmlShader_Vertex:
         {
             ID3DBlob *VertexBlob = nullptr;

             CachedState.VShader = SmlDx11_CreateVertexShader(Shader, &VertexBlob);
             CachedState.InputLayout = 
                   SmlDx11_CreateInputLayout(Pipeline->Layout, Pipeline->LayoutCount,
                                             VertexBlob);

             VertexBlob->Release();
         } break;

         case SmlShader_Pixel:
         {
             CachedState.PShader = SmlDx11_CreatePixelShader(Shader);
         } break;

         default:
         {
             Sml_Assert(!"Unknown shader type.");
         } break;

         }
    }

    // TODO: Missing a lot of the data for the cached state.
}

static void 
SmlDx11_Setup(sml_renderer *Renderer)
{

    sml_u8 *CmdPtr = (sml_u8*)Renderer->OfflinePushBase;
    size_t  Offset = 0;

    while(Offset < Renderer->OfflinePushSize)
    {
        auto *Header = (sml_setup_command_header*)(CmdPtr + Offset);
        Offset      += sizeof(sml_setup_command_header);

        switch(Header->Type)
        {

        case SmlSetupCommand_StaticGroup:
        {
            auto *Payload = (sml_setup_command_static_group*)(CmdPtr + Offset);
            SmlDx11_SetupStaticGroup(Payload);
        };

        default:
        {
            Sml_Assert(!"Unknown setup command type.");
            return;
        } break;

        }

        Offset += Header->Size;
    }

    Renderer->OfflinePushSize = 0;
}
