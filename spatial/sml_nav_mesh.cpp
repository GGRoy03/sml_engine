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
    auto NavPolygons = sml_dynamic_array<sml_nav_poly>(Clusters.Count);

    for(sml_u32 ClusterIdx = 0; ClusterIdx < Clusters.Count; ClusterIdx++)
    {
        auto Boundary = sml_dynamic_array<sml_tri_edge>(0);
        auto Cluster  = Clusters[ClusterIdx];

        for(sml_u32 TriIdx = 0; TriIdx < Cluster.Count; TriIdx++)
        {
            sml_neighbor_tris Neighbors = List->Neighbors[TriIdx];

            if(Neighbors.Count < 3)
            {
                sml_walkalbe_tri Tri = List->Walkable[TriIdx];

                for(sml_u32 EdgeIdx = 0; EdgeIdx < 3; EdgeIdx++)
                {
                    sml_tri_edge Edge = SmlInt_MakeEdgeKey(EdgeIdx, Tri.Points);

                    auto &Triangles = List->EdteToTris.Get(Edge);
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

        auto LoopVertices = sml_dynamic_array<sml_point>(0);
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
        NavPoly.Verts = sml_dynamic_array<sml_vector3>(Loop.Vertices.Count);

        for(sml_u32 LoopIdx = 0; LoopIdx < Loop.Vertices.Count; LoopIdx++)
        {
            sml_point PointIdx = LoopVertices[LoopVtxIdx];
            NavPoly.Vertx.Push(List->Positions[PointIdx]);
        }

        NavPolygons.Push(NavPoly);
    }

    return NavPolygons;
}
