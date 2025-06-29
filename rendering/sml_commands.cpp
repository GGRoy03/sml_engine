// ===================================
//  Common Type Definitions
// ===================================

enum class sml_instance  : sml_u32 {};
enum class sml_instanced : sml_u32 {};

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
    SmlSetupCommand_Instance,
    SmlSetupCommand_Instanced,
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

struct sml_setup_command_instance
{
    sml_mesh    *Mesh;
    sml_vector3  Position;
    sml_u32      Material;
    sml_instance Instance;
};

struct sml_setup_command_instanced
{
    sml_u32       Count;
    sml_mesh     *Mesh;
    sml_u32       Material;
    sml_instanced Instanced;
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

static sml_instance
Sml_SetupInstance(sml_renderer *Renderer, sml_u32 Material, sml_vector3 Position,
                  sml_mesh *Mesh)
{
    sml_setup_command_header Header = {};
    Header.Type = SmlSetupCommand_Instance;
    Header.Size = (sml_u32)sizeof(sml_setup_command_instance);

    sml_setup_command_instance Payload = {};
    Payload.Material = Material;
    Payload.Position = Position;
    Payload.Instance = (sml_instance)Renderer->Instances.Count;
    Payload.Mesh     = Mesh;

    Sml_PushToOfflineBuffer(Renderer, &Header , sizeof(sml_setup_command_header));
    Sml_PushToOfflineBuffer(Renderer, &Payload, Header.Size);

    ++Renderer->Instances.Count;

    return Payload.Instance;
}

static sml_instanced
Sml_SetupInstanced(sml_renderer *Renderer, sml_u32 Material, sml_mesh *Mesh,
                   sml_u32 InstanceCount)
{
    sml_setup_command_header Header = {};
    Header.Type = SmlSetupCommand_Instanced;
    Header.Size = (sml_u32)sizeof(sml_setup_command_instanced);

    sml_setup_command_instanced Payload = {};
    Payload.Material  = Material;
    Payload.Instanced = (sml_instanced)Renderer->Instances.Count;
    Payload.Mesh      = Mesh;
    Payload.Count     = InstanceCount;

    Sml_PushToOfflineBuffer(Renderer, &Header , sizeof(sml_setup_command_header));
    Sml_PushToOfflineBuffer(Renderer, &Payload, Header.Size);

    ++Renderer->Instances.Count;

    return Payload.Instanced;
}

///////////////////////////////////////////////////////
///                  UPDATE COMMANDS                ///
///////////////////////////////////////////////////////

// ===================================
//  Type Definitions
// ===================================

enum SmlUpdateCommand_Type
{
    SmlUpdateCommand_None,

    SmlUpdateCommand_InstanceData,
    SmlUpdateCommand_InstancedData,
};

struct sml_update_command_header
{
    SmlUpdateCommand_Type Type;
    sml_u32               Size;
};

struct sml_update_command_instance_data
{
    sml_vector3  Data;
    sml_instance Instance;
};

struct sml_update_command_instanced_data
{
    sml_vector3  *Data;
    sml_instanced Instanced;
};

// ===================================
// Helper Functions
// ===================================

static void
SmlInt_PushToUpdateBuffer(sml_renderer *Renderer, void *Data, size_t Size)
{
    if(Renderer->UpdatePushSize + Size <= Renderer->UpdatePushCapacity)
    {
        void *WritePointer = (sml_u8*)
            Renderer->UpdatePushBase + Renderer->UpdatePushSize;

        memcpy(WritePointer, Data, Size);
        Renderer->UpdatePushSize += Size;
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
Sml_UpdateInstance(sml_renderer *Renderer, sml_vector3 Data, sml_instance Instance)
{
    sml_update_command_header Header = {};
    Header.Type = SmlUpdateCommand_InstanceData;
    Header.Size = (sml_u32)sizeof(sml_update_command_instance_data);

    sml_update_command_instance_data Payload = {};
    Payload.Data     = Data;
    Payload.Instance = Instance;

    SmlInt_PushToUpdateBuffer(Renderer, &Header , sizeof(sml_update_command_header));
    SmlInt_PushToUpdateBuffer(Renderer, &Payload, Header.Size);
}

static void
Sml_UpdateInstanced(sml_renderer *Renderer, sml_vector3 *Data, sml_instanced Instanced)
{
    sml_update_command_header Header = {};
    Header.Type = SmlUpdateCommand_InstancedData;
    Header.Size = (sml_u32)sizeof(sml_update_command_instanced_data);

    sml_update_command_instanced_data Payload = {};
    Payload.Data      = Data;
    Payload.Instanced = Instanced;

    SmlInt_PushToUpdateBuffer(Renderer, &Header , sizeof(sml_update_command_header));
    SmlInt_PushToUpdateBuffer(Renderer, &Payload, Header.Size);
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
    SmlDrawCommand_Instance,
    SmlDrawCommand_Instanced,
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

struct sml_draw_command_instance
{
    sml_instance Instance;
};

struct sml_draw_command_instanced
{
    sml_instanced Instanced;
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
Sml_DrawInstance(sml_renderer *Renderer, sml_instance Instance)
{
    sml_draw_command_header Header = {};
    Header.Type = SmlDrawCommand_Instance;
    Header.Size = sizeof(sml_draw_command_instance);

    sml_draw_command_instance Payload = {};
    Payload.Instance = Instance;

    Sml_PushToRuntimeCommandBuffer(Renderer, &Header, sizeof(sml_draw_command_header));
    Sml_PushToRuntimeCommandBuffer(Renderer, &Payload, Header.Size);
}

static void
Sml_DrawInstanced(sml_renderer *Renderer, sml_instanced Instanced)
{
    sml_draw_command_header Header = {};
    Header.Type = SmlDrawCommand_Instanced;
    Header.Size = sizeof(sml_draw_command_instanced);

    sml_draw_command_instanced Payload = {};
    Payload.Instanced = Instanced;

    Sml_PushToRuntimeCommandBuffer(Renderer, &Header, sizeof(sml_draw_command_header));
    Sml_PushToRuntimeCommandBuffer(Renderer, &Payload, Header.Size);
}
