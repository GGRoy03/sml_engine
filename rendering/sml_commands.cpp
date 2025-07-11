namespace SML 
{

// ===================================
// Type Definitions
// ===================================


// NOTE: Could every setup command be lazy initalized? With a better system?
// API would be : GetXXHandle, we use that handle to check if init, if now we
// do init with data? Is the data different?

enum Command_Type
{
    Command_None,

    // Setup
    SetupCommand_Material,
    SetupCommand_Instance,
    SetupCommand_Instanced,

    // Update
    UpdateCommand_Camera,
    UpdateCommand_InstanceData,
    UpdateCommand_InstancedData,

    // Draw
    DrawCommand_Clear,
    DrawCommand_DrawInstance,
    DrawCommand_DrawInstanced,
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

struct update_command_camera
{
    sml_matrix4 ViewProjection;
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
PushRenderCommand(command_header *Header, void *Payload, sml_u32 PayloadSize)
{
    size_t NeededSize = sizeof(command_header) + PayloadSize;
    if(Renderer->CommandPushSize + NeededSize <= Renderer->CommandPushCapacity)
    {
        sml_u8 *WritePointer = 
            (sml_u8*)Renderer->CommandPushBase + Renderer->CommandPushSize;

        memcpy(WritePointer, Header , sizeof(command_header));
        WritePointer += sizeof(command_header);

        memcpy(WritePointer, Payload, PayloadSize);

        Renderer->CommandPushSize += NeededSize;
    }
    else
    {
        Sml_Assert(!"Command push buffer overflow.\n");
    }
}

// ===================================
// User API
// ===================================

static sml_u32
SetupMaterial(sml_material_texture *MatTexArray, sml_u32 MatTexCount,
              sml_bit_field Features, sml_bit_field Flags)
{
    command_header Header = {};
    Header.Type = SetupCommand_Material;
    Header.Size = sizeof(setup_command_material);

    setup_command_material Payload = {};
    Payload.MaterialTextureArray = MatTexArray;
    Payload.MaterialTextureCount = MatTexCount;
    Payload.Features             = Features;
    Payload.Flags                = Flags;
    Payload.MaterialIndex        = Renderer->Materials.Count;

    PushRenderCommand(&Header, &Payload, Header.Size);

    ++Renderer->Materials.Count;

    return Payload.MaterialIndex;
}

static sml_instance
SetupInstance(sml_heap_block VtxHeap, sml_heap_block IdxHeap, sml_u32 IdxCount,
              sml_u32 Material, sml_bit_field Flags)
{
    command_header Header = {};
    Header.Type = SetupCommand_Instance;
    Header.Size = (sml_u32)sizeof(setup_command_instance);

    setup_command_instance Payload = {};
    Payload.Flags       = Flags;
    Payload.VtxHeapData = VtxHeap;
    Payload.IdxHeapData = IdxHeap;
    Payload.IdxCount    = IdxCount;
    Payload.Material    = Material;
    Payload.Instance    = (sml_instance)Renderer->Instances.Count;

    PushRenderCommand(&Header, &Payload, Header.Size);

    ++Renderer->Instances.Count;
    return Payload.Instance;
}

static void
UpdateInstance(sml_vector3 Data, sml_instance Instance)
{
    command_header Header = {};
    Header.Type = UpdateCommand_InstanceData;
    Header.Size = (sml_u32)sizeof(update_command_instance_data);

    update_command_instance_data Payload = {};
    Payload.Data     = Data;
    Payload.Instance = Instance;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

static void
UpdateInstanced(sml_vector3 *Data, sml_instanced Instanced)
{
    command_header Header = {};
    Header.Type = UpdateCommand_InstancedData;
    Header.Size = (sml_u32)sizeof(update_command_instanced_data);

    update_command_instanced_data Payload = {};
    Payload.Data      = Data;
    Payload.Instanced = Instanced;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

static void
UpdateCamera(sml_matrix4 Data)
{
    command_header Header = {};
    Header.Type = UpdateCommand_Camera;
    Header.Size = sml_u32(sizeof(update_command_camera));

    update_command_camera Payload = {};
    Payload.ViewProjection = Data;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

static void
DrawClearScreen(sml_vector4 Color)
{
    command_header Header = {};
    Header.Type = DrawCommand_Clear;
    Header.Size = (sml_u32)sizeof(draw_command_clear);

    draw_command_clear Payload = {};
    Payload.Color = Color;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

static void
DrawInstance(sml_instance Instance)
{
    command_header Header = {};
    Header.Type = DrawCommand_DrawInstance;
    Header.Size = (sml_u32)sizeof(draw_command_instance);

    draw_command_instance Payload = {};
    Payload.Instance = Instance;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

static void
DrawInstanced(sml_instanced Instanced)
{
    command_header Header = {};
    Header.Type = DrawCommand_DrawInstanced;
    Header.Size = (sml_u32)sizeof(draw_command_instanced);

    draw_command_instanced Payload = {};
    Payload.Instanced = Instanced;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

} // namespace SML
