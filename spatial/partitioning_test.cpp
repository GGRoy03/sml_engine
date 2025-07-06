#include "math.h" // cosf

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
    const sml_f32 CosThresold    = cosf(MaxPlanarAngle * (3.14159265f/180.0f));

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
        auto Boundary = sml_dynamic_array<sml_tri_edge>(0);

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
                        auto *VtxPtr = (sml_vertex*)List->Mesh->VertexData;

                        sml_tri_edge TriEdge;
                        TriEdge.EdgeA = A;
                        TriEdge.EdgeB = B;
                        TriEdge.PosA  = VtxPtr[TriEdge.EdgeA].Position;
                        TriEdge.PosB  = VtxPtr[TriEdge.EdgeB].Position;

                        Boundary.Push(TriEdge);
                    }
                }
            }
        }

        Sml_Assert(Boundary.Count > 0);

        auto CurrentKey    = Boundary[0].EdgeA;
        auto CurrentCorner = Boundary[0].EdgeB;

        sml_dynamic_array LoopVertices = sml_dynamic_array<sml_u32>(0);
        LoopVertices.Push(CurrentKey);
        LoopVertices.Push(CurrentCorner);

        // NOTE: I have to rework the way I build the loop vertices.

        auto  Poly2D = sml_dynamic_array<sml_vector2>(LoopVertices.Count);
        auto *VtxPtr = (sml_vertex*)List->Mesh->VertexData;
        for(sml_u32 Idx = 0; Idx < Poly2D.Capacity; Idx++)
        {
            sml_u32 VtxIdx = LoopVertices[Idx];
            sml_vector2 Position = sml_vector2(VtxPtr[VtxIdx].Position.x,
                                               VtxPtr[VtxIdx].Position.z);
            Poly2D.Push(Position);
        }

        auto IdxList = SmlInt_Triangulate(Poly2D, SmlTriangulate_Fan);

        for(sml_u32 Idx = 0; Idx < IdxList.Count; Idx += 3)
        {
            sml_u32 I0 = IdxList[Idx];
            sml_u32 I1 = IdxList[Idx + 1];
            sml_u32 I2 = IdxList[Idx + 2];

            sml_vector3 P0 = VtxPtr[LoopVertices[I0]].Position;
            sml_vector3 P1 = VtxPtr[LoopVertices[I1]].Position;
            sml_vector3 P2 = VtxPtr[LoopVertices[I2]].Position;
        }

        Boundary.Free();
    }
}
