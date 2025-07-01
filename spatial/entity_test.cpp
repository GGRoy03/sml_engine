#define SML_MAX_ENT 16

enum class sml_entity_instance  : sml_u32 {};
enum class sml_entity_instanced : sml_u32 {};

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

    union
    {
        sml_instance  Instance;
        sml_instanced Instanced;
    } BackendHandle;
};

struct sml_entity_manager
{
    sml_entity *Pool;
    sml_u32    *FreeList;
    sml_u32     Capacity;
    sml_u32     FreeCount;
};

static sml_entity_manager EntityManager;

static inline sml_entity*
SmlInt_GetEntityPointer(sml_u32 Id)
{
    sml_entity *Entity = EntityManager.Pool + Id;

    return Entity;
}


static sml_entity_instance
Sml_CreateEntity(sml_mesh *Mesh, sml_vector3 Position, sml_u32 Material)
{
    if(!EntityManager.Pool)
    {
        EntityManager.Pool      = (sml_entity*)malloc(SML_MAX_ENT*sizeof(sml_entity));
        EntityManager.FreeList  = (sml_u32*)malloc(SML_MAX_ENT*sizeof(sml_u32));
        EntityManager.Capacity  = SML_MAX_ENT;
        EntityManager.FreeCount = SML_MAX_ENT;

        for(u32 Index = 0; Index < SML_MAX_ENT; Index++)
        {
            EntityManager.FreeList[Index] = SML_MAX_ENT - 1 - Index;
        }
    }

    if(EntityManager.FreeCount == 0)
    {
        Sml_Assert(!"Max entity count reached.");
        return sml_entity_instance(0);
    }

    sml_entity_instance Id =
        sml_entity_instance(EntityManager.FreeList[--EntityManager.FreeCount]);

    sml_entity *Entity = EntityManager.Pool + sml_u32(Id);
    Entity->Position = Position;
    Entity->Material = Material;
    Entity->Mesh     = Mesh;
    Entity->Type     = SmlEntity_Instance;

    Entity->BackendHandle.Instance = Sml_SetupInstance(Entity->Mesh, Entity->Material);

    Sml_UpdateInstance(Entity->Position, Entity->BackendHandle.Instance);

    return Id;
}

// WARN:
// 1) This function simply does not work yet.

static sml_entity_instanced
Sml_CreateEntity(sml_mesh *Mesh, sml_u32 Material, sml_u32 Count)
{
    Sml_Unused(Count);

    if(!EntityManager.Pool)
    {
        EntityManager.Pool      = (sml_entity*)malloc(SML_MAX_ENT*sizeof(sml_entity));
        EntityManager.FreeList  = (sml_u32*)malloc(SML_MAX_ENT*sizeof(sml_u32));
        EntityManager.Capacity  = SML_MAX_ENT;
        EntityManager.FreeCount = SML_MAX_ENT;

        for(u32 Index = 0; Index < SML_MAX_ENT; Index++)
        {
            EntityManager.FreeList[Index] = SML_MAX_ENT - 1 - Index;
        }
    }

    if(EntityManager.FreeCount == 0)
    {
        Sml_Assert(!"Max entity count reached.");
        return sml_entity_instanced(0);
    }

    sml_entity_instanced Id =
        sml_entity_instanced(EntityManager.FreeList[--EntityManager.FreeCount]);

    sml_entity *Entity = EntityManager.Pool + sml_u32(Id);
    Entity->Material = Material;
    Entity->Mesh     = Mesh;
    Entity->Type     = SmlEntity_Instanced;

    return Id;
}

static void
Sml_UpdateEntity(sml_entity_instance EntityId, sml_vector3 Position)
{
    sml_entity *Entity = SmlInt_GetEntityPointer(sml_u32(EntityId));
    Entity->Position = Position;

    Sml_UpdateInstance(Position, Entity->BackendHandle.Instance);
}

static void
Sml_DrawEntity(sml_entity_instance EntityId)
{
    sml_entity *Entity = SmlInt_GetEntityPointer(sml_u32(EntityId));
    Sml_DrawInstance(Entity->BackendHandle.Instance);
}

static void
Sml_ShowEntityDebugUI()
{
    ImGui::Begin("Entity Debug");

    for (sml_u32 Index = 0; Index < EntityManager.Capacity; ++Index)
    {
        sml_entity *E = SmlInt_GetEntityPointer(Index);

        char Label[32];
        sprintf_s(Label, 32, "Entity %u", Index);

        if (ImGui::TreeNode(Label))
        {
            // NOTE: Then this needs to switch? Or we do a list for
            // every type? Not sure.

            if (ImGui::InputFloat3("Position", &E->Position.x))
            {
            }

            ImGui::TreePop();
        }
    }

    ImGui::End();
}
