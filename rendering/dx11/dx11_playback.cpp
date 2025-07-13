// ===================================
// Type Definitions
// ===================================

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

static inline void
Dx11_SetMaterialContext(context_command_material *Payload)
{
    dx11_backend *Backend = (dx11_backend*)Renderer->Backend;

    Backend->MaterialContext.Handle   = Payload->Material;
    Backend->MaterialContext.Features = Payload->ShaderFeatures;
    Backend->MaterialContext.Cached   = {};
}

static void
Dx11_UpdateCamera(update_command_camera *Payload)
{
    ID3D11DeviceContext *Ctx = Dx11.Context;

    if(Dx11.FrameBuffer == nullptr)
    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth      = sizeof(frame_rendering_data);
        Desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        Desc.Usage          = D3D11_USAGE_DYNAMIC;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT Status = Dx11.Device->CreateBuffer(&Desc, nullptr, &Dx11.FrameBuffer);
        Sml_Assert(SUCCEEDED(Status));
    }

    D3D11_MAPPED_SUBRESOURCE Mapped;
    Ctx->Map(Dx11.FrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    memcpy(Mapped.pData, &Payload->ViewProjection, sizeof(frame_rendering_data));
    Ctx->Unmap(Dx11.FrameBuffer, 0);
    Ctx->VSSetConstantBuffers(0, 1, &Dx11.FrameBuffer);
}

static void
Dx11_DrawClearScreen(draw_command_clear* Payload)
{
    Dx11.Context->ClearRenderTargetView(Dx11.RenderView, &Payload->Color.x);

    UINT  ClearFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
    FLOAT Depth      = 1;
    UINT8 Stencil    = 0;
    Dx11.Context->ClearDepthStencilView(Dx11.DepthView, ClearFlags, Depth, Stencil);
}

static void
Dx11_DrawInstance(draw_command_instance *Payload)
{
    dx11_backend *Backend  = (dx11_backend*)Renderer->Backend;
    dx11_instance Instance = Backend->Instances[Payload->Instance];
    dx11_material Material = Backend->Materials[Instance.Data.Material];
    Sml_Assert(Instance.PerObject && Material.Constants);

    auto *Ctx = Dx11.Context;

    UINT Offset = 0;
    Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Ctx->IASetInputLayout(Material.Layout);
    Ctx->IASetVertexBuffers(0, 1, &Instance.Buffers.Vtx, &Material.Stride, &Offset);
    Ctx->IASetIndexBuffer(Instance.Buffers.Idx, DXGI_FORMAT_R32_UINT, 0);

    Ctx->VSSetShader(Material.VShader, nullptr, 0);
    Ctx->VSSetConstantBuffers(1, 1, &Instance.PerObject);

    Ctx->PSSetShader(Material.PShader, nullptr, 0);
    Ctx->PSSetSamplers(0, 1, &Material.SamplerState);
    Ctx->PSSetShaderResources(0, Material.SampledCount, Material.Sampled);
    Ctx->PSSetConstantBuffers(2, 1, &Material.Constants);

    Ctx->DrawIndexed(Instance.Buffers.IdxCnt, 0, 0);
}

static void
Dx11_Playback()
{
    ID3D11DeviceContext *Ctx = Dx11.Context;

    // =============================================================
    Ctx->OMSetRenderTargets(1, &Dx11.RenderView, Dx11.DepthView);
    Ctx->RSSetViewports(1, &Dx11.Viewport);
    // =============================================================

    sml_u8 *CmdPtr = (sml_u8*)Renderer->CommandPushBase;
    size_t  Offset = 0;

    while(Offset < Renderer->CommandPushSize)
    {
        command_header *Header = (command_header*)(CmdPtr + Offset);
        Offset += sizeof(command_header);

        switch(Header->Type)
        {

        case ContextCommand_Material:
        {
            auto *Payload = (context_command_material*)(CmdPtr + Offset);
            Dx11_SetMaterialContext(Payload);
        } break;

        case UpdateCommand_Camera:
        {
            auto *Payload = (update_command_camera*)(CmdPtr + Offset);
            Dx11_UpdateCamera(Payload);
        } break;

        case UpdateCommand_Instance:
        {
            auto *Payload = (update_command_instance*)(CmdPtr + Offset);
            Dx11_UpdateInstance(Payload);
        } break;

        case UpdateCommand_Material:
        {
            auto* Payload = (update_command_material*)(CmdPtr + Offset);
            Dx11_UpdateMaterial(Payload);
        } break;

        case DrawCommand_Clear:
        {
            auto *Payload = (draw_command_clear*)(CmdPtr + Offset);
            Dx11_DrawClearScreen(Payload);
        } break;

        case DrawCommand_Instance:
        {
            auto* Payload = (draw_command_instance*)(CmdPtr + Offset);
            Dx11_DrawInstance(Payload);
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
