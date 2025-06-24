// ===================================
// Type Definitions
// ===================================

struct sml_dx11_vbuffer
{
    ID3D11Buffer *Handle;
    size_t        Size;
};

struct sml_dx11_ibuffer
{
    ID3D11Buffer *Handle;
    size_t        Size;
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
    ID3D11Buffer             *Handle;
};

struct sml_dx11_pipeline
{
    ID3D11VertexShader *VShader;
    ID3D11PixelShader  *PShader;
    ID3D11InputLayout  *InputLayout;

    UINT Stride;

    sml_dx11_vbuffer VertexBuffer;
    sml_dx11_ibuffer IndexBuffer;

    sml_dx11_pbr_material Material;
};

// ===================================
//  Global variables
// ===================================


// ===================================
//  Internal Helpers
// ===================================

static const char*
SmlDx11_GetShaderProfile(SmlShader_Type Type)
{
    switch(Type)
    {

    case SmlShader_Vertex: return "vs_5_0";
    case SmlShader_Pixel : return "ps_5_0";

    }
}

static void
SmlDx11_BindPipeline(sml_dx11_pipeline *Pipeline)
{
    Sml_Assert(Pipeline->InputLayout);
    Sml_Assert(Pipeline->VShader);
    Sml_Assert(Pipeline->PShader);
    Sml_Assert(Pipeline->Material.Handle);
    Sml_Assert(Pipeline->Material.SamplerState);

    ID3D11DeviceContext *Context = Dx11.Context;

    UINT Stride = Pipeline->Stride;
    UINT Offset = 0;

    // IA
    Context->IASetInputLayout(Pipeline->InputLayout);
    Context->IASetIndexBuffer(Pipeline->IndexBuffer.Handle, DXGI_FORMAT_R32_UINT, 0);
    Context->IASetVertexBuffers(0, 1, &Pipeline->VertexBuffer.Handle,
                                &Stride, &Offset);

    // VS
    Context->VSSetShader(Pipeline->VShader, nullptr, 0);

    // PS
    sml_dx11_pbr_material *Material = &Pipeline->Material;
    for(u32 Index = 0; Index < SmlMaterial_Count; Index++)
    {
        sml_dx11_sampled_material *Sampled = Material->Sampled + Index;
        ID3D11ShaderResourceView  *View    = Sampled->Texture.View;

        if(View)
        {
            Context->PSSetShaderResources(Sampled->Texture.Slot, 1, &View);
        }
    }

    Context->PSSetConstantBuffers(1, 1, &Material->Handle);
    Context->PSSetSamplers(0, 1, &Material->SamplerState);
    Context->PSSetShader(Pipeline->PShader, nullptr, 0);
}

static void
SmlDx11_CompilePipelineShaders(sml_pipeline_desc Desc, sml_dx11_pipeline *Pipeline)
{
    HRESULT   Status     = S_OK;
    ID3DBlob *ShaderBlob = nullptr;
    ID3DBlob *ErrorBlob  = nullptr;

    for (u32 Index = 0; Index < Desc.ShaderCount; ++Index)
    {
        const sml_shader_desc *ShaderDesc = Desc.Shaders + Index;
        const char            *EntryPoint = ShaderDesc->Info.Buffer.EntryPoint;
        const char            *Profile    = SmlDx11_GetShaderProfile(ShaderDesc->Type);

        UINT CompileFlagsConstants = 
                ShaderDesc->Flags & SmlShader_Debug
                    ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
                    : D3DCOMPILE_OPTIMIZATION_LEVEL3;

        if (ShaderDesc->Flags & SmlShader_CompileFromBuffer)
        {
            Status = D3DCompile(ShaderDesc->Info.Buffer.ByteCode,
                                (SIZE_T)ShaderDesc->Info.Buffer.Size,
                                nullptr, nullptr, nullptr,
                                EntryPoint, Profile,
                                CompileFlagsConstants, 0,
                                &ShaderBlob, &ErrorBlob);
        }
        else
        {
        }

        if (FAILED(Status))
        {
            if (ErrorBlob)
            {
                OutputDebugStringA("Shader compiler error: ");
                OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
                OutputDebugStringA("\n");
                ErrorBlob->Release();
            }

            Sml_Assert(!"Failed to compile shader.");
            return;
        }

        switch (ShaderDesc->Type)
        {

        case SmlShader_Vertex:
        {
            Status = Dx11.Device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
                                                     ShaderBlob->GetBufferSize(),
                                                     nullptr,
                                                     &Pipeline->VShader);
            Sml_Assert(SUCCEEDED(Status));

            D3D11_INPUT_ELEMENT_DESC Elements[16] = {};
            UINT                     Stride       = 0;

            for (u32 Input = 0; Input < Desc.LayoutCount; ++Input)
            {
                auto                     *Layout  = Desc.Layout + Input;
                D3D11_INPUT_ELEMENT_DESC *Element = Elements    + Input;

                Element->SemanticName      = Layout->Name;
                Element->SemanticIndex     = 0;
                Element->Format            = SmlDx11_GetDXGIFormat(Layout->Format);
                Element->InputSlot         = Layout->Slot;
                Element->AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                Element->InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;

                size_t ElementSize = Sml_GetSizeOfDataType(Layout->Format);
                Stride            += (UINT)ElementSize;
            }

            Status = Dx11.Device->CreateInputLayout(Elements, Desc.LayoutCount,
                                                    ShaderBlob->GetBufferPointer(),
                                                    ShaderBlob->GetBufferSize(),
                                                    &Pipeline->InputLayout);
            Pipeline->Stride = Stride;
        } break;

        case SmlShader_Pixel:
        {
            Status = Dx11.Device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
                                                    ShaderBlob->GetBufferSize(),
                                                    nullptr,
                                                    &Pipeline->PShader);
        } break;

        default:
            Sml_Assert(!"Unknown shader type. Cannot compile");
            break;
        }

        Sml_Assert(SUCCEEDED(Status));

        ShaderBlob->Release();
        ShaderBlob = nullptr;
    }
}

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

// ===================================
//  User API
// ===================================

// WARN: 
// 1) Does not handle compiler flags.
// 2) Does not handle instanced pipelines
// 3) Does not handle semantic indices
// 4) We malloc the pipeline instead of using an allocator

static sml_dx11_pipeline*
SmlDx11_CreatePipeline(sml_pipeline_desc PipelineDesc)
{
    sml_dx11_pipeline *Pipeline = (sml_dx11_pipeline*)malloc(sizeof(sml_dx11_pipeline));
    HRESULT            Status   = S_OK;

    SmlDx11_CompilePipelineShaders(PipelineDesc, Pipeline);

    D3D11_BUFFER_DESC VDesc = {};
    VDesc.Usage             = D3D11_USAGE_DYNAMIC;
    VDesc.ByteWidth         = Kilobytes(1);
    VDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
    VDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

    Status = Dx11.Device->CreateBuffer(&VDesc, nullptr, &Pipeline->VertexBuffer.Handle);
    Sml_Assert(SUCCEEDED(Status));

    D3D11_BUFFER_DESC IDesc = {};
    IDesc.Usage             = D3D11_USAGE_DYNAMIC;
    IDesc.ByteWidth         = Kilobytes(1);
    IDesc.BindFlags         = D3D11_BIND_INDEX_BUFFER;
    IDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

    Status = Dx11.Device->CreateBuffer(&IDesc, nullptr, &Pipeline->IndexBuffer.Handle);
    Sml_Assert(SUCCEEDED(Status));

    return Pipeline;
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

    Status = Dx11.Device->CreateBuffer(&CDesc, &ConstantData, &Material.Handle);
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
