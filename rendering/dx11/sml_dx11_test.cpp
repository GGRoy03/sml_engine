// ===================================
//  Type Definitions
// ===================================

struct sml_dx11_static_mesh
{
    ID3D11Buffer *VertexBuffer;
    ID3D11Buffer *IndexBuffer;
};

struct sml_dx11_static_group
{
    sml_dx11_static_mesh Meshes[4];
    sml_u32              MeshCount;

    SmlTopology_Type Topology;
    UINT             Stride;

    ID3D11VertexShader *VShader;
    ID3D11PixelShader  *PShader;
    ID3D11InputLayout  *InputLayout;

    sml_dx11_vbuffer VertexBuffer;
    sml_dx11_ibuffer IndexBuffer;

    size_t TotalVertexSize;
    size_t TotalIndexSize;
};

// ===================================
//  User API Implementation
// ===================================

// WARN: 
// 1) This uses the same code as SmlDx11_CompilePipelineShaders
// 2) Should probably return a pointer

static sml_dx11_static_group
SmlDx11_BeginStaticGroup(sml_pipeline_desc Desc)
{

    sml_dx11_static_pipeline Pipeline = {};

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
        }
    }

    return Pipeline;
}

static void
SmlDx11_AttachMeshToStaticGroup(sml_dx11_static_pipeline *Pipeline, 
                                ml_static_mesh Mesh)
{
    Pipeline->TotalVertexSize              += Mesh.VertexDataSize;
    Pipeline->TotalIndexSize               += Mesh.IndexDataSize;
    Pipeline->Meshes[Pipeline->MeshCount++] = Mesh;
}


// WARN: Uses malloc instead of the engine's allocator.

static void
SmlDx11_EndStaticGroup(sml_dx11_static_group *Group)
{
    D3D11_BUFFER_DESC VDesc = {};
    VDesc.Usage             = D3D11_USAGE_IMMUTABLE;
    VDesc.ByteWidth         = Group->TotalVertexSize;
    VDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_BUFFER_DESC IDesc = {};
    IDesc.Usage             = D3D11_USAGE_IMMUTABLE;
    IDesc.ByteWidth         = Group->TotalIndexSize;
    IDesc.BindFlags         = D3D11_BIND_INDEX_BUFFER;


    void *CPUVertexBuffer = malloc(Group->TotalVertexSize);
    void *CPUIndexBuffer  = malloc(Group->TotalIndexSize);

    for(u32 Index = 0; Index < Group->MeshCount; Index++)
    {
        sml_static_mesh *Mesh = Group->Meshes + Index;

        memcpy(CPUVertexBuffer, Mesh->VertexData, Mesh->VertexDataSize);
        memcpy(CPUIndexBuffer , Mesh->Indexdata , Mesh->IndexDataSize );
    }

    D3D11_SUBRESOURCE_DATA VertexData;
    VertexData.pSysMem = CPUVertexBuffer;

    D3D11_SUBRESOURCE_DATA IndexData;
    IndexData.pSysMem = CPUIndexBuffer;

    HRESULT Status = S_OK;

    Status = Dx11.Context->CreateBuffer(&VDesc, &VertexData,
                                        &Group->VertexBuffer.Handle);
    Sml_Assert(Status);

    Status = Dx11.Context->CreateBuffer(&IDesc, &Indexdata,
                                        &Group->IndexBuffer.Handle);
    Sml_Assert(Status);
}
