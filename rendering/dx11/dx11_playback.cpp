namespace SML 
{

// ===================================
// Type Definitions
// ===================================

struct dx11_shader_variant
{
    ID3D11InputLayout* InputLayout;
    UINT               Stride;

    ID3D11VertexShader* VertexShader;
    ID3D11PixelShader*  PixelShader;
    sml_bit_field       FeatureFlags;
};

struct dx11_material_texture
{
    ID3D11ShaderResourceView* ResourceView;
    sml_u32                   Slot;
};

struct dx11_material
{
    dx11_shader_variant       Variant;
    dx11_material_texture     Sampled[SmlMaterial_Count];
    ID3D11SamplerState*       SamplerState;
    ID3D11Buffer*             ConstantBuffer;
};

struct dx11_geometry
{
    ID3D11Buffer* VertexBuffer;
    ID3D11Buffer* IndexBuffer;
    sml_u32       IndexCount;
};

struct dx11_instance
{
    sml_instance_data Data;
    dx11_geometry     Geometry;
    ID3D11Buffer*     PerObject;
};

struct dx11_instanced
{
    sml_u32                   Count;
    sml_u32                   Material;
    sml_matrix4*              CPUMapped;
    dx11_geometry             Geometry;
    ID3D11Buffer*             Buffer;
    ID3D11ShaderResourceView* ResourceView;
};

// ===================================
// Internal Helpers
// ===================================

static ID3D11ShaderResourceView*
Dx11_LoadTextureFromPath(const char* FileName)
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
Dx11_CreateVertexShader(sml_bit_field Flags, D3D_SHADER_MACRO* Defines,
                        ID3DBlob** OutShaderBlob)
{
    HRESULT Status = S_OK;

    const char*  EntryPoint   = "VSMain";
    const char*  Profile      = "vs_5_0";
    const char*  ByteCode     = SmlUberVertexShader_HLSL;
    const SIZE_T ByteCodeSize = strlen(SmlUberVertexShader_HLSL);

    UINT CompileFlags = Flags & SmlShader_CompileDebug
        ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
        : D3DCOMPILE_OPTIMIZATION_LEVEL3;

    *OutShaderBlob = Dx11_CompileShader(ByteCode, ByteCodeSize, EntryPoint,
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
Dx11_CreatePixelShader(sml_bit_field Flags, D3D_SHADER_MACRO* Defines)
{
    HRESULT   Status = S_OK;
    ID3DBlob* ShaderBlob = nullptr;

    const char*  EntryPoint   = "PSMain";
    const char*  Profile      = "ps_5_0";
    const char*  ByteCode     = SmlUberPixelShader_HLSL;
    const SIZE_T ByteCodeSize = strlen(SmlUberPixelShader_HLSL);

    UINT CompileFlags = Flags & SmlShader_CompileDebug
        ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
        : D3DCOMPILE_OPTIMIZATION_LEVEL3;

    ShaderBlob = Dx11_CompileShader(ByteCode, ByteCodeSize, EntryPoint,
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
Dx11_GenerateShaderDefines(sml_bit_field Features, D3D_SHADER_MACRO* Defines)
{
    sml_u32 AtEnabled = 0;

    if (Features & SmlShaderFeat_AlbedoMap)
    {
        Defines[AtEnabled].Name = "HAS_ALBEDO_MAP";
        Defines[AtEnabled].Definition = "1";
        AtEnabled++;
    }

    if (Features & SmlShaderFeat_Instanced)
    {
        Defines[AtEnabled].Name       = "HAS_INSTANCING";
        Defines[AtEnabled].Definition = "1";
        AtEnabled++;
    }

    if(Features & SmlShaderFeat_Color)
    {
        Defines[AtEnabled].Name       = "HAS_COLORS";
        Defines[AtEnabled].Definition = "1";
    }
}

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

    if(Features & SmlShaderFeat_Color)
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

static void
Dx11_CreateMaterial(setup_command_material* Payload)
{
    HRESULT       Status = S_OK;
    dx11_material Material = {};

    D3D_SHADER_MACRO Defines[SmlShaderFeat_Count + 1] = {};
    Dx11_GenerateShaderDefines(Payload->Features, Defines);
    Defines[SmlShaderFeat_Count] = { nullptr, nullptr };

    ID3DBlob* VSBlob = nullptr;
    Material.Variant.VertexShader =
        Dx11_CreateVertexShader(Payload->Flags, Defines, &VSBlob);

    Material.Variant.InputLayout =
        Dx11_CreateInputLayout(Payload->Features, VSBlob,
                                  &Material.Variant.Stride);

    Material.Variant.PixelShader =
        Dx11_CreatePixelShader(Payload->Flags, Defines);

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

    SmlInt_PushToBackendResource(&Renderer->Materials, &Material,
                                 Payload->MaterialIndex);
}

static void
Dx11_CreateInstance(setup_command_instance* Payload)
{
    dx11_instance Instance = {};
    Instance.Data.Material = Payload->Material;

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth      = sizeof(sml_matrix4);
        Desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        Desc.Usage          = D3D11_USAGE_DYNAMIC;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT Status = Dx11.Device->CreateBuffer(&Desc, NULL, &Instance.PerObject);
        Sml_Assert(SUCCEEDED(Status));
    }

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = (UINT)Payload->VtxHeapData.Size;
        Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Desc.Usage     = D3D11_USAGE_IMMUTABLE;

        D3D11_SUBRESOURCE_DATA Data = {};
        Data.pSysMem = Payload->VtxHeapData.Data;

        HRESULT Status = 
            Dx11.Device->CreateBuffer(&Desc, &Data, &Instance.Geometry.VertexBuffer);

        Sml_Assert(SUCCEEDED(Status));
    }

    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = (UINT)Payload->IdxHeapData.Size;
        Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        Desc.Usage     = D3D11_USAGE_IMMUTABLE;

        D3D11_SUBRESOURCE_DATA Data = {};
        Data.pSysMem = Payload->IdxHeapData.Data;

        HRESULT Status = 
            Dx11.Device->CreateBuffer(&Desc, &Data, &Instance.Geometry.IndexBuffer);

        Instance.Geometry.IndexCount = Payload->IdxCount;

        Sml_Assert(SUCCEEDED(Status));
    }

    if(Payload->Flags & Command_InstanceFreeHeap)
    {
        SmlMemory.Free(Payload->VtxHeapData);
        SmlMemory.Free(Payload->IdxHeapData);
    }

    SmlInt_PushToBackendResource(&Renderer->Instances, &Instance,
                                 sml_u32(Payload->Instance));
}

static void
Dx11_UpdateInstanceData(update_command_instance_data* Payload)
{
    dx11_instance *Instance = (dx11_instance*)
        SmlInt_GetBackendResource(&Renderer->Instances, (sml_u32)Payload->Instance);

    sml_matrix4 World = SmlMat4_Translation(Payload->Data);

    D3D11_MAPPED_SUBRESOURCE Mapped = {};
    Dx11.Context->Map(Instance->PerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    memcpy(Mapped.pData, &World, sizeof(sml_matrix4));
    Dx11.Context->Unmap(Instance->PerObject, 0);
}

static void
Dx11_UpdateInstancedData(update_command_instanced_data* Payload)
{
    dx11_instanced *Instanced = (dx11_instanced*)
        SmlInt_GetBackendResource(&Renderer->Instanced, (sml_u32)Payload->Instanced);

    size_t DataSize = Instanced->Count * sizeof(sml_matrix4);

    for (u32 Index = 0; Index < Instanced->Count; Index++)
    {
        Instanced->CPUMapped[Index] = SmlMat4_Translation(Payload->Data[Index]);
    }

    D3D11_MAPPED_SUBRESOURCE Mapped = {};
    Dx11.Context->Map(Instanced->Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    memcpy(Mapped.pData, Instanced->CPUMapped, DataSize);
    Dx11.Context->Unmap(Instanced->Buffer, 0);
}

static void
Dx11_DrawClearScreen(draw_command_clear* Payload)
{
    Dx11.Context->ClearRenderTargetView(Dx11.RenderView, &Payload->Color.x);
    Dx11.Context->ClearDepthStencilView(Dx11.DepthView,
                                        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                        1, 0);
}

static void
Dx11_DrawInstance(draw_command_instance* Payload)
{
    dx11_instance *Instance = (dx11_instance*)
        SmlInt_GetBackendResource(&Renderer->Instances, (sml_u32)Payload->Instance);

    dx11_material *Material = (dx11_material*)
        SmlInt_GetBackendResource(&Renderer->Materials,
                                  (sml_u32)Instance->Data.Material);

    ID3D11DeviceContext *Ctx    = Dx11.Context;
    UINT                 Offset = 0;

    Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Ctx->IASetInputLayout(Material->Variant.InputLayout);
    Ctx->IASetVertexBuffers(0, 1, &Instance->Geometry.VertexBuffer,
                            &Material->Variant.Stride, &Offset);
    Ctx->IASetIndexBuffer(Instance->Geometry.IndexBuffer, DXGI_FORMAT_R32_UINT ,0);

    Ctx->VSSetShader(Material->Variant.VertexShader, nullptr, 0);
    Ctx->VSSetConstantBuffers(1, 1, &Instance->PerObject);

    Ctx->PSSetConstantBuffers(2, 1, &Material->ConstantBuffer);
    Ctx->PSSetSamplers(0, 1, &Material->SamplerState);
    Ctx->PSSetShader(Material->Variant.PixelShader, nullptr, 0);

    for (u32 Index = 0; Index < SmlMaterial_Count; Index++)
    {
        ID3D11ShaderResourceView* View = Material->Sampled[Index].ResourceView;

        if (View)
        {
            Ctx->PSSetShaderResources(0, 1, &View);
        }
    }

    Ctx->DrawIndexed(Instance->Geometry.IndexCount, 0, 0);
}

static void
Dx11_DrawInstanced(draw_command_instanced* Payload)
{
    dx11_instanced *Instanced = (dx11_instanced*)
        SmlInt_GetBackendResource(&Renderer->Instanced, (sml_u32)Payload->Instanced);

    dx11_material *Material = (dx11_material*)
        SmlInt_GetBackendResource(&Renderer->Materials, (sml_u32)Instanced->Material);

    ID3D11DeviceContext *Ctx    = Dx11.Context;
    UINT                 Offset = 0;

    Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Ctx->IASetInputLayout(Material->Variant.InputLayout);
    Ctx->IASetVertexBuffers(0, 1, &Instanced->Geometry.VertexBuffer,
                            &Material->Variant.Stride, &Offset);
    Ctx->IASetIndexBuffer(Instanced->Geometry.IndexBuffer, DXGI_FORMAT_R32_UINT ,0);

    Ctx->VSSetShader(Material->Variant.VertexShader, nullptr, 0);
    Ctx->VSSetShaderResources(10, 1, &Instanced->ResourceView);

    Ctx->PSSetConstantBuffers(2, 1, &Material->ConstantBuffer);
    Ctx->PSSetSamplers(0, 1, &Material->SamplerState);
    Ctx->PSSetShader(Material->Variant.PixelShader, nullptr, 0);

    for (u32 Index = 0; Index < SmlMaterial_Count; Index++)
    {
        ID3D11ShaderResourceView* View = Material->Sampled[Index].ResourceView;

        if (View)
        {
            Ctx->PSSetShaderResources(0, 1, &View);
        }
    }

    Ctx->DrawIndexedInstanced(Instanced->Geometry.IndexCount, Instanced->Count,
                              0, 0, 0);
}

static void
Dx11_Playback()
{
    ID3D11DeviceContext *Ctx = Dx11.Context;

    // =============================================================
    Ctx->OMSetRenderTargets(1, &Dx11.RenderView, Dx11.DepthView);
    Ctx->RSSetViewports(1, &Dx11.Viewport);

    if(Dx11.CameraBuffer == nullptr)
    {
        D3D11_BUFFER_DESC CBufferDesc = {};
        CBufferDesc.ByteWidth      = sizeof(sml_matrix4);
        CBufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        CBufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
        CBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT Status = Dx11.Device->CreateBuffer(&CBufferDesc, nullptr,
                                                   &Dx11.CameraBuffer);
        Sml_Assert(SUCCEEDED(Status));
    }

    D3D11_MAPPED_SUBRESOURCE Mapped;
    Ctx->Map(Dx11.CameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    memcpy(Mapped.pData, &Renderer->CameraData, sizeof(sml_matrix4));
    Ctx->Unmap(Dx11.CameraBuffer, 0);
    Ctx->VSSetConstantBuffers(0, 1, &Dx11.CameraBuffer);

    // =============================================================

    sml_u8   *CmdPtr = (sml_u8*)Renderer->CommandPushBase;
    size_t    Offset = 0;

    while(Offset < Renderer->CommandPushSize)
    {
        command_header *Header = (command_header*)(CmdPtr + Offset);
        Offset += sizeof(command_header);

        switch(Header->Type)
        {
        case Command_Material:
        {
            auto *Payload = (setup_command_material*)(CmdPtr + Offset);
            Dx11_CreateMaterial(Payload);
        } break;

        case Command_Instance:
        {
            auto *Payload = (setup_command_instance*)(CmdPtr + Offset);
            Dx11_CreateInstance(Payload);
        } break;

        case Command_InstanceData:
        {
            auto *Payload = (update_command_instance_data*)(CmdPtr + Offset);
            Dx11_UpdateInstanceData(Payload);
        } break;

        case Command_InstancedData:
        {
            auto *Payload = (update_command_instanced_data*)(CmdPtr + Offset);
            Dx11_UpdateInstancedData(Payload);
        } break;

        case Command_Clear:
        {
            auto *Payload = (draw_command_clear*)(CmdPtr + Offset);
            Dx11_DrawClearScreen(Payload);
        } break;

        case Command_DrawInstance:
        {
            auto *Payload = (draw_command_instance*)(CmdPtr + Offset);
            Dx11_DrawInstance(Payload);
        } break;

        case Command_DrawInstanced:
        {
            auto *Payload = (draw_command_instanced*)(CmdPtr + Offset);
            Dx11_DrawInstanced(Payload);
        } break;

        default:
        {
            Sml_Assert(!"Unknown command type OR command !implemented by DirectX11");
            return;
        }
        }

        Offset += Header->Size;
    }

    // TODO: Make this a command?
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    Dx11.SwapChain->Present(1, 0);

    Renderer->CommandPushSize = 0;
}

} // namespace SML
