// ===================================
//  Type Definitions
// ===================================

using sml_bit_field = u32;

enum SmlRenderer_Backend
{
    SmlRenderer_None,

    SmlRenderer_DirectX11,

    SmlRenderer_Count
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

struct sml_pipeline_layout
{
    const char  *Name; 
    sml_u32      Slot;
    SmlData_Type Format;
};

struct sml_pipeline_desc
{
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

using sml_material_handle = sml_u32;

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


struct sml_renderer;
using sml_renderer_entry_function = void(*)(sml_renderer *Renderer);

struct sml_renderer
{
    // Offline Push Buffer
    void  *OfflinePushBase;
    size_t OfflinePushSize;
    size_t OfflinePushCapacity;

    // Runtime Push Buffer
    void  *RuntimePushBase;
    size_t RuntimePushSize;
    size_t RuntimePushCapacity;

    // Misc data
    sml_matrix4 CameraData;

    // Entry points
    sml_renderer_entry_function Playback;
    sml_renderer_entry_function Setup;
};

// ===================================
//  Global Variables
// ===================================

static sml_pipeline SmlPipeline;
static sml_u32      NextPipelineHandle = 0;

static const char* SmlDefaultShader_HLSL = R"(
//-------------------------------------------------------------------------------------
// Constant Buffers
//-------------------------------------------------------------------------------------
cbuffer TransformBuffer : register(b0)
{
    float4x4 ViewProjection;
};

cbuffer MaterialBuffer : register(b1)
{
    float3 AlbedoFactor;
    float  MetallicFactor;
    float  RoughnessFactor;
};

//-------------------------------------------------------------------------------------
// Textures & Sampler
//-------------------------------------------------------------------------------------
Texture2D    AlbedoTexture            : register(t0);
Texture2D    NormalTexture            : register(t1);
Texture2D    MetallicRoughnessTexture : register(t2);
Texture2D    AmbientOcclusionTexture  : register(t3);

SamplerState MaterialSampler : register(s0);

//-------------------------------------------------------------------------------------
// Vertex Input / Output
//-------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------
// Vertex Shader
//-------------------------------------------------------------------------------------
VSOutput VSMain(VSInput IN)
{
    VSOutput OUT;

    OUT.Position = mul(ViewProjection, float4(IN.Position, 1.0f));
    OUT.Normal   = IN.Normal;
    OUT.UV       = IN.UV;

    return OUT;
}

//-------------------------------------------------------------------------------------
// Pixel Shader
//-------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------
// Technique / Pass (for FX-based systems, optionally)
//-------------------------------------------------------------------------------------
)";

// ===================================
// Helper Functions
// ===================================

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

    case SmlRenderer_DirectX11: return SmlDefaultShader_HLSL;

    default: return nullptr;

    }
}

static size_t
Sml_GetDefaultShaderSize(SmlRenderer_Backend Backend)
{

    switch(Backend)
    {

    case SmlRenderer_DirectX11: return strlen(SmlDefaultShader_HLSL);

    default: return 0;

    }
}

// ===================================
// Renderer agnostic files
// ===================================

#include "sml_camera.cpp"
#include "sml_commands.cpp"
#include "sml_shaders.cpp"

// ===================================
// Renderer specific files
// ===================================

#ifdef _WIN32
#include "dx11/sml_dx11_context.cpp"
#endif

// ===================================
// User API
// ===================================

// WARN:
// 1) Uses malloc instead of the engine's allocator.
// 2) Uses macros that do not exist in this code base

static sml_renderer*
Sml_CreateRenderer(SmlRenderer_Backend Backend, sml_window Window)
{
    sml_renderer     *Renderer        = nullptr;
    sml_material_desc DefaultMaterial = {};
    sml_pipeline_desc DefaultPipeline = {};

    {
        sml_texture_desc* Albedo = DefaultMaterial.Textures + SmlMaterial_Albedo;
        Albedo->Info.Path = "../../small_engine/data/textures/brick_wall/brick_wall_albedo.png";
        Albedo->MaterialType = SmlMaterial_Albedo;
        Albedo->BindSlot = 0;

        sml_texture_desc* Normal = DefaultMaterial.Textures + SmlMaterial_Normal;
        Normal->Info.Path = "../../small_engine/data/textures/brick_wall/brick_wall_normal.png";
        Normal->MaterialType = SmlMaterial_Normal;
        Normal->BindSlot = 1;

        sml_texture_desc* Metallic = DefaultMaterial.Textures + SmlMaterial_Metallic;
        Metallic->Info.Path = "../../small_engine/data/textures/brick_wall/brick_wall_metallic.png";
        Metallic->MaterialType = SmlMaterial_Metallic;
        Metallic->BindSlot = 2;

        sml_texture_desc* Ambient = DefaultMaterial.Textures + SmlMaterial_AmbientOcc;
        Ambient->Info.Path = "../../small_engine/data/textures/brick_wall/brick_wall_ao.png";
        Ambient->MaterialType = SmlMaterial_AmbientOcc;
        Ambient->BindSlot = 3;

        sml_pbr_material_constant* Constants = &DefaultMaterial.Constants;
        Constants->AlbedoFactor = sml_vector3(1.0f, 1.0f, 1.0f);
        Constants->MetallicFactor = 1.0f;
        Constants->RoughnessFactor = 1.0f;
    }

    switch(Backend)
    {

#ifdef _WIN32

    case SmlRenderer_DirectX11:
    {
        Renderer = SmlDx11_Initialize(Window, &DefaultPipeline, &DefaultMaterial);
    } break;

#endif

    default:
    {
        Sml_Assert(!"Backend not available for this platform.");
        return nullptr;
    } break;

    }

    Renderer->RuntimePushBase     = malloc(Sml_Kilobytes(5));
    Renderer->RuntimePushSize     = 0;
    Renderer->RuntimePushCapacity = Sml_Kilobytes(5);

    Renderer->OfflinePushBase     = malloc(Sml_Kilobytes(5));
    Renderer->OfflinePushSize     = 0;
    Renderer->OfflinePushCapacity = Sml_Kilobytes(5);

    return Renderer;
}
