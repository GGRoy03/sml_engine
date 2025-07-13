// ===================================
// Type Definitions
// ===================================

struct dx11_material_texture
{
    ID3D11ShaderResourceView* ResourceView;
    sml_u32                   Slot;
};

struct dx11_material
{
    ID3D11InputLayout        *Layout;
    UINT                      Stride;
    ID3D11VertexShader       *VShader;
    ID3D11PixelShader        *PShader;
    ID3D11ShaderResourceView *Sampled[MaterialType_Count];
    sml_u32                   SampledCount;
    ID3D11SamplerState       *SamplerState;
    ID3D11Buffer             *Constants;
};

struct dx11_geometry
{
    ID3D11Buffer* Vtx;
    ID3D11Buffer* Idx;
    sml_u32       IdxCnt;
};

struct dx11_instance
{
    instance_data Data;
    dx11_geometry Buffers;
    ID3D11Buffer *PerObject;
};

struct dx11_backend
{
    slot_map<dx11_material, material> Materials;
    slot_map<dx11_instance, instance> Instances;
};

// ===================================
// Internal Helpers
// ===================================

static ID3DBlob*
Dx11_CompileShader(const char* ByteCode, size_t ByteCodeSize, const char* EntryPoint,
                   const char* Profile, D3D_SHADER_MACRO* Defines, UINT Flags)
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
Dx11_CreateVtxShader(sml_bit_field Flags, D3D_SHADER_MACRO *Defines,
                     ID3DBlob **OutShaderBlob)
{
    HRESULT Status = S_OK;

    const char*  EntryPoint   = "VSMain";
    const char*  Profile      = "vs_5_0";
    const char*  ByteCode     = UberVertexShader_HLSL;
    const SIZE_T ByteCodeSize = strlen(UberVertexShader_HLSL);

    UINT CompileFlags = 0;
    if(Flags & RenderCommand_DebugShader)
    {
        CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    }
    else
    {
        CompileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
    } 

    *OutShaderBlob = Dx11_CompileShader(ByteCode, ByteCodeSize,
                                        EntryPoint, Profile,
                                        Defines, CompileFlags);

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
Dx11_CreatePxlShader(sml_bit_field Flags, D3D_SHADER_MACRO *Defines)
{
    HRESULT   Status     = S_OK;
    ID3DBlob *ShaderBlob = nullptr;

    const char*  EntryPoint   = "PSMain";
    const char*  Profile      = "ps_5_0";
    const char*  ByteCode     = SmlUberPixelShader_HLSL;
    const SIZE_T ByteCodeSize = strlen(SmlUberPixelShader_HLSL);

    UINT CompileFlags = 0;
    if(Flags & RenderCommand_DebugShader)
    {
        CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    }
    else
    {
        CompileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
    } 

    ShaderBlob = Dx11_CompileShader(ByteCode, ByteCodeSize,
                                    EntryPoint, Profile,
                                    Defines, CompileFlags);

    ID3D11PixelShader* PixelShader = nullptr;
    Status = Dx11.Device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
                                            ShaderBlob->GetBufferSize(),
                                            nullptr, &PixelShader);
    ShaderBlob->Release();

    if (FAILED(Status))
    {
        Sml_Assert(!"SML_DX11: Failed to compile vertex shader.");
        return nullptr;
    }

    return PixelShader;
}

// WARN:
// X) Code is bad.

