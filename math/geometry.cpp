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

};

// ===================================
// Internal Helpers
// ===================================

static inline sml_edge
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

static sml_walkable_list
SmlInt_BuildWalkableList(sml_dynamic_array<sml_vector3> *Positions,
                         sml_dynamic_array<sml_u32>     *Indices,
                         sml_f32                         SlopeDeg)
{
    sml_walkable_list List = {};
    List.Positions = Positions;
    List.Indices   = Indices;

    sml_u32 TriCount = Indices.Count / 3;
    List.Walkable    = sml_dynamic_array<sml_walkable_tri2>(TriCount);

    sml_f32 SlopeThresold = cosf(SlopeDeg * (3.14158265f / 180.0f));

    for(sml_u32 TriIdx = 0; TriIdx < TriCount; TriIdx++)
    {
        sml_walkable_tri2 Tri  = {};
        sml_u32           Base = TriIdx * 3;

        Tri.Points[0] = Indices[Base + 0];
        Tri.Points[1] = Indices[Base + 1];
        Tri.Points[2] = Indices[Base + 2];

        Tri.v0 = Positions->Values[Tri.Points[0]];
        Tri.v1 = Positions->Values[Tri.Points[1]];
        Tri.v2 = Positions->Values[Tri.Points[2]];

        sml_vector3 EdgeVec0 = Tri.v1 - Tri.v0;
        sml_vector3 EdgeVec1 = Tri.v2 - Tri.v0;

        Tri.Normal = SmlVec3_Normalize(SmlVec3_VectorProduct(EdgeVec0, EdgeVec1));

        if(Tri.Normal.y > SlopeThresold)
        {
            List.Walkable.Push(Tri);
        }
    }

    sml_u32 EdgeCnt = List.Walkable.Count * 3;
    List.EdgeToTris = sml_hashmap<sml_tri_edge, sml_edge_tris>(EdgeCnt);

    for(sml_u32 TriIdx = 0; TriIdx < List.Walkable.Count; TriIdx++)
    {
        sml_walkable_tri Tri = List->Walkable[TriIdx];

        for(sml_u32 EdgeIdx = 0; EdgeIdx < 3; EdgeIdx++)
        {
            sml_tri_edge Edge = SmlInt_MakeEdgeKey(EdgeIdx, Tri.Points);

            auto &Triangles = List->EdgeToTris.Get(Edge);
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

            auto &Triangles = List->EdgeToTri.Get(Key);
            if (Triangles.Count == 2)
            {
                sml_tri SharedTri = 
                    (Triangles.Values[0] == TriIdx ? Triangles.Values[1] :
                                                     Triangles.Values[0]);
                Neighbors.Tris[Neighbors.Count++] = SharedTri;
            }
        }
    }

    return List;
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
