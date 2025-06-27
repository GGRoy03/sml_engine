// ===================================
// Type Definitions
// ===================================

// NOTE: Temporary include
#include <unordered_map>

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
    ID3D11Buffer             *GPUHandle;
};


// WARN:
// 1) Should probably hold an optimized draw-list of some sort
// when we add materials.

struct sml_dx11_static_group_cached_state
{
    ID3D11Buffer *VertexBuffer;
    ID3D11Buffer *IndexBuffer;

    ID3D11VertexShader *VShader;
    ID3D11PixelShader  *PShader;
    ID3D11InputLayout  *InputLayout;

    UINT    Stride;
    sml_u32 IndexCount;

    sml_dx11_pbr_material Material;
};

// ===================================
//  Global Variables
// ===================================

// NOTE: For now we simply use a std::map for our cache.
// This will probably change, but for simplicity the map should
// be more than fine.
std::unordered_map<sml_u32, sml_dx11_static_group_cached_state> StaticCache;

// ===================================
// Internal Helpers
// ===================================

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

    Status = Dx11.Device->CreateBuffer(&CDesc, &ConstantData, &Material.GPUHandle);
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

// ===================================
// API Implementation
// ===================================

static void 
SmlDx11_Setup(sml_renderer *Renderer)
{

    sml_u8 *CmdPtr = (sml_u8*)Renderer->OfflinePushBase;
    size_t  Offset = 0;

    while(Offset < Renderer->OfflinePushSize)
    {
        auto *Header = (sml_setup_command_header*)(CmdPtr + Offset);
        Offset      += sizeof(sml_setup_command_header);

        switch(Header->Type)
        {

        case SmlSetupCommand_Material:
        {
            auto *Payload = (sml_setup_command_material*)(Header + 1);
            SmlDx11_SetupMaterial(Payload);
        } break;

        case SmlSetupCommand_MeshGroup:
        {
            auto *Payload = (sml_setup_command_mesh_group*)(Header + 1);
            SmlDx11_SetupMeshGroup(Payload);
        } break;

        default:
        {
            Sml_Assert(!"Unknown setup command type.");
            return;
        } break;

        }

        Offset += Header->Size;
    }

    Renderer->OfflinePushSize = 0;
}
