// ===================================
//  Type Definitions
// ===================================

enum SmlRenderer_Backend
{
    SmlRenderer_None,
#ifdef _WIN32
    SmlRenderer_DX11,
#endif

    SmlRenderer_Count
};

enum SmlRender_Mode
{
    SmlRender_None,

    SmlRender_Forward,

    SmlRender_Count,
};

enum SmlRenderCommand_Type
{
    SmlRenderCommand_None,

    SmlRenderCommand_Mesh,

    SmlRenderCommand_Count,
};

enum SmlMaterial_Type
{
    SmlMaterial_Unknown,

    SmlMaterial_Albedo,
    SmlMaterial_Normal,
    SmlMaterial_Metallic,
    SmlMaterial_AmbientOcc,

    SmlMaterial_Count,
};

#define SmlMaterialStart SmlMaterialTexture_Albedo

struct sml_render_command_header
{
    SmlRenderCommand_Type Type;
    u32                   Size;
};

struct sml_render_mesh_payload
{
    void  *Data;
    size_t DataSize;
    u32    IndexCount;
};

using sml_render_playback_function = void(*)(sml_render_command_header *Header);

enum SmlData_Type
{
    SmlData_Vector2Float,
    SmlData_Vector3Float,
};

enum SmlShader_Flag
{
    SmlShader_CompileFromBuffer = 1 << 0,
    SmlShader_Debug             = 1 << 1,
};

enum SmlShader_Type
{
    SmlShader_Vertex,
    SmlShader_Pixel,

    SmlShaderType_Count,
};

using sml_bit_field = u32;

// Pipeline

struct sml_pipeline_layout
{
    const char  *Name; 
    sml_u32      Slot;
    SmlData_Type Format;
};

struct sml_shader_desc
{
    SmlShader_Type Type;
    sml_bit_field  Flags;

    union
    {
        const char *Path;
        struct
        {
            const char *EntryPoint;
            const char *ByteCode;
            size_t      Size;
        } Buffer;

    } Info;
};

struct sml_pipeline_desc
{
    sml_shader_desc *Shaders;
    sml_u32          ShaderCount;

    sml_pipeline_layout *Layout;
    sml_u32              LayoutCount;
};

enum SmlTexture_Flag
{
    SmlTexture_CompileFromBuffer,
};

struct sml_texture_desc
{
    sml_bit_field Flags;
    sml_u32       BindSlot;

    // NOTE: Maybe this shouldn't be here.
    SmlMaterial_Type MaterialType;

    union
    {
        const char *Path;
        struct
        {
            const char *Pixels;
            size_t      Size;
        } Buffer;

    } Info;
};

struct sml_pbr_material_constant
{
    sml_vector3 AlbedoFactor;
    sml_f32     MetallicFactor;
    sml_f32     RoughnessFactor;
    sml_f32     Padding[3];
};

struct sml_material_desc
{
    sml_texture_desc          Textures[SmlMaterial_Count];
    sml_pbr_material_constant Constants;
};

struct sml_pipeline
{
    u32   Handle;
    void *BackendData;
};

// ===================================
//  Global variables
// ===================================

static sml_pipeline SmlPipeline;
static sml_u32      NextPipelineHandle = 0;

static SmlRenderer_Backend ActiveBackend;

static const char* SmlDefaultShader_HLSL = R"(

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 View;
    float4x4 Projection;
};

cbuffer MaterialBuffer : register(b1)
{
    float3 AlbedoFactor;
    float  MetallicFactor;
    float  RoughnessFactor;
};

//--------------------------------------------------------------------------------------
// Textures & Sampler
//--------------------------------------------------------------------------------------
Texture2D    AlbedoTexture            : register(t0);
Texture2D    NormalTexture            : register(t1);
Texture2D    MetallicRoughnessTexture : register(t2);
Texture2D    AmbientOcclusionTexture  : register(t3);

SamplerState MaterialSampler : register(s0);

//--------------------------------------------------------------------------------------
// Vertex input / output
//--------------------------------------------------------------------------------------
struct VSInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal   : TEXCOORD1;
    float2 UV       : TEXCOORD2;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VSOutput VSMain(VSInput IN)
{
    VSOutput OUT;

    OUT.Position = mul(Projection, mul(float4(IN.Position, 1.0f), View));
    OUT.Normal   = IN.Normal;
    OUT.UV       = IN.UV;

    return OUT;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_Target
{
    // Sample all maps
    float3 albedo   = AlbedoTexture.Sample(MaterialSampler, IN.UV).rgb * AlbedoFactor;
    float  metallic = MetallicRoughnessTexture.Sample(MaterialSampler, IN.UV).b * MetallicFactor;
    float  roughness= MetallicRoughnessTexture.Sample(MaterialSampler, IN.UV).g * RoughnessFactor;
    float  ao       = AmbientOcclusionTexture.Sample(MaterialSampler, IN.UV).r;

    float3 color = albedo;

    return float4(color, 1.0f);
}

//--------------------------------------------------------------------------------------
// Technique / Pass (for FX-based systems, optionally)
//--------------------------------------------------------------------------------------
)";

// ===================================
// Renderer agnostic files
// ===================================

// ===================================
// Internal Helpers
// ===================================

static const char*
Sml_ShaderAsString(SmlShader_Type Type)
{
    switch(Type)
    {

    case SmlShader_Vertex: return "Vertex Shader\n";
    case SmlShader_Pixel : return "Pixel Shader\n";

    }
}

