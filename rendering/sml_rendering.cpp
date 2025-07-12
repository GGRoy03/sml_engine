// ===================================
//  Type Definitions
// ===================================

template<typename Tag, typename Underlying = sml_u32>
struct strong_index 
{
    Underlying Value;

    constexpr strong_index() noexcept = default;

    constexpr explicit strong_index(Underlying v) noexcept
      : Value(v) {}

    constexpr operator Underlying() const noexcept 
    {
      return Value;
    }
};

using sml_bit_field = u32;

struct material_tag{};
struct instance_tag{};

using material = strong_index<material_tag>;
using instance = strong_index<instance_tag>;

enum Renderer_Backend
{
    Renderer_None,

    Renderer_DirectX11,
};

enum Material_Type
{
    Material_Albedo,

    MaterialType_Count,
};

enum RenderingContext_Type
{
    RenderingContext_None,

    RenderingContext_Material,
};

#define MATERIAL_TYPE_TO_SLOT(M) (M-1)

struct texture
{
    sml_u8 *Pixels;
    size_t  DataSize;
    sml_i32 Width;
    sml_i32 Height;
    sml_i32 Pitch;
    sml_i32 Channels;

    sml_heap_block PixelHeap;
};

struct material_texture
{
    sml_u8       *Pixels;
    size_t        DataSize;
    sml_i32       Width, Height;
    sml_i32       Pitch, Channels;
    Material_Type MaterialType;
};

struct material_constants
{
    sml_vector3 AlbedoFactor;
    sml_f32     MetallicFactor;
    sml_f32     RoughnessFactor;
    sml_f32     Padding[3];
};

using renderer_playback = void(*)();
static renderer_playback Playback;

struct frame_rendering_data
{
    sml_matrix4 ViewProjection;
};

struct renderer
{
    // Commands
    void  *CommandPushBase;
    size_t CommandPushSize;
    size_t CommandPushCapacity;

    // Misc data
    frame_rendering_data FrameData;

    // Context
    RenderingContext_Type Context;
    union
    {
        sml_bit_field ShaderFeatures;
    } ContextData;

    // Backend
    void *Backend;
};

static renderer* Renderer;

struct instance_data
{
    sml_vector3 Position;
    sml_u32     Material;
};

// ===================================
//  Global Variables
// ===================================

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
// Renderer agnostic files
// ===================================

#include "sml_meshes.cpp"
#include "sml_commands.cpp"
#include "sml_camera.cpp"
#include "sml_shaders.cpp"

// ===================================
// Renderer specific files
// ===================================

#ifdef _WIN32
#include "dx11/dx11_context.cpp"
#endif

// ===================================
// Internal Helpers
// ===================================

// NOTE: These are now commands!

static inline void
BeginMaterialContext(sml_bit_field ShaderFeatures)
{
    Renderer->Context      = RenderingContext_Material;
    Renderer->Context.Data = ShaderFeatures;
}

static inline void
EndMaterialContext()
{
    Renderer->Context      = RenderingContext_None;
    Renderer->Context.Data = 0;
}

// ===================================
// User API
// ===================================

static renderer*
Sml_CreateRenderer(Renderer_Backend Backend, sml_window Window)
{
    switch(Backend)
    {

#ifdef _WIN32

    case Renderer_DirectX11:
    {
        Dx11_Initialize(Window);
    } break;

#endif

    default:
    {
        Sml_Assert(!"Backend not available for this platform.");
        return nullptr;
    } break;

    }

    Renderer->CommandPushBase     = SmlMemory.Allocate(Sml_Kilobytes(5)).Data;
    Renderer->CommandPushSize     = 0;
    Renderer->CommandPushCapacity = Sml_Kilobytes(5);

    return Renderer;
}

static material_texture
LoadMaterialTexture(const char *FileName, Material_Type MaterialType)
{
    material_texture MatText = {};

    MatText.Pixels = stbi_load(FileName, &MatText.Width, &MatText.Height,
                               &MatText.Channels, 4);
    Sml_Assert(MatText.Pixels);

    MatText.Pitch        = MatText.Width * 4;
    MatText.MaterialType = MaterialType;
    MatText.DataSize     = MatText.Pitch * MatText.Height;

    return MatText;
}
