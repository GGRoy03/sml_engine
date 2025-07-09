namespace SML 
{

// ===================================
// Type Definitions
// ===================================

enum Command_Type
{
    Command_None,

    Command_Material,
    Command_Instance,
    Command_Instanced,

    Command_InstanceData,
    Command_InstancedData,

    Command_Clear,
    Command_DrawInstance,
    Command_DrawInstanced,
};

enum Command_Flag
{
    Command_InstanceFreeHeap = 1 << 0,
};

struct command_header
{
    Command_Type Type;
    sml_u32      Size;
};

struct setup_command_material
{
    sml_material_texture *MaterialTextureArray;
    sml_u32               MaterialTextureCount;

    sml_bit_field Features;
    sml_bit_field Flags;
    sml_u32       MaterialIndex;
};

struct setup_command_instance
{
    sml_bit_field Flags;

    sml_heap_block VtxHeapData;
    sml_heap_block IdxHeapData;
    sml_u32        IdxCount;

    sml_u32      Material;
    sml_instance Instance;
};

struct update_command_instance_data
{
    sml_vector3  Data;
    sml_instance Instance;
};

struct update_command_instanced_data
{
    sml_vector3   *Data;
    sml_instanced  Instanced;
};

struct draw_command_clear
{
    sml_vector4 Color;
};

struct draw_command_instance
{
    sml_instance Instance;
};

struct draw_command_instanced
{
    sml_instanced Instanced;
};

// ===================================
// Internal Helpers
// ===================================

static void
PushCommand(void *Data, size_t Size)
{
    if (Renderer->CommandPushSize + Size <= Renderer->CommandPushCapacity)
    {
        void *WritePointer =
            (sml_u8*)Renderer->CommandPushBase + Renderer->CommandPushSize;
        memcpy(WritePointer, Data, Size);
        Renderer->CommandPushSize += Size;
    }
    else
    {
        Sml_Assert(!"Push buffer exceeds maximum size");
    }
}

// ===================================
// User API
// ===================================

static sml_u32
SetupMaterial(sml_material_texture *MatTexArray, sml_u32 MatTexCount,
              sml_bit_field Features, sml_bit_field Flags)
{
    size_t PayloadSize = sizeof(setup_command_material);

    command_header Header = {};
    Header.Type = Command_Material;
    Header.Size = (sml_u32)PayloadSize;

    setup_command_material Payload = {};
    Payload.MaterialTextureArray = MatTexArray;
    Payload.MaterialTextureCount = MatTexCount;
    Payload.Features             = Features;
    Payload.Flags                = Flags;
    Payload.MaterialIndex        = Renderer->Materials.Count;

    PushCommand(&Header, sizeof(command_header));
    PushCommand(&Payload, PayloadSize);

    ++Renderer->Materials.Count;
    return Payload.MaterialIndex;
}

static sml_instance
SetupInstance(sml_heap_block VtxHeap, sml_heap_block IdxHeap, sml_u32 IdxCount,
              sml_u32 Material, sml_bit_field Flags)
{
    command_header Header = {};
    Header.Type = Command_Instance;
    Header.Size = (sml_u32)sizeof(setup_command_instance);

    setup_command_instance Payload = {};
    Payload.Flags       = Flags;
    Payload.VtxHeapData = VtxHeap;
    Payload.IdxHeapData = IdxHeap;
    Payload.IdxCount    = IdxCount;
    Payload.Material    = Material;
    Payload.Instance    = (sml_instance)Renderer->Instances.Count;

    PushCommand(&Header, sizeof(command_header));
    PushCommand(&Payload, Header.Size);

    ++Renderer->Instances.Count;
    return Payload.Instance;
}

static void
UpdateInstance(sml_vector3 Data, sml_instance Instance)
{
    command_header Header = {};
    Header.Type = Command_InstanceData;
    Header.Size = (sml_u32)sizeof(update_command_instance_data);

    update_command_instance_data Payload = {};
    Payload.Data     = Data;
    Payload.Instance = Instance;

    PushCommand(&Header, sizeof(command_header));
    PushCommand(&Payload, Header.Size);
}

static void
UpdateInstanced(sml_vector3 *Data, sml_instanced Instanced)
{
    command_header Header = {};
    Header.Type = Command_InstancedData;
    Header.Size = (sml_u32)sizeof(update_command_instanced_data);

    update_command_instanced_data Payload = {};
    Payload.Data      = Data;
    Payload.Instanced = Instanced;

    PushCommand(&Header, sizeof(command_header));
    PushCommand(&Payload, Header.Size);
}

static void
DrawClearScreen(sml_vector4 Color)
{
    command_header Header = {};
    Header.Type = Command_Clear;
    Header.Size = (sml_u32)sizeof(draw_command_clear);

    draw_command_clear Payload = {};
    Payload.Color = Color;

    PushCommand(&Header, sizeof(command_header));
    PushCommand(&Payload, Header.Size);
}

static void
DrawInstance(sml_instance Instance)
{
    command_header Header = {};
    Header.Type = Command_DrawInstance;
    Header.Size = (sml_u32)sizeof(draw_command_instance);

    draw_command_instance Payload = {};
    Payload.Instance = Instance;

    PushCommand(&Header, sizeof(command_header));
    PushCommand(&Payload, Header.Size);
}

static void
DrawInstanced(sml_instanced Instanced)
{
    command_header Header = {};
    Header.Type = Command_DrawInstanced;
    Header.Size = (sml_u32)sizeof(draw_command_instanced);

    draw_command_instanced Payload = {};
    Payload.Instanced = Instanced;

    PushCommand(&Header, sizeof(command_header));
    PushCommand(&Payload, Header.Size);
}

} // namespace SML
