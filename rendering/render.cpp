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
    f32         MetallicFactor;
    f32         RoughnessFactor;
    f32         Padding[3];
};

struct sml_material_desc
{
    sml_texture_desc          Textures[SmlMaterial_Count];
    sml_pbr_material_constant Constants;
};

// ===================================
//  Global variables
// ===================================

static SmlRenderer_Backend ActiveBackend;

static u32 NextPipelineHandle = 0;

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

// ===================================
// Renderer specific files
// ===================================

#ifdef _WIN32
#include "dx11/dx11.cpp"
#endif

// ===================================
// User API
// ===================================

struct sml_pipeline
{
    u32 Handle;

    union
    {

#ifdef _WIN32
        sml_dx11_pipeline Dx11;
#endif

    } Backend;
};

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
        Pipeline.Backend.Dx11 = SmlDx11_CreatePipeline(Desc);
        Pipeline.Handle       = NextPipelineHandle;
    } break;

#endif

    default:
        break;

    }

    return Pipeline;
};

static sml_render_playback_function 
Sml_CreateRenderer(SmlRenderer_Backend Backend, SmlRender_Mode Mode,
                   sml_window Window)
{
    Sml_Unused(Mode);

    ActiveBackend = Backend;

    switch(Backend)
    {

#ifdef _WIN32

    case SmlRenderer_DX11:
    {
        return SmlDx11_Initialize(Window);
    } break;

#endif

    default:
        break;
    }
}

