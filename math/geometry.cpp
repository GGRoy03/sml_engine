//////////////////////////////////////////////////////
///                  2D GEOMETRY                   ///
//////////////////////////////////////////////////////

// ===================================
// Type Definitions
// ===================================

enum SmlTriangulate_Method
{
    SmlTriangulate_Fan,
};

using sml_tri   = sml_u32;
using sml_edge  = sml_u32;
using sml_point = sml_u32;

struct sml_walkable_tri
{
    sml_vector3 v0, v1, v2;
    sml_point   Points[3];
    sml_vector3 Normal;
};

struct sml_tri_edge
{
    sml_point Point0, Point1;

    bool operator==(const sml_tri_edge &Edge) const noexcept
    {
        return (Edge.Point0 == Point0 && Edge.Point1 == Point1);
    }
};

struct sml_edge_tris
{
    sml_tri Tris[2];
    sml_u32 Count;
};

struct sml_neighbor_tris
{
    sml_tri Tris[3];
    sml_u32 Count;
};

struct sml_walkable_list
{
    // List of triangles considered walkable
    sml_dynamic_array<sml_walkable_tri> Walkable;

    // An array of neighbor triangles for every walkable triangle
    sml_dynamic_array<sml_neighbor_tris> Neighbors;

    // Base mesh data
    sml_dynamic_array<sml_vector3> *Positions;
    sml_dynamic_array<sml_u32>     *Indices;

    // Map of an edge to a list of triangles that have this mesh
    sml_hashmap<sml_tri_edge, sml_edge_tris> EdgeToTris;

    // Other meta-data
    sml_f32 SlopeThresold;
};

// ===================================
// Internal Helpers
// ===================================

static inline sml_tri_edge
SmlInt_MakeEdgeKey(sml_u32 EdgeIdx, sml_point *Points)
{
    sml_tri_edge Edge;

    Edge.Point0 = Points[EdgeIdx];
    Edge.Point1 = Points[(EdgeIdx + 1) % 3];

    if(Edge.Point0 > Edge.Point1)
    {
        sml_u32 Temp = Edge.Point0;
        Edge.Point0  = Edge.Point1;
        Edge.Point1  = Temp;
    }

    return Edge;
}

static sml_f32
SmlInt_SignedArea(sml_vector2 *A, sml_vector2 *B, sml_vector2 *C)
{
    sml_f32 SignedArea = (B->x - A->x) * (C->y - A->y) -
                         (B->y - A->y) * (C->x - A->x);

    return SignedArea;
}