static ID3D11InputLayout*
Dx11_CreateInputLayout(sml_bit_field Features, ID3DBlob* VertexBlob,
                      UINT* OutStride)
{
    HRESULT Status = S_OK;

    D3D11_INPUT_ELEMENT_DESC Elements[32] = {};

    Elements[0].SemanticName         = "Position";
    Elements[0].SemanticIndex        = 0;
    Elements[0].Format               = DXGI_FORMAT_R32G32B32_FLOAT;
    Elements[0].InputSlot            = 0;
    Elements[0].AlignedByteOffset    = 0;
    Elements[0].InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
    Elements[0].InstanceDataStepRate = 0;

    Elements[1].SemanticName         = "NORMAL";
    Elements[1].SemanticIndex        = 0;
    Elements[1].Format               = DXGI_FORMAT_R32G32B32_FLOAT;
    Elements[1].InputSlot            = 0;
    Elements[1].AlignedByteOffset    = D3D11_APPEND_ALIGNED_ELEMENT;
    Elements[1].InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
    Elements[1].InstanceDataStepRate = 0;

    if(Features & ShaderFeat_Color)
    {
        Elements[2].SemanticName         = "COLOR";
        Elements[2].SemanticIndex        = 0;
        Elements[2].Format               = DXGI_FORMAT_R32G32B32_FLOAT;
        Elements[2].InputSlot            = 0;
        Elements[2].AlignedByteOffset    = D3D11_APPEND_ALIGNED_ELEMENT;
        Elements[2].InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
        Elements[2].InstanceDataStepRate = 0;

        *OutStride = 9 * sizeof(f32);
    }
    else
    {
        Elements[2].SemanticName         = "TEXCOORD";
        Elements[2].SemanticIndex        = 0;
        Elements[2].Format               = DXGI_FORMAT_R32G32_FLOAT;
        Elements[2].InputSlot            = 0;
        Elements[2].AlignedByteOffset    = D3D11_APPEND_ALIGNED_ELEMENT;
        Elements[2].InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
        Elements[2].InstanceDataStepRate = 0;

        *OutStride = 8 * sizeof(f32);
    }

    ID3D11InputLayout* InputLayout = nullptr;
    Status = Dx11.Device->CreateInputLayout(Elements, 3,
                                            VertexBlob->GetBufferPointer(),
                                            VertexBlob->GetBufferSize(),
                                            &InputLayout);
    if (FAILED(Status))
    {
        Sml_Assert(!"Failed to create input layout.");
        return nullptr;
    }

    return InputLayout;
}

// ===================================
// Playback Implementation
// ===================================

static void
Dx11_UpdateMaterial(update_command_material *Payload)
{
    HRESULT Status = S_OK;

    dx11_backend  *Backend  = (dx11_backend*)Renderer->Backend;
    dx11_material *Material = &Backend->Materials[Payload->Material];

    if(!Material->Constants)
    {
        D3D_SHADER_MACRO Defines[ShaderFeat_Count + 1] = {};

        sml_u32       Enabled = 0;
        sml_bit_field Feats   = Renderer->ContextData.ShaderFeatures;

        if (Feats & ShaderFeat_AlbedoMap)
        {
            Defines[Enabled++] = {"HAS_ALBEDO_MAP", "1"};
        }

        if (Feats & ShaderFeat_Instanced)
        {
            Defines[Enabled++] = {"HAS_INSTANCING", "1"};
        }

        if(Feats & ShaderFeat_Color)
        {
            Defines[Enabled++] = {"HAS_COLORS", "1"};
        }

        ID3DBlob *VSBlob  = nullptr;
        Material->VShader = Dx11_CreateVtxShader(Payload->Flags, Defines, &VSBlob);
        Material->PShader = Dx11_CreatePxlShader(Payload->Flags, Defines);
        Material->Layout  = Dx11_CreateInputLayout(Feats, VSBlob, &Material->Stride);

        {
            material_constants Constants = {};
            Constants.AlbedoFactor    = sml_vector3(1.0f, 1.0f, 1.0f);
            Constants.MetallicFactor  = 1.0f;
            Constants.RoughnessFactor = 1.0f;

            D3D11_BUFFER_DESC Desc = {};
            Desc.ByteWidth = sizeof(Constants);
            Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            Desc.Usage     = D3D11_USAGE_IMMUTABLE;

            D3D11_SUBRESOURCE_DATA InitData = {};
            InitData.pSysMem = &Constants;

            Status = Dx11.Device->CreateBuffer(&Desc, &InitData, &Material->Constants);
            Sml_Assert(SUCCEEDED(Status));
        }

        {
            D3D11_SAMPLER_DESC Desc = {};
            Desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            Desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
            Desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
            Desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
            Desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            Desc.MinLOD         = 0;
            Desc.MaxLOD         = D3D11_FLOAT32_MAX;

            Status = Dx11.Device->CreateSamplerState(&Desc, &Material->SamplerState);
            Sml_Assert(SUCCEEDED(Status));
        }
    }

    // BUG: Does not query the correct index ( We do not have a scheme)
    auto **ResourceView = &Material->Sampled[0];
    if(!*ResourceView)
    {
        texture Texture = Payload->Texture;

        D3D11_TEXTURE2D_DESC Desc = {};
        Desc.Width            = Texture.Width;
        Desc.Height           = Texture.Height;
        Desc.MipLevels        = 1;
        Desc.ArraySize        = 1;
        Desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.SampleDesc.Count = 1;
        Desc.Usage            = D3D11_USAGE_IMMUTABLE;
        Desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA InitData = {};
        InitData.pSysMem     = Texture.Pixels;
        InitData.SysMemPitch = Texture.Pitch;

        ID3D11Texture2D *Tex2D = nullptr;
        Status = Dx11.Device->CreateTexture2D(&Desc, &InitData, &Tex2D);
        Sml_Assert(SUCCEEDED(Status));

        Status = Dx11.Device->CreateShaderResourceView(Tex2D, nullptr, ResourceView);
        Sml_Assert(SUCCEEDED(Status));

        Dx11_Release(Tex2D);

        if(Payload->Flags & RenderCommand_FreeHeap)
        {
            SmlMemory.Free(Texture.PixelHeap);
        }

        ++Material->SampledCount;
    }
}

