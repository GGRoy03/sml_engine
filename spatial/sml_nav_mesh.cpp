// ===================================
// Type Definitions
// ===================================

struct sml_nav_poly
{
    sml_dynamic_array<sml_vector3> Verts;
};

// ===================================
// Internal Helpers
// ===================================

// WARN: 
// 1) Leaks a lot of memory!

static sml_dynamic_array<sml_nav_poly>
SmlInt_BuildNavPolygons(sml_dynamic_array<sml_dynamic_array<sml_u32>> &Clusters,
                        sml_walkable_list *List)
{
    auto NavPolygons  = sml_dynamic_array<sml_nav_poly>(Clusters.Count);
    auto Boundary     = sml_dynamic_array<sml_tri_edge>(0);
    auto LoopVertices = sml_dynamic_array<sml_point>(0);

    for(sml_u32 ClusterIdx = 0; ClusterIdx < Clusters.Count; ClusterIdx++)
    {
        auto Cluster  = Clusters[ClusterIdx];

        for(sml_u32 TriIdx = 0; TriIdx < Cluster.Count; TriIdx++)
        {
            sml_neighbor_tris Neighbors = List->Neighbors[TriIdx];

            if(Neighbors.Count < 3)
            {
                sml_walkable_tri Tri = List->Walkable[TriIdx];

                for(sml_u32 EdgeIdx = 0; EdgeIdx < 3; EdgeIdx++)
                {
                    sml_tri_edge Edge = SmlInt_MakeEdgeKey(EdgeIdx, Tri.Points);

                    auto &Triangles = List->EdgeToTris.Get(Edge);
                    if(Triangles.Count == 1)
                    {
                        Boundary.Push(Edge);
                    }
                }
            }
        }

        Sml_Assert(Boundary.Count > 0);

        auto CurrentPoint  = Boundary[0].Point0;
        auto CurrentCorner = Boundary[0].Point1;

        LoopVertices.Push(CurrentPoint);
        LoopVertices.Push(CurrentCorner);

        while(true)
        {
            sml_point NextPoint = {};
            bool      Found     = false;

            for(sml_u32 Idx = 0; Idx < Boundary.Count; Idx++)
            {
                auto Edge = Boundary[Idx];

                if(Edge.Point0 == CurrentPoint && Edge.Point1 == CurrentCorner)
                {
                    continue;
                }

                if(Edge.Point0 == CurrentCorner || Edge.Point1 == CurrentCorner)
                {
                    NextPoint = (Edge.Point0 == CurrentCorner) ? Edge.Point1 :
                                                                 Edge.Point0;
                    Found = true;
                    break;
                }
            }

            if(!Found)
            {
                Sml_Assert(!"Should not happen!");
                break;
            }

            if(NextPoint == LoopVertices[0])
            {
                break;
            }

            LoopVertices.Push(NextPoint);

            CurrentPoint  = CurrentCorner;
            CurrentCorner = NextPoint;
        }

        sml_nav_poly NavPoly = {};
        NavPoly.Verts = sml_dynamic_array<sml_vector3>(LoopVertices.Count);

        for(sml_u32 LoopIdx = 0; LoopIdx < LoopVertices.Count; LoopIdx++)
        {
            sml_point PointIdx = LoopVertices[LoopIdx];
            NavPoly.Verts.Push(List->Positions->Values[PointIdx]);
        }

        NavPolygons.Push(NavPoly);

        LoopVertices.Reset();
        Boundary.Reset();
    }

    LoopVertices.Free();
    Boundary.Free();

    return NavPolygons;
}

static sml_instance
SmlInt_CreateDebugInstance(sml_dynamic_array<sml_nav_poly> &NavPolygons)
{
    auto DebugVerts   = sml_dynamic_array<sml_vertex>(0);
    auto DebugIndices = sml_dynamic_array<sml_u32>(0);

    for(sml_u32 PolyIdx = 0; PolyIdx < NavPolygons.Count; PolyIdx++)
    { 
        auto   *Poly = NavPolygons.Values + PolyIdx;
        sml_u32 Base = DebugVerts.Count;

        for(sml_u32 VtxIdx = 0; VtxIdx < Poly->Verts.Count; VtxIdx++)
        {
            sml_vertex Vertex;
            Vertex.Position = Poly->Verts[VtxIdx];
            Vertex.Normal   = sml_vector3(0.0f, 0.0f, 0.0f);
            Vertex.UV       = sml_vector2(0.0f, 0.0f);

            DebugVerts.Push(Vertex);
        }

        auto Poly2D = sml_dynamic_array<sml_vector2>(Poly->Verts.Count);
        for(sml_u32 VtxIdx = 0; VtxIdx < Poly->Verts.Count; VtxIdx++)
        {
            auto Pos2D = sml_vector2(Poly->Verts[VtxIdx].x, Poly->Verts[VtxIdx].z);
            Poly2D.Push(Pos2D);
        }

        auto Indices = SmlInt_Triangulate(Poly2D, SmlTriangulate_Fan);
        for(sml_u32 Idx = 0; Idx < Indices.Count; Idx++)
        {
            DebugIndices.Push(Base + Indices[Idx]);
        }

        Poly2D.Free();
    }

    sml_mesh DebugMesh = {};
    DebugMesh.VertexData     = DebugVerts.Values;
    DebugMesh.VertexDataSize = DebugVerts.Count * sizeof(sml_vertex);
    DebugMesh.IndexData      = DebugIndices.Values;
    DebugMesh.IndexDataSize  = DebugIndices.Count * sizeof(sml_u32);

    sml_u32      Material      = Sml_SetupMaterial(nullptr, 0, 0, 0);
    sml_instance DebugInstance = Sml_SetupInstance(&DebugMesh, Material);

    DebugVerts.Free();
    DebugIndices.Free();

    return DebugInstance;
}

// ===================================
// User API
// ===================================

// WARN:
// 1) This leaks quite a large amount of data (List, Clusters)
// 2) Code is really ugly

static sml_dynamic_array<sml_nav_poly>
Sml_BuildNavMesh(sml_dynamic_array<sml_vector3> *Points, 
                 sml_dynamic_array<sml_u32> *Indices, sml_f32 SlopeDegree)
{
    sml_walkable_list List = SmlInt_BuildWalkableList(Points, Indices, SlopeDegree);

    sml_dynamic_array<sml_dynamic_array<sml_tri>> 
    Clusters = SmlInt_BuildPolygonClusters(&List);

    sml_dynamic_array<sml_nav_poly> 
    NavPolygons = SmlInt_BuildNavPolygons(Clusters, &List);

    return NavPolygons;
}
