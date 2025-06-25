///////////////////////////////////////////////////////
///                  SETUP COMMANDS                 ///
///////////////////////////////////////////////////////

// ===================================
//  Type Definitions
// ===================================

enum SmlSetupCommand_Type
{
    SmlSetupCommand_None,

    SmlSetupCommand_StaticGroup,
};

struct sml_setup_command_header
{
    SmlSetupCommand_Type Type;
    sml_u32              Size;
};

struct sml_setup_command_static_group
{
    sml_pipeline_desc *PipelineDesc;

    sml_static_mesh *Meshes[4];
    sml_u32          MeshCount;

    size_t VertexDataSize;
    size_t IndexDataSize;
};

struct sml_static_group_builder
{
    sml_pipeline_desc *Pipeline;
    sml_static_mesh   *Meshes[4];
    sml_u32            MeshCount;
};


// ===================================
//  Global Variables
// ===================================

// TODO: Some cache for static groups

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

// WARN:
// 1) Does not handle overflows gracefully.

static void
Sml_AddMeshToBuilder(sml_static_group_builder *Builder, sml_static_mesh *Mesh)
{
    Sml_Assert(Builder->MeshCount != 4 && Builder->Meshes);

    Builder->Meshes[Builder->MeshCount++] = Mesh;
}

// WARN:
// 1) This does not free the meshes / does not use the engine's allocator.

static sml_u32
Sml_SetupStaticGroup(sml_renderer *Renderer, sml_static_group_builder *Builder)
{
    sml_setup_command_header Header = {};
    Header.Type                     = SmlSetupCommand_StaticGroup;
    Header.Size                     = sizeof(sml_setup_command_static_group);

    sml_setup_command_static_group Payload = {};
    Payload.PipelineDesc                   = Builder->Pipeline;
    Payload.MeshCount                      = Builder->MeshCount;

    // Meshes
    for(u32 Index = 0; Index < Builder->MeshCount; Index++)
    {
        sml_static_mesh *Mesh = Builder->Meshes[Index];

        Payload.VertexDataSize += Mesh->VertexDataSize;
        Payload.IndexDataSize  += Mesh->IndexDataSize;
        Payload.Meshes[Index]   = Mesh;
    }

    Sml_PushToOfflineBuffer(Renderer, &Header , sizeof(sml_setup_command_header));
    Sml_PushToOfflineBuffer(Renderer, &Payload, Header.Size);

    return 0;
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
    SmlDrawCommand_StaticGroup,
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
Sml_ClearScreen(sml_renderer *Renderer, sml_vector4 Color)
{
    sml_draw_command_header Header = {};
    Header.Type                    = SmlDrawCommand_Clear;
    Header.Size                    = sizeof(sml_draw_command_clear);

    sml_draw_command_clear Payload = {};
    Payload.Color                  = Color;

    Sml_PushToRuntimeCommandBuffer(Renderer, &Header, sizeof(sml_draw_command_header));
    Sml_PushToRuntimeCommandBuffer(Renderer, &Payload, Header.Size);
}
