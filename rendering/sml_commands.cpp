///////////////////////////////////////////////////////
///                  COMMAND TYPES                  ///
///////////////////////////////////////////////////////

enum SmlCommand_Type
{
    SmlCommand_None,

    SmlCommand_Material,
    SmlCommand_Instance,
    SmlCommand_Instanced,

    SmlCommand_InstanceData,
    SmlCommand_InstancedData,

    SmlCommand_Clear,
    SmlCommand_DrawInstance,
    SmlCommand_DrawInstanced,
};

struct sml_command_header
{
    SmlCommand_Type Type;
    sml_u32         Size;
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

struct sml_update_command_instance_data
{
    sml_vector3  Data;
    sml_instance Instance;
};

struct sml_update_command_instanced_data
{
    sml_vector3   *Data;
    sml_instanced  Instanced;
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

///////////////////////////////////////////////////////
///                HELPER FUNCTION                 ///
///////////////////////////////////////////////////////

static void
Sml_PushCommand(sml_renderer *Renderer, void *Data, size_t Size)
{
    if(Renderer->CommandPushSize + Size <= Renderer->CommandPushCapacity)
    {
        void *WritePointer = (sml_u8*)Renderer->CommandPushBase + Renderer->CommandPushSize;

        memcpy(WritePointer, Data, Size);
        Renderer->CommandPushSize += Size;
    }
    else
    {
        Sml_Assert(!"Push buffer exceeds maximum size");
    }
}

///////////////////////////////////////////////////////
///                  USER API                      ///
///////////////////////////////////////////////////////

static sml_u32
Sml_SetupMaterial(sml_renderer *Renderer, sml_material_texture *MatTexArray,
                  sml_u32 MatTexCount, sml_bit_field Features, sml_bit_field Flags)
{
    size_t PayloadSize = sizeof(sml_setup_command_material);

    sml_command_header Header = {};
    Header.Type = SmlCommand_Material;
    Header.Size = (sml_u32)PayloadSize;

    sml_setup_command_material Payload = {};
    Payload.MaterialTextureArray = MatTexArray;
    Payload.MaterialTextureCount = MatTexCount;
    Payload.Features             = Features;
    Payload.Flags                = Flags;
    Payload.MaterialIndex        = Renderer->Materials.Count;

    Sml_PushCommand(Renderer, &Header, sizeof(sml_command_header));
    Sml_PushCommand(Renderer, &Payload, PayloadSize);

    ++Renderer->Materials.Count;

    return Payload.MaterialIndex;
}

static void
Sml_SetupInstance(sml_renderer *Renderer, sml_entity *Entity)
{
    sml_command_header Header = {};
    Header.Type = SmlCommand_Instance;
    Header.Size = (sml_u32)sizeof(sml_setup_command_instance);

    sml_setup_command_instance Payload = {};
    Payload.Material = Entity->Material;
    Payload.Instance = (sml_instance)Renderer->Instances.Count;
    Payload.Mesh     = Entity->Mesh;

    Sml_PushCommand(Renderer, &Header, sizeof(sml_command_header));
    Sml_PushCommand(Renderer, &Payload, Header.Size);

    ++Renderer->Instances.Count;
    Entity->BackendHandle.Instance = Payload.Instance;
}

static sml_instanced
Sml_SetupInstanced(sml_renderer *Renderer, sml_u32 Material, sml_mesh *Mesh,
                   sml_u32 InstanceCount)
{
    sml_command_header Header = {};
    Header.Type = SmlCommand_Instanced;
    Header.Size = (sml_u32)sizeof(sml_setup_command_instanced);

    sml_setup_command_instanced Payload = {};
    Payload.Material  = Material;
    Payload.Instanced = (sml_instanced)Renderer->Instances.Count;
    Payload.Mesh      = Mesh;
    Payload.Count     = InstanceCount;

    Sml_PushCommand(Renderer, &Header, sizeof(sml_command_header));
    Sml_PushCommand(Renderer, &Payload, Header.Size);

    ++Renderer->Instances.Count;

    return Payload.Instanced;
}

static void
Sml_UpdateInstance(sml_renderer *Renderer, sml_vector3 Data, sml_instance Instance)
{
    sml_command_header Header = {};
    Header.Type = SmlCommand_InstanceData;
    Header.Size = (sml_u32)sizeof(sml_update_command_instance_data);

    sml_update_command_instance_data Payload = {};
    Payload.Data     = Data;
    Payload.Instance = Instance;

    Sml_PushCommand(Renderer, &Header, sizeof(sml_command_header));
    Sml_PushCommand(Renderer, &Payload, Header.Size);
}

static void
Sml_UpdateInstanced(sml_renderer *Renderer, sml_vector3 *Data, sml_instanced Instanced)
{
    sml_command_header Header = {};
    Header.Type = SmlCommand_InstancedData;
    Header.Size = (sml_u32)sizeof(sml_update_command_instanced_data);

    sml_update_command_instanced_data Payload = {};
    Payload.Data      = Data;
    Payload.Instanced = Instanced;

    Sml_PushCommand(Renderer, &Header, sizeof(sml_command_header));
    Sml_PushCommand(Renderer, &Payload, Header.Size);
}

static void
Sml_DrawClearScreen(sml_renderer *Renderer, sml_vector4 Color)
{
    sml_command_header Header = {};
    Header.Type = SmlCommand_Clear;
    Header.Size = sizeof(sml_draw_command_clear);

    sml_draw_command_clear Payload = {};
    Payload.Color = Color;

    Sml_PushCommand(Renderer, &Header, sizeof(sml_command_header));
    Sml_PushCommand(Renderer, &Payload, Header.Size);
}

static void
Sml_DrawInstance(sml_renderer *Renderer, sml_instance Instance)
{
    sml_command_header Header = {};
    Header.Type = SmlCommand_DrawInstance;
    Header.Size = sizeof(sml_draw_command_instance);

    sml_draw_command_instance Payload = {};
    Payload.Instance = Instance;

    Sml_PushCommand(Renderer, &Header, sizeof(sml_command_header));
    Sml_PushCommand(Renderer, &Payload, Header.Size);
}

static void
Sml_DrawInstanced(sml_renderer *Renderer, sml_instanced Instanced)
{
    sml_command_header Header = {};
    Header.Type = SmlCommand_DrawInstanced;
    Header.Size = sizeof(sml_draw_command_instanced);

    sml_draw_command_instanced Payload = {};
    Payload.Instanced = Instanced;

    Sml_PushCommand(Renderer, &Header, sizeof(sml_command_header));
    Sml_PushCommand(Renderer, &Payload, Header.Size);
}
