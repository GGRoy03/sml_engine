// ===================================
// Type Definitions
// ===================================

enum RenderCommand_Type
{
    Command_None,

    // Setup
    SetupCommand_Instanced,

    // Context
    ContextCommand_Material,

    // Update
    UpdateCommand_Camera,
    UpdateCommand_Instance,
    UpdateCommand_Material,

    // Draw
    DrawCommand_Clear,
    DrawCommand_DrawInstance,
    DrawCommand_DrawInstanced,
};

enum RenderCommand_Flag
{
    RenderCommand_FreeHeap    = 1 << 0,
    RenderCommand_DebugShader = 1 << 1,
};

struct command_header
{
    RenderCommand_Type Type;
    sml_u32            Size;
};

struct context_command_material
{
    sml_bit_field ShaderFeatures;
};

struct update_command_material
{
    // Meta-Data
    texture       Texture;
    Material_Type Type;

    // Misc
    sml_bit_field Flags;

    // Handle
    material Material;
};

struct update_command_instance
{
    // Backend
    material Material;

    // Meta-Data
    sml_heap_block VtxHeap;
    sml_heap_block IdxHeap;
    sml_u32        IdxCount;
    sml_vector3    Position;

    // Misc
    sml_bit_field Flags;

    // Handle
    instance Instance;
};

struct update_command_instanced_data
{
    sml_vector3  *Data;
    sml_instanced Instanced;
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

static void
UpdateMaterial(material Material, texture Texture, Material_Type Type, 
               sml_bit_field Flags)
{
    command_header Header = {};
    Header.Type = UpdateCommand_Material;
    Header.Size = sizeof(update_command_material);

    update_command_material Payload = {};
    Payload.Texture  = Texture;
    Payload.Type     = Type;
    Payload.Flags    = Flags;
    Payload.Material = Material;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

static void 
UpdateInstance(instance Instance, sml_vector3 Position)
{
    command_header Header = {};
    Header.Type = UpdateCommand_Instance;
    Header.Size = sizeof(update_command_instance);

    update_command_instance Payload = {};
    Payload.Position = Position;
    Payload.Instance = Instance;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

static void
UpdateInstance(instance Instance, sml_heap_block VtxHeap, sml_heap_block IdxHeap,
               sml_u32 IdxCount, material Material, sml_bit_field Flags)
{
    command_header Header = {};
    Header.Type = UpdateCommand_Instance;
    Header.Size = sizeof(update_command_instance);

    update_command_instance Payload = {};
    Payload.VtxHeap  = VtxHeap;
    Payload.IdxHeap  = IdxHeap;
    Payload.IdxCount = IdxCount;
    Payload.Material = Material;
    Payload.Flags    = Flags;
    Payload.Instance = Instance;

    PushRenderCommand(&Header, &Payload, Header.Size);
}

static void
UpdateInstance(instance Instance, sml_heap_block VtxHeap, sml_heap_block IdxHeap,
               sml_u32 IdxCount, material Material, sml_vector3 Position, 
               sml_bit_field Flags)
{
    command_header Header = {};
    Header.Type = UpdateCommand_Instance;
    Header.Size = sizeof(update_command_instance);

    update_command_instance Payload = {};
    Payload.VtxHeap  = VtxHeap;
    Payload.IdxHeap  = IdxHeap;
    Payload.IdxCount = IdxCount;
    Payload.Material = Material;
    Payload.Position = Position;
    Payload.Flags    = Flags;
    Payload.Instance = Instance;

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
