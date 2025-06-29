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


struct sml_renderer;
using sml_renderer_entry_function = void(*)(sml_renderer *Renderer);

// NOTE: I would be nice to get the actual type via a template of some sorts.
// It would reduce the amounts of random casts all around the code. Could
// even remove SizeOfType and simplify the logic.
struct sml_backend_resource
{
    void    *Data;
    size_t   SizeOfType;
    sml_u32  Count;
    sml_u32  Capacity;
};

struct sml_renderer
{
    // Offline Push Buffer
    void  *OfflinePushBase;
    size_t OfflinePushSize;
    size_t OfflinePushCapacity;

    // Update Push Buffer
    void  *UpdatePushBase;
    size_t UpdatePushSize;
    size_t UpdatePushCapacity;

    // Runtime Push Buffer
    void  *RuntimePushBase;
    size_t RuntimePushSize;
    size_t RuntimePushCapacity;

    // Misc data
    sml_matrix4 CameraData;

    // Backend Resources
    sml_backend_resource Materials;
    sml_backend_resource Groups;
    sml_backend_resource Instances;
    sml_backend_resource Instanced;

    // Entry points
    sml_renderer_entry_function Setup;
    sml_renderer_entry_function Update;
    sml_renderer_entry_function Playback;
};

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
    sml_renderer *Renderer = nullptr;

    switch(Backend)
    {

#ifdef _WIN32

    case SmlRenderer_DirectX11:
    {
        Renderer = SmlDx11_Initialize(Window);
    } break;

#endif

    default:
    {
        Sml_Assert(!"Backend not available for this platform.");
        return nullptr;
    } break;

    }

    Renderer->UpdatePushBase     = malloc(Sml_Kilobytes(5));
    Renderer->UpdatePushSize     = 0;
    Renderer->UpdatePushCapacity = Sml_Kilobytes(5);

    Renderer->OfflinePushBase     = malloc(Sml_Kilobytes(5));
    Renderer->OfflinePushSize     = 0;
    Renderer->OfflinePushCapacity = Sml_Kilobytes(5);

    Renderer->RuntimePushBase     = malloc(Sml_Kilobytes(5));
    Renderer->RuntimePushSize     = 0;
    Renderer->RuntimePushCapacity = Sml_Kilobytes(5);

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
