///////////////////////////////////////////////////////
///                  SETUP COMMANDS                 ///
///////////////////////////////////////////////////////

// ===================================
//  Type Definitions
// ===================================

using sml_mesh_group_handle = sml_u32;
using sml_material_handle   = sml_u32;

enum SmlSetupCommand_Type
{
    SmlSetupCommand_None,

    SmlSetupCommand_Material,
    SmlSetupCommand_MeshGroup,
};

struct sml_setup_command_header
{
    SmlSetupCommand_Type Type;
    sml_u32              Size;
};

struct sml_setup_command_material
{
    sml_material_texture *MaterialTextureArray;
    sml_u32               MaterialTextureCount;

    sml_bit_field       Features;
    sml_bit_field       Flags;
    sml_material_handle Handle;
};

struct sml_setup_command_mesh_group
{
    sml_mesh             *Meshes;
    sml_u32               MeshCount;
    sml_material_handle   Material;
    sml_mesh_group_handle Handle;
};

// ===================================
//  Global Variables
// ===================================

static sml_u32 NextMaterialHandle  = 1;
static sml_u32 NextMeshGroupHandle = 1;

// ===================================
// Helper Functions
// ===================================

static void
Sml_PushToOfflineBuffer(sml_renderer *Renderer, void *Data, size_t Size)
{
    if(Renderer->OfflinePushSize + Size <= Renderer->OfflinePushCapacity)
    {
        void *WritePointer = (sml_u8*)
            Renderer->OfflinePushBase + Renderer->OfflinePushSize;

        memcpy(WritePointer, Data, Size);
        Renderer->OfflinePushSize += Size;
    }
    else
    {
        Sml_Assert(!"Push buffer exceeds maximum size");
    }
}

// ===================================
// User API 
// ===================================

// NOTE: Instead of taking in the features. Can't we infer from the material textures
// instead? Would be way easier for the user.

static sml_material_handle
Sml_SetupMaterial(sml_renderer *Renderer, sml_material_texture *MatTexArray,
                  sml_u32 MatTexCount, sml_bit_field Features, sml_bit_field Flags)
{
    size_t PayloadSize = sizeof(sml_setup_command_material);

    sml_setup_command_header Header = {};
    Header.Type = SmlSetupCommand_Material;
    Header.Size = (sml_u32)PayloadSize;

    sml_setup_command_material Payload = {};
    Payload.MaterialTextureArray = MatTexArray;
    Payload.MaterialTextureCount = MatTexCount;
    Payload.Features             = Features;
    Payload.Flags                = Flags;
    Payload.Handle               = NextMaterialHandle;

    Sml_PushToOfflineBuffer(Renderer, &Header , sizeof(sml_setup_command_header));
    Sml_PushToOfflineBuffer(Renderer, &Payload, PayloadSize);

    ++NextMaterialHandle;

    return Payload.Handle;
}

static sml_mesh_group_handle
Sml_SetupMeshGroup(sml_renderer *Renderer, sml_mesh *Meshes, sml_u32 MeshCount,
                   sml_material_handle Material)
{
    size_t PayloadSize = sizeof(sml_setup_command_mesh_group);

    sml_setup_command_header Header = {};
    Header.Type = SmlSetupCommand_MeshGroup;
    Header.Size = (sml_u32)PayloadSize;

    sml_setup_command_mesh_group Payload = {};
    Payload.Meshes    = Meshes;
    Payload.MeshCount = MeshCount;
    Payload.Handle    = NextMeshGroupHandle;
    Payload.Material  = Material;

    Sml_PushToOfflineBuffer(Renderer, &Header,  sizeof(sml_setup_command_header));
    Sml_PushToOfflineBuffer(Renderer, &Payload, PayloadSize);

    ++NextMeshGroupHandle;

    return Payload.Handle;
}

///////////////////////////////////////////////////////
///                  DRAW COMMANDS                  ///
///////////////////////////////////////////////////////

// ===================================
//  Type Definitions
// ===================================

enum SmlDrawCommand_Type
{
    SmlDrawCommand_None,

    SmlDrawCommand_Clear,
    SmlDrawCommand_MeshGroup,
};

struct sml_draw_command_header
{
    SmlDrawCommand_Type Type;
    sml_u32             Size;
};

struct sml_draw_command_clear
{
    sml_vector4 Color;
};

struct sml_draw_command_mesh_group
{
    sml_mesh_group_handle Handle;
};

// ===================================
// Helper Functions
// ===================================

static void
Sml_PushToRuntimeCommandBuffer(sml_renderer *Renderer, void *Data, size_t Size)
{
    if(Renderer->RuntimePushSize + Size <= Renderer->RuntimePushCapacity)
    {
        void *WritePointer = (sml_u8*)
            Renderer->RuntimePushBase + Renderer->RuntimePushSize;

        memcpy(WritePointer, Data, Size);
        Renderer->RuntimePushSize += Size;
    }
    else
    {
        Sml_Assert(!"Push buffer exceeds maximum size");
    }
}

// ===================================
// User API 
// ===================================

static void
Sml_DrawClearScreen(sml_renderer *Renderer, sml_vector4 Color)
{
    sml_draw_command_header Header = {};
    Header.Type                    = SmlDrawCommand_Clear;
    Header.Size                    = sizeof(sml_draw_command_clear);

    sml_draw_command_clear Payload = {};
    Payload.Color                  = Color;

    Sml_PushToRuntimeCommandBuffer(Renderer, &Header, sizeof(sml_draw_command_header));
    Sml_PushToRuntimeCommandBuffer(Renderer, &Payload, Header.Size);
}

static void
Sml_DrawMeshGroup(sml_renderer *Renderer, sml_mesh_group_handle Handle)
{
    sml_draw_command_header Header = {};
    Header.Type  = SmlDrawCommand_MeshGroup;
    Header.Size = sizeof(sml_draw_command_mesh_group);

    sml_draw_command_mesh_group Payload = {};
    Payload.Handle = Handle; 

    Sml_PushToRuntimeCommandBuffer(Renderer, &Header, sizeof(sml_draw_command_header));
    Sml_PushToRuntimeCommandBuffer(Renderer, &Payload, Header.Size);
}
