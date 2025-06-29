// ===================================
// Type Definitions
// ===================================

// ===================================
//  Global Variables
// ===================================

// ===================================
// Internal Helpers
// ===================================

// ===================================
// API Implementation
// ===================================

static void
SmlDx11_UpdateInstanceData(sml_update_command_instance_data *Payload,
                            sml_renderer *Renderer)
{
    sml_dx11_instance *Instance = (sml_dx11_instance*)
        SmlInt_GetBackendResource(&Renderer->Instances, (sml_u32)Payload->Instance);

    sml_matrix4 World = SmlMat4_Translation(Payload->Data);

    D3D11_MAPPED_SUBRESOURCE Mapped = {};
    Dx11.Context->Map(Instance->PerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    memcpy(Mapped.pData, &World, sizeof(sml_matrix4));
    Dx11.Context->Unmap(Instance->PerObject, 0);
}

static void
SmlDx11_UpdateInstancedData(sml_update_command_instanced_data * Payload,
                            sml_renderer *Renderer)
{
    sml_dx11_instanced *Instanced = (sml_dx11_instanced*)
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
SmlDx11_Update(sml_renderer *Renderer)
{
    sml_u8 *CmdPtr = (sml_u8*)Renderer->UpdatePushBase;
    size_t  Offset = 0;

    while(Offset < Renderer->UpdatePushSize)
    {
        auto *Header = (sml_update_command_header*)(CmdPtr + Offset);
        Offset      += sizeof(sml_update_command_header);

        switch(Header->Type)
        {

        case SmlUpdateCommand_InstanceData:
        {
            auto *Payload = (sml_update_command_instance_data*)(CmdPtr + Offset);
            SmlDx11_UpdateInstanceData(Payload, Renderer);
        } break;

        case SmlUpdateCommand_InstancedData:
        {
            auto *Payload = (sml_update_command_instanced_data*)(CmdPtr + Offset);
            SmlDx11_UpdateInstancedData(Payload, Renderer);
        } break;

        default:
        {
            Sml_Assert(!"Unknown update command type.");
            return;
        } break;

        }

        Offset += Header->Size;
    }

    Renderer->UpdatePushSize = 0;
}
