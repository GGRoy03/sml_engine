#include "math.h" // cosf

// NOTE: Long term we want to replace these with our own.
#include <unordered_map>
#include <vector>
#include <stack>

struct sml_walkable_tri
{
    sml_vector3 v0, v1, v2;
    sml_u32     Indices[3];
    sml_vector3 Normal;
};

struct sml_edge_key
{
    sml_u32 EdgeA;
    sml_u32 EdgeB;
};

struct sml_adjacent_tris
{
    sml_u32 V[3];
    sml_u32 Count;
};

struct sml_edge_key_hash
{
    size_t operator()(sml_edge_key const &Key) const noexcept
    {
        return (size_t)Key.EdgeA * 73856093u ^ (size_t)Key.EdgeB * 19349663u;
    }
};

struct sml_edge_key_equal
{
    bool operator()(sml_edge_key const &LeftHSKey,
                    sml_edge_key const &RightHSKey) const noexcept
    {
        return LeftHSKey.EdgeA == RightHSKey.EdgeA &&
               LeftHSKey.EdgeB == RightHSKey.EdgeB;
    }
};

struct sml_walkable_list
{
    sml_walkable_tri *Data;
    sml_u32           Count;

    std::unordered_map<sml_edge_key,
                       std::vector<sml_u32>,
                       sml_edge_key_hash,
                       sml_edge_key_equal> EdgeToTri;
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

        // Fetch the indices
        Tri.Indices[0] = Mesh->IndexData[Base + 0];
        Tri.Indices[1] = Mesh->IndexData[Base + 1];
        Tri.Indices[2] = Mesh->IndexData[Base + 2];

        // Fetch the vertices
        Tri.v0 = VertexData[Tri.Indices[0]].Position;
        Tri.v1 = VertexData[Tri.Indices[1]].Position;
        Tri.v2 = VertexData[Tri.Indices[2]].Position;

        // Compute the normal from the edge vectors
        sml_vector3 Edge0  = Tri.v1 - Tri.v0;
        sml_vector3 Edge1  = Tri.v2 - Tri.v0;
        sml_vector3 Normal = SmlVec3_Normalize(SmlVec3_VectorProduct(Edge0, Edge1));

        Tri.Normal = Normal;

        // Check if slope is walkable
        if(Normal.y >= SlopeThresold)
        {
            List.Data[List.Count++] = Tri;
        }

    }

    return List;
}

// WARN: 
// 1) Does not produce optimized indexed meshes.
// 2) Uses malloc/free
// 3) This should not be used

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

    sml_u32      Material = Sml_SetupMaterial(nullptr, 0, SmlShaderFeat_Color, 0);
    sml_instance Instance = Sml_SetupInstance(Mesh, Material);

    return Instance;
}

static void
SmlInt_BuildTriangleAdjency(sml_walkable_list *List)
{
    std::unordered_map<sml_edge_key, std::vector<sml_u32>,
                       sml_edge_key_hash, sml_edge_key_equal> EdgeToTri;
    EdgeToTri.reserve(List->Count * 3);

    for(sml_u32 TriIndex = 0; TriIndex < List->Count; TriIndex++)
    {
        sml_walkable_tri *Tri = List->Data + TriIndex;

        for(sml_u32 EdgeIndex = 0; EdgeIndex < 3; EdgeIndex++)
        {
            sml_edge_key Key;

            Key.EdgeA = Tri->Indices[EdgeIndex];
            Key.EdgeB = Tri->Indices[(EdgeIndex + 1) % 3];

            if(Key.EdgeA > Key.EdgeB)
            {
                sml_u32 Temp = Key.EdgeA;
                Key.EdgeA    = Key.EdgeB;
                Key.EdgeB    = Temp;
            }

            auto &Vector = EdgeToTri[Key];
            Vector.push_back(TriIndex);
        }
    }

    List->EdgeToTri = EdgeToTri;
}

// WARN:
// 1) Uses malloc/free
// 2) Uses std:: stuff (profile)

static void
SmlInt_ClusterCoplanarPatches(sml_walkable_list *List)
{
    const sml_f32 MaxPlanarAngle = 10.0f;
    const sml_f32 CosThresold    = (MaxPlanarAngle * (3.14159265f/180.0f));

    sml_dynamic_array<sml_adjacent_tris> Adjacents(List->Count);

    for (sml_u32 TriIdx = 0; TriIdx < List->Count; ++TriIdx)
    {
        auto *Tri = List->Data + TriIdx;
        auto *Adj = Adjacents.PushNext();

        if (!Adj) return;

        for (sml_u32 Edge = 0; Edge < 3; ++Edge)
        {
            sml_u32 A = Tri->Indices[Edge];
            sml_u32 B = Tri->Indices[(Edge + 1) % 3];

            if (A > B)
            {
                sml_u32 Temp = A;
                A = B;
                B = Temp;
            }

            sml_edge_key Key { A, B };

            auto &Shared = List->EdgeToTri[Key];
            if (Shared.size() == 2)
            {
                Adj->V[Adj->Count++] = (Shared[0] == TriIdx ? Shared[1] : Shared[0]);
            }
        }
    }

    sml_dynamic_array<bool>                       Visited(List->Count);
    sml_dynamic_array<sml_dynamic_array<sml_u32>> Clusters(0);

    for(sml_u32 TriIndex = 0; TriIndex < List->Count; TriIndex++)
    {
        if(Visited.Values[TriIndex]) continue;

        auto  Stack   = sml_dynamic_array<sml_u32>(0);
        auto *Cluster = Clusters.PushNext();

        Stack.Push(TriIndex);
        Visited.Values[TriIndex] = true;

        while(Stack.Count > 0)
        {
            sml_u32 Current = Stack.Pop();

            Cluster->Push(Current);

            auto *Adj           = Adjacents.Values + Current;
            auto  CurrentNormal = List->Data[Current].Normal;

            for(sml_u32 NIndex = 0; NIndex < Adj->Count; NIndex++)
            {
                sml_u32 N = Adj->V[NIndex];
                if(Visited.Values[N]) continue;

                auto &AdjacentNormal = List->Data[N].Normal;
                if(SmlVec3_Dot(CurrentNormal, AdjacentNormal) >= CosThresold)
                {
                    Visited.Values[N] = true;
                    Stack.Push(N);
                }
            }
        }
    }

}
