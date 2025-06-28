///////////////////////////////////////////////////////
///                  SETUP COMMANDS                 ///
///////////////////////////////////////////////////////

// ===================================
//  Type Definitions
// ===================================

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

    sml_bit_field Features;
    sml_bit_field Flags;
    sml_u32       MaterialIndex;
};

struct sml_setup_command_mesh_group
{
    sml_mesh *Meshes;
    sml_u32   MeshCount;
    sml_u32   MaterialIndex;
    sml_u32   GroupIndex;
};

// ===================================
//  Global Variables
// ===================================

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

static sml_u32
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
    Payload.MaterialIndex        = Renderer->Materials.Count;

    Sml_PushToOfflineBuffer(Renderer, &Header , sizeof(sml_setup_command_header));
    Sml_PushToOfflineBuffer(Renderer, &Payload, PayloadSize);

    ++Renderer->Materials.Count;

    return Payload.MaterialIndex;
}

static sml_u32
Sml_SetupMeshGroup(sml_renderer *Renderer, sml_mesh *Meshes, sml_u32 MeshCount,
                   sml_u32 Material)
{
    size_t PayloadSize = sizeof(sml_setup_command_mesh_group);

    sml_setup_command_header Header = {};
    Header.Type = SmlSetupCommand_MeshGroup;
    Header.Size = (sml_u32)PayloadSize;

    sml_setup_command_mesh_group Payload = {};
    Payload.Meshes        = Meshes;
    Payload.MeshCount     = MeshCount;
    Payload.GroupIndex    = Renderer->Groups.Count;
    Payload.MaterialIndex = Material;

    Sml_PushToOfflineBuffer(Renderer, &Header,  sizeof(sml_setup_command_header));
    Sml_PushToOfflineBuffer(Renderer, &Payload, PayloadSize);

    ++Renderer->Groups.Count;

    return Payload.GroupIndex;
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
    sml_u32 GroupIndex;
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
Sml_DrawMeshGroup(sml_renderer *Renderer, sml_u32 GroupIndex)
{
    sml_draw_command_header Header = {};
    Header.Type  = SmlDrawCommand_MeshGroup;
    Header.Size = sizeof(sml_draw_command_mesh_group);

    sml_draw_command_mesh_group Payload = {};
    Payload.GroupIndex = GroupIndex; 

    Sml_PushToRuntimeCommandBuffer(Renderer, &Header, sizeof(sml_draw_command_header));
    Sml_PushToRuntimeCommandBuffer(Renderer, &Payload, Header.Size);
}
