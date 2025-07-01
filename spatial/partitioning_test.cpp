#include "math.h" // cosf

struct sml_walkable_tri
{
    sml_vector3 v0, v1, v2;
    sml_vector3 Normal;
};

struct sml_walkable_list
{
    sml_walkable_tri *Data;
    sml_u32           Count;
};

// WARN: 
// 1) Uses malloc

static sml_walkable_list
SmlInt_ExtractWalkableList(sml_mesh *Mesh, sml_f32 MaxSlopeDegree)
{
    sml_walkable_list List = {};

    sml_u32 TriCount = sml_u32(Mesh->IndexDataSize / sizeof(sml_u32)) / 3;
    List.Data        = (sml_walkable_tri*)malloc(TriCount * sizeof(sml_walkable_tri));
    List.Count       = 0;

    sml_f32     SlopeThresold = cosf(MaxSlopeDegree * (3.14158265f / 180.0f));
    sml_vertex *VertexData    = (sml_vertex*)Mesh->VertexData;

    for(sml_u32 TriIdx = 0; TriIdx < TriCount; TriIdx++)
    {
        sml_walkable_tri Tri  = {};
        sml_u32          Base = TriIdx * 3;

        // Fetch three vertices from the mesh
        Tri.v0 = VertexData[Mesh->IndexData[Base + 0]].Position;
        Tri.v1 = VertexData[Mesh->IndexData[Base + 1]].Position;
        Tri.v2 = VertexData[Mesh->IndexData[Base + 2]].Position;

        // Compute the normal from the edge vectors
        sml_vector3 Edge0  = Tri.v1 - Tri.v0;
        sml_vector3 Edge1  = Tri.v2 - Tri.v0;
        sml_vector3 Normal = SmlVec3_Normalize(SmlVec3_VectorProduct(Edge0, Edge1));

        // Check if slope is walkable -> How does that even work? Normal.y == Dot(N, U)
        // Dot -> How much it points in the same direction as U. Pointing same dir = high value = not-walkable
        // So why the fuck do we check for >= SlopeThresold??

        if(Normal.y >= SlopeThresold)
        {
            List.Data[List.Count++] = Tri;
        }

    }

    return List;
}

// WARN: 
// 1) Does not produce optimized indexed meshes. (Store indices in list)
// 2) Uses malloc/free
// 3) Not really clear that it free the walkable list.

static sml_instance
SmlInt_BuildWalkableInstance(sml_walkable_list *List, sml_vector3 Color)
{
    sml_mesh *Mesh = (sml_mesh*)malloc(sizeof(sml_mesh));

    Mesh->VertexDataSize = List->Count * 3 * sizeof(sml_vertex_color);
    Mesh->IndexDataSize  = List->Count * 3 * sizeof(sml_u32);

    Mesh->VertexData = malloc(Mesh->VertexDataSize);
    Mesh->IndexData  = (sml_u32*)malloc(Mesh->IndexDataSize);

    sml_vertex_color *V = (sml_vertex_color*)Mesh->VertexData;
    sml_u32          *I = (sml_u32*)Mesh->IndexData;

    for (sml_u32 Tri = 0; Tri < List->Count; ++Tri)
    {
        sml_walkable_tri *T    = &List->Data[Tri];
        sml_u32           Base = Tri * 3;

        V[Base + 0].Position = T->v0;
        V[Base + 1].Position = T->v1;
        V[Base + 2].Position = T->v2;

        V[Base + 0].Normal = T->Normal;
        V[Base + 1].Normal = T->Normal;
        V[Base + 2].Normal = T->Normal;

        V[Base + 0].Color = Color;
        V[Base + 1].Color = Color;
        V[Base + 2].Color = Color;

        I[Base + 0] = Base + 0;
        I[Base + 1] = Base + 1;
        I[Base + 2] = Base + 2;
    }

    free(List->Data);
    List->Data = nullptr;

    sml_u32      Material = Sml_SetupMaterial(nullptr, 0, SmlShaderFeat_Color, 0);
    sml_instance Instance = Sml_SetupInstance(Mesh, Material);

    return Instance;
}
