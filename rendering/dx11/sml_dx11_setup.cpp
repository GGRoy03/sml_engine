// WARN: Temporary include for simplicity
#include <unordered_map>

// ===================================
// Type Definitions
// ===================================

// NOTE: We might want to consider holding more things in the variant structure.
struct sml_dx11_shader_variant
{
    ID3D11InputLayout* InputLayout;
    UINT               Stride;

    ID3D11VertexShader* VertexShader;
    ID3D11PixelShader* PixelShader;
    sml_bit_field       FeatureFlags;
};

struct sml_dx11_material_texture
{
    ID3D11ShaderResourceView* ResourceView;
    sml_u32                   Slot;
};

struct sml_dx11_material
{
    sml_dx11_shader_variant   Variant;
    sml_dx11_material_texture Sampled[SmlMaterial_Count];
    ID3D11SamplerState* SamplerState;
    ID3D11Buffer* ConstantBuffer;
};

struct sml_mesh_info
{
    // NOTE: These are not yet used.
    sml_material_handle Material;
    sml_u32 IndexCount;

    sml_vector3 Position;
};

struct sml_dx11_mesh_group
{
    sml_material_handle Material;

    ID3D11Buffer* VertexBuffer;
    ID3D11Buffer* IndexBuffer;

    size_t  VertexBufferSize;
    size_t  IndexBufferSize;
    sml_u32 IndexCount;

    // NOTE: These would be arrays which we iterate over
    // to set the materials / constant buffers. We should sort 
    // them correctly as to minimize state changes. Is a group
    // only a single vertex buffer/index buffer?
    sml_u32       MeshCount;
    sml_mesh_info MeshInfo;
    ID3D11Buffer* PerObject;
};

// ===================================
//  Global Variables
// ===================================

static std::unordered_map<sml_material_handle, sml_dx11_material>     Materials;
static std::unordered_map<sml_mesh_group_handle, sml_dx11_mesh_group> MeshGroups;

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

