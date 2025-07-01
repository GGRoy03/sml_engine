#define SML_MAX_ENT 16

enum class sml_entity_id : sml_u32 {};

enum SmlEntity_Type
{
    SmlEntity_Unknown,

    SmlEntity_Instance,
    SmlEntity_Instanced,
};

struct sml_mesh;
struct sml_entity
{
    SmlEntity_Type Type;

    sml_mesh   *Mesh;
    sml_vector3 Position;
    sml_u32     Material;

    // Meta-data
    char Name[64];

    union
    {
        sml_instance  Instance;
        sml_instanced Instanced;
    } Data;
};

struct sml_entity_manager
{
    sml_entity    *Pool;
    sml_entity_id *FreeList;
    bool          *Alive;
    sml_u32       Capacity;
    sml_u32       FreeCount;
};

static sml_entity_manager EntityManager;

static inline sml_entity*
SmlInt_GetEntityPointer(sml_u32 Id)
{
    sml_entity *Entity = EntityManager.Pool + Id;

    return Entity;
}

// WARN:
// 1) Uses malloc

static sml_entity_id
Sml_CreateEntity(sml_mesh *Mesh, sml_vector3 Position, sml_u32 Material, 
                 const char *Identifier)
{
    if(!EntityManager.Pool)
    {
        EntityManager.Pool      = (sml_entity*)malloc(SML_MAX_ENT*sizeof(sml_entity));
        EntityManager.FreeList  = (sml_entity_id*)malloc(SML_MAX_ENT*sizeof(sml_u32));
        EntityManager.Alive     = (bool*)malloc(SML_MAX_ENT * sizeof(bool));
        EntityManager.Capacity  = SML_MAX_ENT;
        EntityManager.FreeCount = SML_MAX_ENT;

        for(sml_u32 Index = 0; Index < SML_MAX_ENT; Index++)
        {
            EntityManager.FreeList[Index] = sml_entity_id(SML_MAX_ENT - 1 - Index);
            EntityManager.Alive[Index]    = false;
        }
    }

    if(EntityManager.FreeCount == 0)
    {
        Sml_Assert(!"Max entity count reached.");
        return sml_entity_id(0);
    }

    sml_entity_id Id = EntityManager.FreeList[--EntityManager.FreeCount];

    sml_entity *Entity = EntityManager.Pool + sml_u32(Id);
    Entity->Position      = Position;
    Entity->Material      = Material;
    Entity->Mesh          = Mesh;
    Entity->Type          = SmlEntity_Instance;
    Entity->Data.Instance = Sml_SetupInstance(Entity->Mesh, Entity->Material);

    size_t Length = strlen(Identifier);
    memcpy(Entity->Name, Identifier, Length);

    EntityManager.Alive[sml_u32(Id)] = true;

    Sml_UpdateInstance(Entity->Position, Entity->Data.Instance);

    return Id;
}

// WARN:
// 1) This function simply does not work yet.
// 2) Uses malloc

static sml_entity_id
Sml_CreateEntity(sml_mesh *Mesh, sml_u32 Material, sml_u32 Count)
{
    Sml_Unused(Count);

    if(!EntityManager.Pool)
    {
        EntityManager.Pool      = (sml_entity*)malloc(SML_MAX_ENT*sizeof(sml_entity));
        EntityManager.FreeList  = (sml_entity_id*)malloc(SML_MAX_ENT*sizeof(sml_u32));
        EntityManager.Capacity  = SML_MAX_ENT;
        EntityManager.FreeCount = SML_MAX_ENT;

        for(u32 Index = 0; Index < SML_MAX_ENT; Index++)
        {
            EntityManager.FreeList[Index] = sml_entity_id(SML_MAX_ENT - 1 - Index);
            EntityManager.Alive[Index]    = false;
        }
    }

    if(EntityManager.FreeCount == 0)
    {
        Sml_Assert(!"Max entity count reached.");
        return sml_entity_id(0);
    }

    sml_entity_id Id = EntityManager.FreeList[--EntityManager.FreeCount];

    sml_entity *Entity = EntityManager.Pool + sml_u32(Id);
    Entity->Material = Material;
    Entity->Mesh     = Mesh;
    Entity->Type     = SmlEntity_Instanced;

    return Id;
}

static void
Sml_UpdateEntity(sml_entity_id EntityId)
{
    sml_entity *Entity = SmlInt_GetEntityPointer(sml_u32(EntityId));

    switch(Entity->Type)
    {

    case SmlEntity_Instance:
    {
        Sml_UpdateInstance(Entity->Position, Entity->Data.Instance);
    } break;

    case SmlEntity_Instanced:
    {
    } break;

    default:
        Sml_Assert(!"Cannot update entity with type: unknown");
        return;

    }
}

static void
Sml_DrawEntity(sml_entity_id EntityId)
{
    sml_entity *Entity = SmlInt_GetEntityPointer(sml_u32(EntityId));

    switch(Entity->Type)
    {

    case SmlEntity_Instance:
    {
        Sml_DrawInstance(Entity->Data.Instance);
    } break;

    case SmlEntity_Instanced:
    {
    } break;

    default:
        Sml_Assert(!"Cannot update entity with type: unknown");
        return;

    }
}

static void
Sml_ShowEntityDebugUI()
{
    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 20, 20, 255));
    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(255, 140, 0, 200));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(255, 165, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30, 30, 30, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(40, 40, 40, 255));

    ImGui::Begin("Entity Debug", nullptr, Flags);

    for (sml_u32 Index = 0; Index < EntityManager.Capacity; ++Index)
    {
        if (!EntityManager.Alive[Index]) continue;

        sml_entity* E = SmlInt_GetEntityPointer(Index);

        char HeaderLabel[32];
        sprintf_s(HeaderLabel, 32, "Entity %u", Index);

        if (ImGui::CollapsingHeader(HeaderLabel, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Columns(2, "EntityData", false);
            ImGui::SetColumnWidth(0, 80);

            ImGui::Text("Name");      ImGui::NextColumn();
            ImGui::Text(E->Name);    ImGui::NextColumn();

            ImGui::Text("Position");  ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            char PosLabel[32];
            sprintf_s(PosLabel, 32, "##pos%u", Index);
            if (ImGui::InputFloat3(PosLabel, &E->Position.x))
            {
                Sml_UpdateEntity(sml_entity_id(Index));
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
        }
    }

    ImGui::End();

    ImGui::PopStyleColor(5);
}
