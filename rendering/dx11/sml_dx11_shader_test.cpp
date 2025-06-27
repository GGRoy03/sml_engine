// WARN: Temporary include for simplicity
#include <unordered_map>

// ===================================
// Type Definitions
// ===================================

struct sml_dx11_shader_variant
{
    ID3D11InputLayout  *InputLayout;
    ID3D11VertexShader *VertexShader;
    ID3D11PixelShader  *PixelShader;
    sml_bit_field       FeatureFlags;
};

struct sml_dx11_material
{
    sml_dx11_shader_variant Variant;
    ID3D11Buffer           *ConstantBuffer;
};

enum SmlGPUResource_Type
{
    SmlGPUResource_Immutable,
};

struct sml_dx11_mesh_group
{
    ID3D11Buffer *VertexBuffer;
    ID3D11Buffer *IndexBuffer;

    SmlGPUResource_Type ResourceType;

    size_t  VertexBufferSize;
    size_t  IndexBufferSize;
    sml_u32 IndexCount;
};

// ===================================
// Global Variables
// ===================================

// WARN: These are very messy and only there for prototyping
static std::unordered_map<sml_material_handle, sml_dx11_material>     Materials;
static std::unordered_map<sml_mesh_group_handle, sml_dx11_mesh_group> MeshGroups;

// ===================================
// Internal Helpers
// ===================================