static size_t
Sml_GetSizeOfDataType(SmlData_Type Type)
{
    switch(Type)
    {

    case SmlData_Vector2Float: return 2 * sizeof(sml_f32);
    case SmlData_Vector3Float: return 3 * sizeof(sml_f32);

    default: return 0;

    }
}

static const char*
Sml_GetDefaultShaderByteCode(SmlRenderer_Backend Backend)
{
    switch(Backend)
    {

    case SmlRenderer_DX11: return SmlDefaultShader_HLSL;

    }
}

static size_t
Sml_GetDefaultShaderSize(SmlRenderer_Backend Backend)
{

    switch(Backend)
    {

    case SmlRenderer_DX11: return strlen(SmlDefaultShader_HLSL);

    }
}

// ===================================
// Renderer specific files
// ===================================

#ifdef _WIN32
#include "dx11/sml_dx11.cpp"
#endif

// ===================================
// User API
// ===================================


static sml_pipeline
Sml_CreatePipeline(sml_pipeline_desc Desc)
{
    Sml_Assert(Desc.ShaderCount < SmlShaderType_Count);

    sml_pipeline Pipeline = {};

    switch(ActiveBackend)
    {

#ifdef _WIN32

    case SmlRenderer_DX11:
    {
        Pipeline.BackendData = SmlDx11_CreatePipeline(Desc);
        Pipeline.Handle      = NextPipelineHandle;
    } break;

#endif

    default:
        break;

    }

    NextPipelineHandle++;

    return Pipeline;
};

static sml_render_playback_function 
Sml_CreateRenderer(SmlRenderer_Backend Backend,
                   sml_window Window)
{
    sml_material_desc MaterialDesc = {};
    sml_pipeline_desc PipelineDesc = {};

    // Default pipeline init
    {
        sml_shader_desc ShaderDesc[2] = {};

        sml_shader_desc *VShaderDesc        = &ShaderDesc[0];
        VShaderDesc->Type                   = SmlShader_Vertex;
        VShaderDesc->Flags                  = SmlShader_CompileFromBuffer;
        VShaderDesc->Info.Buffer.EntryPoint = "VSMain";
        VShaderDesc->Info.Buffer.ByteCode   = Sml_GetDefaultShaderByteCode(Backend);
        VShaderDesc->Info.Buffer.Size       = Sml_GetDefaultShaderSize(Backend);

        sml_shader_desc *PShaderDesc        = &ShaderDesc[1];
        PShaderDesc->Type                   = SmlShader_Pixel;
        PShaderDesc->Flags                  = SmlShader_CompileFromBuffer;
        PShaderDesc->Info.Buffer.EntryPoint = "PSMain";
        PShaderDesc->Info.Buffer.ByteCode   = Sml_GetDefaultShaderByteCode(Backend);
        PShaderDesc->Info.Buffer.Size       = Sml_GetDefaultShaderSize(Backend);

        sml_pipeline_layout Layout[3] = 
        {
            {"POSITION", 0, SmlData_Vector3Float},
            {"NORMAL"  , 0, SmlData_Vector3Float},
            {"TEXCOORD", 0, SmlData_Vector2Float},
        };

        PipelineDesc.Shaders     = ShaderDesc;
        PipelineDesc.ShaderCount = 2;
        PipelineDesc.Layout      = Layout;
        PipelineDesc.LayoutCount = 3;
    }

    // Default material
    {
        sml_texture_desc *Albedo = MaterialDesc.Textures + SmlMaterial_Albedo;
        Albedo->Info.Path        = "../../small_engine/data/textures/brick_wall/brick_wall_albedo.png";
        Albedo->MaterialType     = SmlMaterial_Albedo;
        Albedo->BindSlot         = 0;

        sml_texture_desc *Normal = MaterialDesc.Textures + SmlMaterial_Normal;
        Normal->Info.Path        = "../../small_engine/data/textures/brick_wall/brick_wall_normal.png";
        Normal->MaterialType     = SmlMaterial_Normal;
        Normal->BindSlot         = 1;

        sml_texture_desc *Metallic = MaterialDesc.Textures + SmlMaterial_Metallic;
        Metallic->Info.Path        = "../../small_engine/data/textures/brick_wall/brick_wall_metallic.png";
        Metallic->MaterialType     = SmlMaterial_Metallic;
        Metallic->BindSlot         = 2;

        sml_texture_desc *Ambient = MaterialDesc.Textures + SmlMaterial_AmbientOcc;
        Ambient->Info.Path        = "../../small_engine/data/textures/brick_wall/brick_wall_ao.png";
        Ambient->MaterialType     = SmlMaterial_AmbientOcc;
        Ambient->BindSlot         = 3;

        sml_pbr_material_constant *Constants = &MaterialDesc.Constants;
        Constants->AlbedoFactor              = sml_vector3(1.0f, 1.0f, 1.0f);
        Constants->MetallicFactor            = 1.0f;
        Constants->RoughnessFactor           = 1.0f;
    }

    ActiveBackend = Backend;

    switch(Backend)
    {

#ifdef _WIN32

    case SmlRenderer_DX11:
    {
        return SmlDx11_Initialize(Window, MaterialDesc, PipelineDesc);
    } break;

#endif

    default:
        break;
    }

}

