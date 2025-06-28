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

// WARN:
// 1) Hardcoded topology.
// 2) Uses magic numbers
// 3) Missing vertex shader constant buffers. (World Matrix)
// 4) Makes copies on the stack
// 5) Shader resource view binding is a bit akward
//    because of how the data is structured.
// 6) This is not even the correct way to draw a group.
//    Simply use this to prototype drawing a single entity
//    within a group. It should iterate over mesh-info.
//    For now, we keep it really simple.
 
static void
SmlDx11_DrawMeshGroup(sml_draw_command_mesh_group *Payload, sml_renderer *Renderer)
{
    sml_dx11_mesh_group *Group = (sml_dx11_mesh_group*)
        SmlInt_GetBackendResource(&Renderer->Groups, Payload->GroupIndex);

    sml_dx11_material *Material = (sml_dx11_material*)
        SmlInt_GetBackendResource(&Renderer->Materials, Group->MaterialIndex);

    ID3D11DeviceContext *Context = Dx11.Context;
    UINT                 Offset  = 0;

    // IA
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context->IASetInputLayout(Material->Variant.InputLayout);
    Context->IASetVertexBuffers(0, 1, &Group->VertexBuffer, &Material->Variant.Stride,
                                &Offset);
    Context->IASetIndexBuffer(Group->IndexBuffer, DXGI_FORMAT_R32_UINT ,0);

    // VS
    Context->VSSetShader(Material->Variant.VertexShader, nullptr, 0);

    // NOTE: Simple way to set the per object data. Obviously it's quite bad
    // but it should work.
    {
        D3D11_MAPPED_SUBRESOURCE Mapped = {};
        Context->Map(Group->PerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);

        sml_matrix4 World = SmlMat4_Identity();
        memcpy(Mapped.pData, &World, sizeof(World));

        Context->Unmap(Group->PerObject, 0);

        Context->VSSetConstantBuffers(1, 1, &Group->PerObject);
    }

    // PS
    Context->PSSetConstantBuffers(2, 1, &Material->ConstantBuffer);
    Context->PSSetSamplers(0, 1, &Material->SamplerState);
    Context->PSSetShader(Material->Variant.PixelShader, nullptr, 0);

    for (u32 Index = 0; Index < SmlMaterial_Count; Index++)
    {
        ID3D11ShaderResourceView* View = Material->Sampled[Index].ResourceView;

        if (View)
        {
            Context->PSSetShaderResources(0, 1, &View);
        }
    }

    // Draw
    Context->DrawIndexed(Group->IndexCount, 0, 0);
}

// WARN:
// 1) Beginning of frame is kind of messy.

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
            SmlDx11_DrawMeshGroup(Payload, Renderer);
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