static ID3DBlob*
SmlDx11_CompileShader(const char *ByteCode, size_t ByteCodeSize,
                      const char* EntryPoint, const char *Profile,
                      D3D_SHADER_MACRO* Defines, UINT Flags)
{
    ID3DBlob *ShaderBlob = nullptr;
    ID3DBlob *ErrorBlob  = nullptr;

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
SmlDx11_CreateVertexShader(sml_bit_field Flags, D3D_SHADER_MACRO *Defines,
                           ID3DBlob **OutShaderBlob)
{
    HRESULT Status = S_OK;

    const char  *EntryPoint   = "VSMain";
    const char  *Profile      = "vs_5_0";
    const char  *ByteCode     = SmlUberVertexShader_HLSL;
    const SIZE_T ByteCodeSize = strlen(SmlUberVertexShader_HLSL);

    UINT CompileFlags = Flags & SmlShader_CompileDebug
                            ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
                            : D3DCOMPILE_OPTIMIZATION_LEVEL3;

    *OutShaderBlob = SmlDx11_CompileShader(ByteCode, ByteCodeSize, EntryPoint,
                                           Profile, Defines, CompileFlags);

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

static ID3D11PixelShader*
SmlDx11_CreatePixelShader(sml_bit_field Flags, D3D_SHADER_MACRO *Defines)
{
    HRESULT   Status     = S_OK;
    ID3DBlob *ShaderBlob = nullptr;

    const char  *EntryPoint   = "PSMain";
    const char  *Profile      = "ps_5_0";
    const char  *ByteCode     = SmlUberPixelShader_HLSL;
    const SIZE_T ByteCodeSize = strlen(SmlUberPixelShader_HLSL);

    UINT CompileFlags = Flags & SmlShader_CompileDebug
                            ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
                            : D3DCOMPILE_OPTIMIZATION_LEVEL3;

    ShaderBlob = SmlDx11_CompileShader(ByteCode, ByteCodeSize, EntryPoint,
                                       Profile, Defines, CompileFlags);

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

static void
SmlDx11_GenerateShaderDefines(sml_bit_field Features, D3D_SHADER_MACRO *Defines)
{
    sml_u32 AtEnabled = 0;

    if(Features & SmlShaderFeat_AlbedoMap)
    {
        Defines[AtEnabled].Name       = "HAS_ALBEDO_MAP";
        Defines[AtEnabled].Definition = "1";
        AtEnabled++;
    }
}

static void
SmlDx11_CompileShaderVariant(sml_bit_field Features, sml_bit_field Flags)
{
    sml_dx11_shader_variant Variant = {};

    D3D_SHADER_MACRO Defines[SmlShaderFeat_Count] = {};
    SmlDx11_GenerateShaderDefines(Features, Defines);

    ID3DBlob *VertexBlob = nullptr;
    Variant.VertexShader = SmlDx11_CreateVertexShader(Flags, Defines, &VertexBlob);
    Sml_Unused(VertexBlob);

    Variant.PixelShader = SmlDx11_CreatePixelShader(Flags, Defines);
}

// ===================================
// API Implementation
// ===================================

// BUG: 
// 1) Does not create the input layout.
// 2) Does not create the uniform buffer.

// WARN: This code is a bit messy.

static void
SmlDx11_SetupMaterial(sml_setup_command_material *Payload)
{
    sml_dx11_material Material = {};

    D3D_SHADER_MACRO Defines[SmlShaderFeat_Count] = {};
    SmlDx11_GenerateShaderDefines(Payload->Features, Defines);

    ID3DBlob *VertexBlob = nullptr;
    Material.Variant.VertexShader = SmlDx11_CreateVertexShader(Payload->Flags,
                                                               Defines, &VertexBlob);
    Sml_Unused(VertexBlob);

    Material.Variant.PixelShader = SmlDx11_CreatePixelShader(Payload->Flags, Defines);

    Material.Variant.FeatureFlags = Payload->Features;

    Materials[Payload->Handle] = Material;
}

// WARN:
// 1) This code needs some clean-ups
// 2) Uses malloc/free

static void
SmlDx11_SetupMeshGroup(sml_setup_command_mesh_group *Payload)
{
    sml_dx11_mesh_group Group = {};

    for(u32 Index = 0; Index < Payload->MeshCount; Index++)
    {
        sml_mesh *Mesh = Payload->Meshes + Index;

        Group.VertexBufferSize += Mesh->VertexDataSize;
        Group.IndexBufferSize  += Mesh->IndexDataSize;
        Group.IndexCount       += sml_u32(Mesh->IndexDataSize / sizeof(sml_u32));
    }

    sml_u8 *CPUVertexBuffer = (sml_u8*)malloc(Group.VertexBufferSize);
    sml_u8 *CPUIndexBuffer  = (sml_u8*)malloc(Group.IndexBufferSize);
    size_t  VertexOffset    = 0;
    size_t  IndexOffset     = 0;

    for(u32 Index = 0; Index < Payload->MeshCount; Index++)
    {
        sml_mesh *Mesh = Payload->Meshes + Index;

        memcpy(CPUVertexBuffer + VertexOffset, Mesh->VertexData, Mesh->VertexDataSize);
        memcpy(CPUIndexBuffer  + IndexOffset , Mesh->IndexData , Mesh->IndexDataSize );

        VertexOffset += Mesh->VertexDataSize;
        IndexOffset  += Mesh->IndexDataSize;
    }

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = (UINT)Group.VertexBufferSize;
        Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Desc.Usage     = D3D11_USAGE_IMMUTABLE;

        D3D11_SUBRESOURCE_DATA Data = {};
        Data.pSysMem = CPUVertexBuffer;

        HRESULT Status = Dx11.Device->CreateBuffer(&Desc, &Data, &Group.VertexBuffer);
        Sml_Assert(Status);
    }

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = (UINT)Group.IndexBufferSize;
        Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        Desc.Usage     = D3D11_USAGE_IMMUTABLE;

        D3D11_SUBRESOURCE_DATA Data = {};
        Data.pSysMem = CPUIndexBuffer;

        HRESULT Status = Dx11.Device->CreateBuffer(&Desc, &Data, &Group.IndexBuffer);
        Sml_Assert(Status);
    }

    free(CPUVertexBuffer);
    free(CPUIndexBuffer);

    MeshGroups[Payload->Handle] = Group;
}
