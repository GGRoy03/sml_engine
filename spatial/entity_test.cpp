// ===================================
// Type Definitions
// ===================================

#define SML_MAX_ENT 16
enum class sml_entity_id : sml_u32 {};

enum SmlEntity_Type
{
    SmlEntity_Unknown,

    SmlEntity_Instance,
    SmlEntity_Instanced,
};

struct sml_entity
{
    // Core-data
    sml_vector3 Position;
    sml_u32     Material;

    // Meta-data
    char Name[64];
    bool Alive;

    // Backend-specific data
    SmlEntity_Type Type;
    union
    {
        instance Instance;
    } Data;
};

struct sml_entity_manager
{
    sml_entity    *Pool;
    sml_entity_id *FreeList;
    sml_u32        Capacity;
    sml_u32        FreeCount;
};

// ===================================
// Global Variables
// ===================================

static sml_entity_manager EntityManager = {};

static inline sml_entity*
SmlInt_GetEntityPointer(sml_u32 Id)
{
    return &EntityManager.Pool[Id];
}

static void
SmlInt_EnsurePool()
{
    if (EntityManager.Pool) return;

    EntityManager.Capacity  = SML_MAX_ENT;
    EntityManager.FreeCount = SML_MAX_ENT;
    EntityManager.Pool      = (sml_entity*)malloc(sizeof(sml_entity) * SML_MAX_ENT);
    EntityManager.FreeList  = 
        (sml_entity_id*)malloc(sizeof(sml_entity_id) * SML_MAX_ENT);

    for (sml_u32 Index = 0; Index < SML_MAX_ENT; ++Index)
    {
        EntityManager.FreeList[Index]   = sml_entity_id(SML_MAX_ENT - 1 - Index);
        EntityManager.Pool[Index].Alive = false;
    }
}

// ===================================
// User API
// ===================================

static sml_entity_id
Sml_CreateEntity(sml_heap_block VtxHeap, sml_heap_block IdxHeap, sml_u32 IdxCount,
                 sml_vector3 Position, sml_u32 Material, const char *Identifier)
{
    Sml_Unused(VtxHeap);
    Sml_Unused(IdxHeap);
    Sml_Unused(IdxCount);

    SmlInt_EnsurePool();

    if (EntityManager.FreeCount == 0)
    {
        Sml_Assert(!"Max entity count reached.");
        return sml_entity_id(0);
    }

    sml_entity_id Id  = EntityManager.FreeList[--EntityManager.FreeCount];
    sml_u32       Idx = sml_u32(Id);
    sml_entity   *E   = SmlInt_GetEntityPointer(Idx);

    // WARN: Should use the new API to create an instance.

    E->Type     = SmlEntity_Instance;
    E->Position = Position;
    E->Material = Material;
    E->Alive    = true;

    strncpy(E->Name, Identifier, 63);

    return Id;
}

static void
Sml_UpdateEntity(sml_entity_id EntityId)
{
    sml_entity *E = SmlInt_GetEntityPointer(sml_u32(EntityId));

    switch (E->Type)
    {

    case SmlEntity_Instance:
    {
    } break;

    case SmlEntity_Instanced:
    {
    } break;

    default:
        Sml_Assert(!"Cannot update entity of unknown type");
        return;

    }
}

static void
Sml_DrawEntity(sml_entity_id EntityId)
{
    sml_entity *E = SmlInt_GetEntityPointer(sml_u32(EntityId));

    switch (E->Type)
    {

    case SmlEntity_Instance:
    {
    } break;

    case SmlEntity_Instanced:
    {
    } break;

    default:
        Sml_Assert(!"Cannot draw entity of unknown type");
        return;
    }
}

static void
SmlDebug_ShowEntityUI()
{
    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 20, 20, 255));
    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(255, 140, 0, 200));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(255, 165, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30, 30, 30, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(40, 40, 40, 255));

    ImGui::Begin("Entity Debug", nullptr, Flags);

    for (sml_u32 Index = 0; Index < EntityManager.Capacity; ++Index)
    {
        sml_entity* E = SmlInt_GetEntityPointer(Index);
        if (!E->Alive) continue;

        char Header[32];
        sprintf_s(Header, 32, "%s##%u", E->Name, Index);

        if (ImGui::CollapsingHeader(Header, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Columns(2, "EntityData", false);
            ImGui::SetColumnWidth(0, 80);

            ImGui::Text("Name");    ImGui::NextColumn();
            ImGui::Text(E->Name);   ImGui::NextColumn();

            ImGui::Text("Position");ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            char PosLabel[32];
            sprintf_s(PosLabel, 32, "##pos%u", Index);
            if (ImGui::InputFloat3(PosLabel, &E->Position.x))
            {
                Sml_UpdateEntity(sml_entity_id(Index));
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::NextColumn();
            ImGui::Columns(1);
        }
    }

    ImGui::End();
    ImGui::PopStyleColor(5);
}
