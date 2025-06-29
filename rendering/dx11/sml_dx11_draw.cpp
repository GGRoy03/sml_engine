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
SmlDx11_DrawInstance(sml_draw_command_instance *Payload, sml_renderer *Renderer)
{
    sml_dx11_instance *Instance = (sml_dx11_instance*)
        SmlInt_GetBackendResource(&Renderer->Instances, Payload->Instance);

    sml_dx11_material *Material = (sml_dx11_material*)
        SmlInt_GetBackendResource(&Renderer->Materials, Instance->Data.Material);

    ID3D11DeviceContext *Ctx    = Dx11.Context;
    UINT                 Offset = 0;

    // IA
    Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Ctx->IASetInputLayout(Material->Variant.InputLayout);
    Ctx->IASetVertexBuffers(0, 1, &Instance->Geometry.VertexBuffer,
                            &Material->Variant.Stride, &Offset);
    Ctx->IASetIndexBuffer(Instance->Geometry.IndexBuffer, DXGI_FORMAT_R32_UINT ,0);

    // VS
    Ctx->VSSetShader(Material->Variant.VertexShader, nullptr, 0);

    {
        D3D11_MAPPED_SUBRESOURCE Mapped = {};
        Ctx->Map(Instance->PerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);

        sml_matrix4 World = SmlMat4_Identity();
        memcpy(Mapped.pData, &World, sizeof(World));

        Ctx->Unmap(Instance->PerObject, 0);

        Ctx->VSSetConstantBuffers(1, 1, &Instance->PerObject);
    }

    // PS
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

    // Draw
    Ctx->DrawIndexed(Instance->Geometry.IndexCount, 0, 0);
}

// NOTE: Is it generally better to update resources just before they are drawn or
// as we do the work for the frame? Should be the former right?

static void
SmlDx11_DrawInstanced(sml_draw_command_instanced *Payload, sml_renderer *Renderer)
{
    sml_dx11_instanced *Instanced = (sml_dx11_instanced*)
        SmlInt_GetBackendResource(&Renderer->Instanced, Payload->Instanced);

    sml_dx11_material *Material = (sml_dx11_material*)
        SmlInt_GetBackendResource(&Renderer->Materials, Instanced->Material);

    ID3D11DeviceContext *Ctx    = Dx11.Context;
    UINT                 Offset = 0;

    // IA
    Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Ctx->IASetInputLayout(Material->Variant.InputLayout);
    Ctx->IASetVertexBuffers(0, 1, &Instanced->Geometry.VertexBuffer,
                            &Material->Variant.Stride, &Offset);
    Ctx->IASetIndexBuffer(Instanced->Geometry.IndexBuffer, DXGI_FORMAT_R32_UINT ,0);

    // VS
    Ctx->VSSetShader(Material->Variant.VertexShader, nullptr, 0);

    {
        size_t       DataSize = Payload->Count * sizeof(sml_matrix4);
        sml_matrix4* CPUMatrices = (sml_matrix4*)malloc(DataSize);

        for (u32 Index = 0; Index < Payload->Count; Index++)
        {
            CPUMatrices[Index] = SmlMat4_Translation(Payload->Data[Index]);
        }

        D3D11_MAPPED_SUBRESOURCE Mapped = {};
        Dx11.Context->Map(Instanced->Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
        memcpy(Mapped.pData, CPUMatrices, DataSize);
        Dx11.Context->Unmap(Instanced->Buffer, 0);

        Dx11.Context->VSSetShaderResources(10, 1, &Instanced->ResourceView);
        
        free(CPUMatrices);
    }

    // PS
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

    // Draw
    Ctx->DrawIndexedInstanced(Instanced->Geometry.IndexCount, Payload->Count, 0, 0, 0);
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

        case SmlDrawCommand_Instance:
        {
            auto *Payload = (sml_draw_command_instance*)(CmdPtr + Offset);
            SmlDx11_DrawInstance(Payload, Renderer);
        } break;

        case SmlDrawCommand_Instanced:
        {
            auto *Payload = (sml_draw_command_instanced*)(CmdPtr + Offset);
            SmlDx11_DrawInstanced(Payload, Renderer);
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