static ID3DBlob*
SmlDx11_CompileShader(const char* ByteCode, size_t ByteCodeSize,
    const char* EntryPoint, const char* Profile,
    D3D_SHADER_MACRO* Defines, UINT Flags)
{
    ID3DBlob* ShaderBlob = nullptr;
    ID3DBlob* ErrorBlob = nullptr;

    HRESULT Status = D3DCompile(ByteCode, ByteCodeSize,
        nullptr, Defines, nullptr,
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

static ID3D11VertexShader*
SmlDx11_CreateVertexShader(sml_bit_field Flags, D3D_SHADER_MACRO* Defines,
    ID3DBlob** OutShaderBlob)
{
    HRESULT Status = S_OK;

    const char* EntryPoint = "VSMain";
    const char* Profile = "vs_5_0";
    const char* ByteCode = SmlUberVertexShader_HLSL;
    const SIZE_T ByteCodeSize = strlen(SmlUberVertexShader_HLSL);

    UINT CompileFlags = Flags & SmlShader_CompileDebug
        ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
        : D3DCOMPILE_OPTIMIZATION_LEVEL3;

    *OutShaderBlob = SmlDx11_CompileShader(ByteCode, ByteCodeSize, EntryPoint,
        Profile, Defines, CompileFlags);

    ID3D11VertexShader* VertexShader = nullptr;
    Status = Dx11.Device->CreateVertexShader((*OutShaderBlob)->GetBufferPointer(),
        (*OutShaderBlob)->GetBufferSize(),
        nullptr, &VertexShader);
    if (FAILED(Status))
    {
        Sml_Assert(!"SML_DX11: Failed to compile vertex shader.");
        return nullptr;
    }

    return VertexShader;
}

static ID3D11PixelShader*
SmlDx11_CreatePixelShader(sml_bit_field Flags, D3D_SHADER_MACRO* Defines)
{
    HRESULT   Status = S_OK;
    ID3DBlob* ShaderBlob = nullptr;

    const char* EntryPoint = "PSMain";
    const char* Profile = "ps_5_0";
    const char* ByteCode = SmlUberPixelShader_HLSL;
    const SIZE_T ByteCodeSize = strlen(SmlUberPixelShader_HLSL);

    UINT CompileFlags = Flags & SmlShader_CompileDebug
        ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
        : D3DCOMPILE_OPTIMIZATION_LEVEL3;

    ShaderBlob = SmlDx11_CompileShader(ByteCode, ByteCodeSize, EntryPoint,
        Profile, Defines, CompileFlags);

    ID3D11PixelShader* PixelShader = nullptr;
    Status = Dx11.Device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
        ShaderBlob->GetBufferSize(),
        nullptr, &PixelShader);
    ShaderBlob->Release();

    if (FAILED(Status))
    {
        Sml_Assert(!"SML_DX11: Failed to compile pixel shader.");
        return nullptr;
    }

    return PixelShader;
}

static void
SmlDx11_GenerateShaderDefines(sml_bit_field Features, D3D_SHADER_MACRO* Defines)
{
    sml_u32 AtEnabled = 0;

    if (Features & SmlShaderFeat_AlbedoMap)
    {
        Defines[AtEnabled].Name = "HAS_ALBEDO_MAP";
        Defines[AtEnabled].Definition = "1";
        AtEnabled++;
    }
}

// NOTE: For now this simply returns the hardcoded layout for the vertex
// shader.
static ID3D11InputLayout*
SmlDx11_CreateInputLayout(ID3DBlob* VertexBlob, UINT* OutStride)
{
    HRESULT Status = S_OK;

    D3D11_INPUT_ELEMENT_DESC Elements[3] =
    {
        {"POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0},

        {"NORMAL", 0 ,DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },

        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11InputLayout* InputLayout = nullptr;
    Status = Dx11.Device->CreateInputLayout(Elements, 3,
        VertexBlob->GetBufferPointer(),
        VertexBlob->GetBufferSize(),
        &InputLayout);
    *OutStride = 8 * sizeof(sml_f32);

    if (FAILED(Status))
    {
        Sml_Assert(!"Failed to create input layout.");
        return nullptr;
    }

    return InputLayout;
}

// ===================================
// API Implementation
// ===================================


static void
SmlDx11_SetupMaterial(sml_setup_command_material* Payload)
{
    HRESULT           Status = S_OK;
    sml_dx11_material Material = {};

    D3D_SHADER_MACRO Defines[SmlShaderFeat_Count + 1] = {};
    SmlDx11_GenerateShaderDefines(Payload->Features, Defines);
    Defines[SmlShaderFeat_Count] = { nullptr, nullptr };

    ID3DBlob* VSBlob = nullptr;
    Material.Variant.VertexShader =
        SmlDx11_CreateVertexShader(Payload->Flags, Defines, &VSBlob);
    Material.Variant.InputLayout =
        SmlDx11_CreateInputLayout(VSBlob, &Material.Variant.Stride);

    Material.Variant.PixelShader =
        SmlDx11_CreatePixelShader(Payload->Flags, Defines);

    for (sml_u32 Index = 0; Index < Payload->MaterialTextureCount; ++Index)
    {
        sml_material_texture* SrcTex = Payload->MaterialTextureArray + Index;

        D3D11_TEXTURE2D_DESC TexDesc = {};
        TexDesc.Width = SrcTex->Width;
        TexDesc.Height = SrcTex->Height;
        TexDesc.MipLevels = 1;
        TexDesc.ArraySize = 1;
        TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        TexDesc.SampleDesc.Count = 1;
        TexDesc.Usage = D3D11_USAGE_IMMUTABLE;
        TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA InitData = {};
        InitData.pSysMem = SrcTex->Pixels;
        InitData.SysMemPitch = SrcTex->Pitch;
        InitData.SysMemSlicePitch = 0;

        ID3D11Texture2D* Tex2D = nullptr;
        Status = Dx11.Device->CreateTexture2D(&TexDesc, &InitData, &Tex2D);
        Sml_Assert(SUCCEEDED(Status) && Tex2D);

        ID3D11ShaderResourceView** View = &Material.Sampled[Index].ResourceView;
        Status = Dx11.Device->CreateShaderResourceView(Tex2D, nullptr, View);
        Sml_Assert(SUCCEEDED(Status));

        Material.Sampled[Index].Slot = MATERIAL_TYPE_TO_SLOT(SrcTex->MaterialType);

        if (Tex2D)
        {
            Tex2D->Release();
        }

        stbi_image_free(SrcTex->Pixels);
    }

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = sizeof(sml_material_constants);
        Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Desc.Usage = D3D11_USAGE_IMMUTABLE;

        sml_material_constants Constants = {};
        Constants.AlbedoFactor = sml_vector3(1.0f, 1.0f, 1.0f);
        Constants.MetallicFactor = 1.0f;
        Constants.RoughnessFactor = 1.0f;

        D3D11_SUBRESOURCE_DATA InitData = {};
        InitData.pSysMem = &Constants;

        Status = Dx11.Device->CreateBuffer(&Desc, &InitData, &Material.ConstantBuffer);
        Sml_Assert(SUCCEEDED(Status));
    }

    D3D11_SAMPLER_DESC SampDesc = {};
    SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SampDesc.MinLOD = 0;
    SampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    Status = Dx11.Device->CreateSamplerState(&SampDesc, &Material.SamplerState);
    Sml_Assert(SUCCEEDED(Status));

    Material.Variant.FeatureFlags = Payload->Features;

    Materials[Payload->Handle] = Material;
}

// WARN:
// 1) This code needs some clean-ups
// 2) Uses malloc/free

static void
SmlDx11_SetupMeshGroup(sml_setup_command_mesh_group* Payload)
{
    Sml_Assert(Payload->Meshes);

    sml_dx11_mesh_group Group = {};

    for (u32 Index = 0; Index < Payload->MeshCount; Index++)
    {
        sml_mesh* Mesh = Payload->Meshes + Index;

        Group.VertexBufferSize += Mesh->VertexDataSize;
        Group.IndexBufferSize += Mesh->IndexDataSize;
        Group.IndexCount += sml_u32(Mesh->IndexDataSize / sizeof(sml_u32));
    }

    sml_u8* CPUVertexBuffer = (sml_u8*)malloc(Group.VertexBufferSize);
    sml_u8* CPUIndexBuffer = (sml_u8*)malloc(Group.IndexBufferSize);
    size_t  VertexOffset = 0;
    size_t  IndexOffset = 0;

    for (u32 Index = 0; Index < Payload->MeshCount; Index++)
    {
        sml_mesh* Mesh = Payload->Meshes + Index;

        memcpy(CPUVertexBuffer + VertexOffset, Mesh->VertexData, Mesh->VertexDataSize);
        memcpy(CPUIndexBuffer + IndexOffset, Mesh->IndexData, Mesh->IndexDataSize);

        VertexOffset += Mesh->VertexDataSize;
        IndexOffset += Mesh->IndexDataSize;
    }

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = (UINT)Group.VertexBufferSize;
        Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Desc.Usage = D3D11_USAGE_IMMUTABLE;

        D3D11_SUBRESOURCE_DATA Data = {};
        Data.pSysMem = CPUVertexBuffer;

        HRESULT Status = Dx11.Device->CreateBuffer(&Desc, &Data, &Group.VertexBuffer);
        Sml_Assert(SUCCEEDED(Status));
    }

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = (UINT)Group.IndexBufferSize;
        Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        Desc.Usage = D3D11_USAGE_IMMUTABLE;

        D3D11_SUBRESOURCE_DATA Data = {};
        Data.pSysMem = CPUIndexBuffer;

        HRESULT Status = Dx11.Device->CreateBuffer(&Desc, &Data, &Group.IndexBuffer);
        Sml_Assert(SUCCEEDED(Status));
    }

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = sizeof(sml_matrix4);
        Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT Status = Dx11.Device->CreateBuffer(&Desc, nullptr, &Group.PerObject);
        Sml_Assert(SUCCEEDED(Status));

        // TEST: Try with some simple hardcoded values
        Group.MeshCount = 1;
        Group.MeshInfo.Position = sml_vector3(0.0f, 0.0f, 0.0f);
        Group.Material = Payload->Material;
    }

    free(CPUVertexBuffer);
    free(CPUIndexBuffer);

    MeshGroups[Payload->Handle] = Group;
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

        case SmlSetupCommand_Material:
        {
            auto *Payload = (sml_setup_command_material*)(CmdPtr + Offset);
            SmlDx11_SetupMaterial(Payload);
        } break;

        case SmlSetupCommand_MeshGroup:
        {
            auto *Payload = (sml_setup_command_mesh_group*)(CmdPtr + Offset);
            SmlDx11_SetupMeshGroup(Payload);
        } break;

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
