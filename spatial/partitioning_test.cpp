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
    sml_mesh *Mesh;

    sml_dynamic_array<sml_walkable_tri> 
    Walkable;

    sml_hashmap<sml_edge_key, sml_dynamic_array<sml_u32>> 
    EdgeToTri;
};

static sml_walkable_list
SmlInt_ExtractWalkableList(sml_mesh *Mesh, sml_f32 MaxSlopeDegree)
{
    sml_walkable_list List = {};
    List.Mesh = Mesh;

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

            auto &Array = List->EdgeToTri.Get(Key);
            if (!Array.Values)
            {
                Array = sml_dynamic_array<sml_u32>(3);
            }
            Array.Push(TriIndex);
        }
    }
}

static sml_f32
SmlInt_SignedArea(sml_vector2 *A, sml_vector2 *B, sml_vector2 *C)
{
    sml_f32 SignedArea = (B->x - A->x) * (C->y - A->y) -
                         (B->y - A->y) * (C->x - A->x);

    return SignedArea;
}

// WARN:
// 1) This is a big prototype hence all of the leaks.

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

        auto Stack   = sml_dynamic_array<sml_u32>(0);
        auto Cluster = sml_dynamic_array<sml_u32>(0);

        Stack.Push(TriIndex);
        Visited.Values[TriIndex] = true;

        while(Stack.Count > 0)
        {
            sml_u32 Current = Stack.Pop();

            Cluster.Push(Current);

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

        Clusters.Push(Cluster);
    }

    // NOTE: Extract the boundaries

    for(sml_u32 ClusterIdx = 0; ClusterIdx < Clusters.Count; ClusterIdx++)
    {
        auto Boundary = sml_dynamic_array<sml_edge_key>(0);

        sml_dynamic_array<sml_u32> *Cluster  = Clusters.Values + ClusterIdx;

        for(sml_u32 TriIdx = 0; TriIdx < Cluster->Count; TriIdx++)
        {
            sml_adjacent_tris *NeighborTris = Adjacents.Values + TriIdx;

            if(NeighborTris->Count < 3)
            {
                sml_walkable_tri *Tri = List->Walkable.Values + TriIdx;

                for(sml_u32 EdgeIdx = 0; EdgeIdx < 3; EdgeIdx++)
                {
                    sml_u32 A = Tri->Indices[EdgeIdx];
                    sml_u32 B = Tri->Indices[(EdgeIdx + 1) % 3];

                    if (A > B)
                    {
                        sml_u32 Temp = A;
                        A = B;
                        B = Temp;
                    }

                    sml_edge_key Key{A, B};
                    auto Array = List->EdgeToTri.Get(Key);

                    if(Array.Count == 1)
                    {
                        Boundary.Push(Key);
                    }
                }
            }
        }

        Sml_Assert(Boundary.Count > 0);

        auto CurrentKey    = Boundary[0];
        auto CurrentCorner = CurrentKey.EdgeB;

        sml_dynamic_array LoopVertices = sml_dynamic_array<sml_u32>(0);
        LoopVertices.Push(CurrentKey.EdgeA);
        LoopVertices.Push(CurrentKey.EdgeB);

        while(true)
        {
            sml_edge_key NextKey = {};
            bool         Found   = false;

            for (sml_u32 EdgeKeyIdx = 0; EdgeKeyIdx < Boundary.Count; EdgeKeyIdx++)
            {
                sml_edge_key Key = Boundary[EdgeKeyIdx];

                if (Key.EdgeA == CurrentKey.EdgeA && Key.EdgeB == CurrentKey.EdgeB)
                {
                    continue;
                }

                if (Key.EdgeA == CurrentCorner || Key.EdgeB == CurrentCorner)
                {
                    NextKey = Key;
                    Found   = true;
                    break;
                }

            }

            if(!Found) Sml_Assert(!"Not possible!");

            sml_u32 New = (NextKey.EdgeA == CurrentCorner) ? NextKey.EdgeB :
                                                             NextKey.EdgeA;
            if(New == LoopVertices[0]) break;

            LoopVertices.Push(New);

            CurrentKey    = NextKey;
            CurrentCorner = New;
        }

        // NOTE: To perform triangulation we need to project the triangles onto a
        // 2D space. We use X, Z harcoded for now, because we know the input shape.
        // We might need to do some sort to determine which plane the projection
        // should occur on before feeding it to the routine.

        auto  Poly2D = sml_dynamic_array<sml_vector2>(LoopVertices.Count);
        auto *VtxPtr = (sml_vertex*)List->Mesh->VertexData;
        for(sml_u32 Idx = 0; Idx < Poly2D.Capacity; Idx++)
        {
            sml_u32 VtxIdx = LoopVertices[Idx];
            sml_vector2 Position = sml_vector2(VtxPtr[VtxIdx].Position.x,
                                               VtxPtr[VtxIdx].Position.z);
            Poly2D.Push(Position);
        }

        // NOTE: We use the simplest case of triangulation, because we know we
        // are feeding convex shapes.

        auto IdxList = SmlInt_Triangulate(Poly2D, SmlTriangulate_Fan);

        Boundary.Free();
    }
}
