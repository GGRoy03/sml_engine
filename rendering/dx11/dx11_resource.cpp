namespace SML
{

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
    dx11_shader_variant   Variant;
    dx11_material_texture Sampled[SmlMaterial_Count];
    ID3D11SamplerState*   SamplerState;
    ID3D11Buffer*         ConstantBuffer;
};

struct dx11_geometry
{
    ID3D11Buffer* Vtx;
    ID3D11Buffer* Idx;
    sml_u32       IdxCnt;
};

struct dx11_instance
{
    sml_instance_data Data;
    dx11_geometry     Buffers;
    ID3D11Buffer*     PerObject;
};

struct dx11_backend
{
    slot_map<dx11_material, material> Materials;
    slot_map<dx11_instance, instance> Instances;
};

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

            auto Status =
                Dx11.Device->CreateBuffer(&Desc, &Data, &Instance->Buffers.Vtx);

            Sml_Assert(SUCCEEDED(Status));
        }

        {
            D3D11_BUFFER_DESC Desc = {};
            Desc.ByteWidth = (UINT)Payload->IdxHeap.Size;
            Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            Desc.Usage     = D3D11_USAGE_IMMUTABLE;

            D3D11_SUBRESOURCE_DATA Data = {};
            Data.pSysMem = Payload->IdxHeap.Data;

            auto Status =
                Dx11.Device->CreateBuffer(&Desc, &Data, &Instance->Buffers.Idx);

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


} // namespace SML