static void
Dx11_UpdateInstance(update_command_instance *Payload)
{
    dx11_backend *Backend = (dx11_backend*)Renderer->Backend;

    dx11_instance *Instance = &Backend->Instances[Payload->Instance];

    if(!Instance->PerObject)
    {
        Sml_Assert(Payload->VtxHeap.Data && Payload->IdxHeap.Data);

        {
            D3D11_BUFFER_DESC Desc = {};
            Desc.ByteWidth      = sizeof(sml_matrix4);
            Desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
            Desc.Usage          = D3D11_USAGE_DYNAMIC;
            Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            auto Status = Dx11.Device->CreateBuffer(&Desc, NULL, &Instance->PerObject);
            Sml_Assert(SUCCEEDED(Status));
        }

        {
            D3D11_BUFFER_DESC Desc = {};
            Desc.ByteWidth = (UINT)Payload->VtxHeap.Size;
            Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            Desc.Usage     = D3D11_USAGE_IMMUTABLE;

            D3D11_SUBRESOURCE_DATA Data = {};
            Data.pSysMem = Payload->VtxHeap.Data;

            auto Status = Dx11.Device->CreateBuffer(&Desc, &Data, &Instance->Buffers.Vtx);

            Sml_Assert(SUCCEEDED(Status));
        }

        {
            D3D11_BUFFER_DESC Desc = {};
            Desc.ByteWidth = (UINT)Payload->IdxHeap.Size;
            Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            Desc.Usage     = D3D11_USAGE_IMMUTABLE;

            D3D11_SUBRESOURCE_DATA Data = {};
            Data.pSysMem = Payload->IdxHeap.Data;

            auto Status = Dx11.Device->CreateBuffer(&Desc, &Data, &Instance->Buffers.Idx);

            Instance->Buffers.IdxCnt = Payload->IdxCount;

            Sml_Assert(SUCCEEDED(Status));
        }

        if(Payload->Flags && RenderCommand_FreeHeap)
        {
            SmlMemory.Free(Payload->VtxHeap);
            SmlMemory.Free(Payload->IdxHeap);
        }
    }

    sml_matrix4 World = SmlMat4_Translation(Payload->Position);

    D3D11_MAPPED_SUBRESOURCE Mapped = {};
    Dx11.Context->Map(Instance->PerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    memcpy(Mapped.pData, &World, sizeof(sml_matrix4));
    Dx11.Context->Unmap(Instance->PerObject, 0);
}

// ===================================
// User API
// ===================================

static material
RequestMaterial()
{
    dx11_backend *Backend = (dx11_backend*)Renderer->Backend;

    material Result = Backend->Materials.GetFreeSlot();

    Sml_Assert(Result != Backend->Materials.Invalid);

    return Result;
}

static instance
RequestInstance()
{
    dx11_backend *Backend = (dx11_backend*)Renderer->Backend;

    instance Result = Backend->Instances.GetFreeSlot();

    Sml_Assert(Result != Backend->Instances.Invalid);

    return Result;
}
