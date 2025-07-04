#include "math.h" // cosf

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

    bool operator==(const sml_edge_key& O) const
    {
        return EdgeA == O.EdgeA && EdgeB == O.EdgeB;
    }
};

struct sml_adjacent_tris
{
    sml_u32 V[3];
    sml_u32 Count;
};

struct sml_walkable_list
{
    sml_dynamic_array<sml_walkable_tri> 
    Walkable;

    sml_hashmap<sml_edge_key, sml_dynamic_array<sml_u32>> 
    EdgeToTri;
};

static sml_walkable_list
SmlInt_ExtractWalkableList(sml_mesh *Mesh, sml_f32 MaxSlopeDegree)
{
    sml_walkable_list List = {};

    sml_u32 IdxCnt = sml_u32(Mesh->IndexDataSize / sizeof(sml_u32));
    sml_u32 TriCnt = IdxCnt / 3;

    List.Walkable = sml_dynamic_array<sml_walkable_tri>(TriCnt);

    sml_f32     SlopeThresold = cosf(MaxSlopeDegree * (3.14158265f / 180.0f));
    sml_vertex *VertexData    = (sml_vertex*)Mesh->VertexData;

    for(sml_u32 TriIdx = 0; TriIdx < TriCnt; TriIdx++)
    {
        sml_walkable_tri Tri  = {};
        sml_u32          Base = TriIdx * 3;

        Tri.Indices[0] = Mesh->IndexData[Base + 0];
        Tri.Indices[1] = Mesh->IndexData[Base + 1];
        Tri.Indices[2] = Mesh->IndexData[Base + 2];

        Tri.v0 = VertexData[Tri.Indices[0]].Position;
        Tri.v1 = VertexData[Tri.Indices[1]].Position;
        Tri.v2 = VertexData[Tri.Indices[2]].Position;

        sml_vector3 Edge0  = Tri.v1 - Tri.v0;
        sml_vector3 Edge1  = Tri.v2 - Tri.v0;
        sml_vector3 Normal = SmlVec3_Normalize(SmlVec3_VectorProduct(Edge0, Edge1));

        Tri.Normal = Normal;

        if(Normal.y >= SlopeThresold)
        {
            List.Walkable.Push(Tri);
        }
    }

    sml_u32 EdgeCnt = List.Walkable.Count * 3;
    List.EdgeToTri  = sml_hashmap<sml_edge_key, sml_dynamic_array<sml_u32>>(EdgeCnt);

    return List;
}

static void
SmlInt_BuildTriangleAdjency(sml_walkable_list *List)
{
    for(sml_u32 TriIndex = 0; TriIndex < List->Walkable.Count; TriIndex++)
    {
        sml_walkable_tri *Tri = List->Walkable.Values + TriIndex;

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

            auto Array = List->EdgeToTri.Get(Key);
            Array.Push(TriIndex);
        }
    }
}

static void
SmlInt_ClusterCoplanarPatches(sml_walkable_list *List)
{
    const sml_f32 MaxPlanarAngle = 10.0f;
    const sml_f32 CosThresold    = (MaxPlanarAngle * (3.14159265f/180.0f));

    sml_dynamic_array<sml_adjacent_tris> Adjacents(List->Walkable.Count);

    for (sml_u32 TriIdx = 0; TriIdx < List->Walkable.Count; ++TriIdx)
    {
        auto *Tri = List->Walkable.Values + TriIdx;
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

            auto Shared = List->EdgeToTri.Get(Key);
            if (Shared.Count == 2)
            {
                Adj->V[Adj->Count++] = (Shared.Values[0] == TriIdx ? 
                                                       Shared.Values[1] :
                                                       Shared.Values[0]);
            }
        }
    }

    sml_dynamic_array<bool> 
    Visited(List->Walkable.Count);

    sml_dynamic_array<sml_dynamic_array<sml_u32>>
    Clusters(0);

    for(sml_u32 TriIndex = 0; TriIndex < List->Walkable.Count; TriIndex++)
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
            auto  CurrentNormal = List->Walkable[Current].Normal;

            for(sml_u32 NIndex = 0; NIndex < Adj->Count; NIndex++)
            {
                sml_u32 N = Adj->V[NIndex];
                if(Visited.Values[N]) continue;

                auto AdjacentNormal = List->Walkable[N].Normal;
                if(SmlVec3_Dot(CurrentNormal, AdjacentNormal) >= CosThresold)
                {
                    Visited.Values[N] = true;
                    Stack.Push(N);
                }
            }
        }
    }
}
