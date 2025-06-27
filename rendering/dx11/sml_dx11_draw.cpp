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
SmlDx11_DrawClearScreen(sml_draw_command_clear *Payload)
{
    Dx11.Context->ClearRenderTargetView(Dx11.RenderView, &Payload->Color.x);
    Dx11.Context->ClearDepthStencilView(Dx11.DepthView,
                                        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                        1, 0);
}

static void
SmlDx11_DrawMeshGroup(sml_draw_command_mesh_group *Payload)
{
    sml_dx11_mesh_group Group = MeshGroups[Payload->Handle];
}

// WARN:
// 1) No pipeline logic binding or anything
// 2) No draw calls
// 3) No topology (Add to pipeline)

static void 
SmlDx11_Playback(sml_renderer *Renderer)
{
    ID3D11DeviceContext *Ctx = Dx11.Context;
    // ===============================================================================
    Ctx->OMSetRenderTargets(1, &Dx11.RenderView, Dx11.DepthView);
    Ctx->RSSetViewports(1, &Dx11.Viewport);

    if(Dx11.CameraBuffer == nullptr)
    {
        HRESULT Status = S_OK;

        D3D11_BUFFER_DESC CBufferDesc = {};
        CBufferDesc.ByteWidth         = sizeof(sml_matrix4);
        CBufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        CBufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
        CBufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

        Status = Dx11.Device->CreateBuffer(&CBufferDesc, nullptr, &Dx11.CameraBuffer);
        Sml_Assert(SUCCEEDED(Status));
    }

    D3D11_MAPPED_SUBRESOURCE Mapped;
    Ctx->Map(Dx11.CameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);

    memcpy(Mapped.pData, &Renderer->CameraData, sizeof(sml_matrix4));
    Ctx->Unmap(Dx11.CameraBuffer, 0);

    Ctx->VSSetConstantBuffers(0, 1, &Dx11.CameraBuffer);

    // ===============================================================================

    sml_u8 *CmdPtr = (sml_u8*)Renderer->RuntimePushBase;
    size_t  Offset = 0;

    while(Offset < Renderer->RuntimePushSize)
    {
        auto *Header = (sml_draw_command_header*)(CmdPtr + Offset);
        Offset      += sizeof(sml_draw_command_header);

        switch(Header->Type)
        {

        case SmlDrawCommand_Clear:
        {
            auto *Payload = (sml_draw_command_clear*)(CmdPtr + Offset);
            SmlDx11_DrawClearScreen(Payload);
        } break;

        case SmlDrawCommand_MeshGroup:
        {
            auto *Payload = (sml_draw_command_mesh_group*)(CmdPtr + Offset);
            SmlDx11_DrawMeshGroup(Payload);
        } break;

        default:
        {
            Sml_Assert(!"Unknown command type.");
            return;
        } break;

        }

        Offset += Header->Size;
    }

    Dx11.SwapChain->Present(1, 0);

    Renderer->RuntimePushSize = 0;
}
