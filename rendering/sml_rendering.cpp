// ===================================
//  Type Definitions
// ===================================

using sml_bit_field = u32;

enum class material : sml_u32 {};
enum class instance : sml_u32 {};

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

#define MATERIAL_TYPE_TO_SLOT(M) (M-1)

struct sml_material_texture
{
    sml_u8          *Pixels;
    size_t           DataSize;
    sml_i32          Width, Height;
    sml_i32          Pitch, Channels;
    SmlMaterial_Type MaterialType;
};

struct sml_material_constants
{
    sml_vector3 AlbedoFactor;
    sml_f32     MetallicFactor;
    sml_f32     RoughnessFactor;
    sml_f32     Padding[3];
};


using sml_renderer_entry_function = void(*)();
static sml_renderer_entry_function Playback;

// NOTE: Use slot maps? They are templated..
// This is legacy.
struct sml_backend_resource
{
    void   *Data;
    size_t  SizeOfType;
    sml_u32 Count;
    sml_u32 Capacity;
};

struct frame_rendering_data
{
    sml_matrix4 ViewProjection;
};

struct sml_renderer
{
    // Commands
    void  *CommandPushBase;
    size_t CommandPushSize;
    size_t CommandPushCapacity;

    // Misc data
    frame_rendering_data FrameData;

    // Backend
    void *Backend;

    // Backend Resources
    sml_backend_resource Materials;
    sml_backend_resource Groups;
    sml_backend_resource Instances;
    sml_backend_resource Instanced;
};

static sml_renderer* Renderer;

struct sml_instance_data
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
// Helper Functions
// ===================================

static void
SmlInt_PushToBackendResource(sml_backend_resource *Resource, void *Data, sml_u32 Index)
{
    Sml_Assert(Resource->Data);

    if(Resource->Count < Resource->Capacity)
    {
        void *WritePointer = (sml_u8*)Resource->Data + (Index *  Resource->SizeOfType);
        memcpy(WritePointer, Data, Resource->SizeOfType);
    }
}

static void*
SmlInt_GetBackendResource(sml_backend_resource *Resource, sml_u32 Index)
{
    Sml_Assert(Resource->Data);

    void *ReadPointer = (sml_u8*)Resource->Data + (Index *  Resource->SizeOfType);

    return ReadPointer;
}

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
// User API
// ===================================

// WARN:
// 1) Uses malloc instead of the engine's allocator.
// 2) Uses macros that do not exist in this code base

static sml_renderer*
Sml_CreateRenderer(SmlRenderer_Backend Backend, sml_window Window)
{
    switch(Backend)
    {

#ifdef _WIN32

    case SmlRenderer_DirectX11:
    {
        SML::Dx11_Initialize(Window);
    } break;

#endif

    default:
    {
        Sml_Assert(!"Backend not available for this platform.");
        return nullptr;
    } break;

    }

    Renderer->CommandPushBase     = malloc(Sml_Kilobytes(5));
    Renderer->CommandPushSize     = 0;
    Renderer->CommandPushCapacity = Sml_Kilobytes(5);

    return Renderer;
}

static sml_material_texture
Sml_LoadMaterialTexture(const char *FileName, SmlMaterial_Type MaterialType)
{
    sml_material_texture MatText = {};

    MatText.Pixels = stbi_load(FileName, &MatText.Width, &MatText.Height,
                               &MatText.Channels, 4);
    Sml_Assert(MatText.Pixels);

    MatText.Pitch        = MatText.Width * 4;
    MatText.MaterialType = MaterialType;
    MatText.DataSize     = MatText.Pitch * MatText.Height;

    return MatText;
}