static sml_walkable_list
SmlInt_BuildWalkableList(sml_dynamic_array<sml_vector3> *Positions,
                         sml_dynamic_array<sml_u32>     *Indices,
                         sml_f32                         SlopeDeg)
{
    sml_walkable_list List = {};
    List.Positions = Positions;
    List.Indices   = Indices;

    sml_u32 TriCount = Indices->Count / 3;
    List.Walkable    = sml_dynamic_array<sml_walkable_tri>(TriCount);

    List.SlopeThresold = cosf(SlopeDeg * (3.14158265f / 180.0f));

    for(sml_u32 TriIdx = 0; TriIdx < TriCount; TriIdx++)
    {
        sml_walkable_tri Tri  = {};
        sml_u32          Base = TriIdx * 3;

        Tri.Points[0] = Indices->Values[Base + 0];
        Tri.Points[1] = Indices->Values[Base + 1];
        Tri.Points[2] = Indices->Values[Base + 2];

        Tri.v0 = Positions->Values[Tri.Points[0]];
        Tri.v1 = Positions->Values[Tri.Points[1]];
        Tri.v2 = Positions->Values[Tri.Points[2]];

        sml_vector3 EdgeVec0 = Tri.v1 - Tri.v0;
        sml_vector3 EdgeVec1 = Tri.v2 - Tri.v0;

        Tri.Normal = SmlVec3_Normalize(SmlVec3_VectorProduct(EdgeVec0, EdgeVec1));

        if(Tri.Normal.y > List.SlopeThresold)
        {
            List.Walkable.Push(Tri);
        }
    }

    sml_u32 EdgeCnt = List.Walkable.Count * 3;
    List.EdgeToTris = sml_hashmap<sml_tri_edge, sml_edge_tris>(EdgeCnt);

    for(sml_u32 TriIdx = 0; TriIdx < List.Walkable.Count; TriIdx++)
    {
        sml_walkable_tri Tri = List.Walkable[TriIdx];

        for(sml_u32 EdgeIdx = 0; EdgeIdx < 3; EdgeIdx++)
        {
            sml_tri_edge Edge = SmlInt_MakeEdgeKey(EdgeIdx, Tri.Points);

            auto &Triangles = List.EdgeToTris.Get(Edge);
            Triangles.Tris[Triangles.Count++] = TriIdx;
        }
    }

    List.Neighbors = sml_dynamic_array<sml_neighbor_tris>(List.Walkable.Count);

    for (sml_u32 TriIdx = 0; TriIdx < List.Walkable.Count; ++TriIdx)
    {
        sml_walkable_tri Tri = List.Walkable[TriIdx];

        sml_neighbor_tris Neighbors = {};

        for (sml_u32 EdgeIdx = 0; EdgeIdx < 3; EdgeIdx++)
        {
            sml_tri_edge Edge = SmlInt_MakeEdgeKey(EdgeIdx, Tri.Points);

            auto &Triangles = List.EdgeToTris.Get(Edge);
            if (Triangles.Count == 2)
            {
                sml_tri SharedTri = 
                    (Triangles.Tris[0] == TriIdx ? Triangles.Tris[1] :
                                                   Triangles.Tris[0]);
                Neighbors.Tris[Neighbors.Count++] = SharedTri;
            }
        }

        List.Neighbors.Push(Neighbors);
    }

    return List;
}

static sml_dynamic_array<sml_dynamic_array<sml_tri>>
SmlInt_BuildPolygonClusters(sml_walkable_list *List)
{
    auto Visited  = sml_dynamic_array<bool>(List->Walkable.Count);
    auto Clusters = sml_dynamic_array<sml_dynamic_array<sml_tri>>(0);
    auto Stack    = sml_dynamic_array<sml_tri>(0);

    for(sml_u32 TriIdx = 0; TriIdx < List->Walkable.Count; TriIdx++)
    {
        if(Visited[TriIdx]) continue;

        auto Cluster = sml_dynamic_array<sml_tri>(0);

        Stack.Push(TriIdx);
        Visited[TriIdx] = true;

        while(Stack.Count > 0)
        {
            sml_tri Current = Stack.Pop();
            Cluster.Push(Current);

            auto Neighbors = List->Neighbors[Current];
            auto Normal    = List->Walkable[Current].Normal;

            for(sml_u32 NIdx = 0; NIdx < Neighbors.Count; NIdx++)
            {
                sml_tri Neighbor = Neighbors.Tris[NIdx];

                if(Visited[Neighbor]) continue;

                auto NeighborNormal = List->Walkable[Neighbor].Normal;
                if(SmlVec3_Dot(Normal, NeighborNormal) >= List->SlopeThresold)
                {
                    Visited[Neighbor] = true;
                    Stack.Push(Neighbor);
                }
            }
        }
    }

    Visited.Free();
    Stack.Free();

    return Clusters;
}

// WARN:
// 1) Is there a way to estime the amount of indices needed given a Polygon count?

static sml_dynamic_array<sml_u32>
SmlInt_Triangulate(sml_dynamic_array<sml_vector2> Polygon, 
                   SmlTriangulate_Method Method)
{
    auto IdxList = sml_dynamic_array<sml_u32>(0);

    switch(Method)
    {

    case SmlTriangulate_Fan:
    {
        for(sml_u32 Idx = 1; Idx + 1 < Polygon.Count; Idx++)
        {
            IdxList.Push(0);
            IdxList.Push(Idx);
            IdxList.Push(Idx + 1);
        }
    } break;

    default:
        Sml_Assert(!"Invalid triangulation method.");
        break;

    }

    Sml_Assert(IdxList.Count % 3 == 0);

    return IdxList;
}
