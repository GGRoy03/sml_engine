// ===================================
// Type Definitions
// ===================================

struct sml_nav_poly
{
    sml_dynamic_array<sml_vector3> Verts;
};

struct sml_navmesh_debug_manager
{
    sml_mesh_record *ActiveMesh;

    bool IsInitialized;
};

// ===================================
// Internal Helpers
// ===================================


static sml_dynamic_array<sml_nav_poly>
Sml_BuildNavMesh(sml_vector3 *Points,
                 sml_u32     *Indices,
                 sml_u32      IdxCount,
                 sml_f32      SlopeDegree);

static sml_instance
SmlInt_CreateNavMeshDebugInstance(sml_nav_poly *NavPolygons,
                                  sml_u32       Count);

static void
SmlDebug_FormatToByteUnits(sml_f64 NumBytes, char *OutBuffer, sml_u32 OutBufferSize)
{
    char    Unit[16] = {};
    sml_f64 Display  = NumBytes;

    if (Display > Sml_Megabytes(1))
    {
        Display /= Sml_Megabytes(1);
        memcpy(Unit, "Megabytes", 10);
    }
    else if (Display > Sml_Kilobytes(1))
    {
        Display /= Sml_Kilobytes(1);
        memcpy(Unit, "Kylobytes", 10);
    }
    else
    {
        memcpy(Unit, "Bytes", 6);
    }

    snprintf(OutBuffer, OutBufferSize, "%.2f %s", Display, Unit);
}


static void 
SmlDebug_NavMeshUI()
{
    static sml_navmesh_debug_manager Manager;

    if (!Manager.IsInitialized)
    {
        Manager.ActiveMesh = nullptr;

        Manager.IsInitialized = true;
    }

    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg      , IM_COL32(20, 20, 20, 255));
    ImGui::PushStyleColor(ImGuiCol_Header        , IM_COL32(255, 140, 0, 200));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered , IM_COL32(255, 160, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBg       , IM_COL32(30, 30, 30, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(255, 140, 0, 100));
    ImGui::PushStyleColor(ImGuiCol_Text          , IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Border        , IM_COL32(255, 140, 0, 100));
    ImGui::PushStyleColor(ImGuiCol_Separator     , IM_COL32(255, 140, 0, 100));

    ImGui::Begin("Nav‑Mesh Builder##NavMeshUI", nullptr, Flags);

    float LeftWidth = 250.0f;
    ImGui::BeginChild("##MeshListPanel", ImVec2(LeftWidth, 0), true);
    ImGui::Text("Meshes");
    ImGui::Separator();

    ImGui::BeginChild("##MeshListScroll", ImVec2(0, 275), false);
    u32 Idx = SmlMeshes.Head;
    while (Idx != sml_slot_map<sml_mesh_record, u32>::Invalid)
    {
        auto* Rec = SmlMeshes.Data + Idx;
        if (ImGui::Selectable(Rec->Name, false))
        {
            Manager.ActiveMesh = SmlMeshes.Data + Idx;
        }
        Idx = SmlMeshes.Next[Idx];
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Mesh Metadata");
    ImGui::BeginChild("##MeshMetaLeft", ImVec2(0, 0), false);

    ImGuiTableFlags MetaDataFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if (Manager.ActiveMesh && ImGui::BeginTable("MeshMetadata", 2, MetaDataFlags))
    {
        auto *Act = Manager.ActiveMesh;

        ImGui::TableSetupColumn("Property",ImGuiTableColumnFlags_WidthFixed, 125.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
            ImGui::Text("Name");
        ImGui::TableSetColumnIndex(1);
            ImGui::Text(Act->Name);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
            ImGui::Text("Vertex Data Size");
        ImGui::TableSetColumnIndex(1);
            char VtxByte[32] = {};
            SmlDebug_FormatToByteUnits(sml_f64(Act->VtxHeap.Size), VtxByte, 32);
            ImGui::Text(VtxByte);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
            ImGui::Text("Index  Data Size");
        ImGui::TableSetColumnIndex(1);
            char IdxByte[32] = {};
            SmlDebug_FormatToByteUnits(sml_f64(Act->IdxHeap.Size), IdxByte, 32);
            ImGui::Text(IdxByte);

        ImGui::EndTable();
    }

    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##GroupAndMetaPanel", ImVec2(0, 0), true);

    ImGui::Text("Groups");
    ImGui::BeginChild("##GroupBuilder", ImVec2(0, 200), true);
    ImGui::TextWrapped("Drag meshes here to form a group");
    ImGui::EndChild();

    ImGui::Spacing();


    ImGui::Text("Group Metadata");
    ImGui::Separator();
    ImGui::BeginChild("##GroupMeta", ImVec2(0, 0), false);
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor(8);
}


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
            NavPoly.Verts.Push(List->Positions[PointIdx]);
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
SmlInt_CreateNavMeshDebugInstance(sml_nav_poly *NavPolygons, sml_u32 Count)
{
    auto DebugVtx = sml_dynamic_array<sml_vertex_color>(0);
    auto DebugIdx = sml_dynamic_array<sml_u32>(0);

    for(sml_u32 PolyIdx = 0; PolyIdx < Count; PolyIdx++)
    { 
        auto   *Poly = NavPolygons + PolyIdx;
        sml_u32 Base = DebugVtx.Count;

        for(sml_u32 VtxIdx = 0; VtxIdx < Poly->Verts.Count; VtxIdx++)
        {
            sml_vertex_color Vertex;
            Vertex.Position = Poly->Verts[VtxIdx];
            Vertex.Normal   = sml_vector3(0.0f, 0.0f, 0.0f);
            Vertex.Color    = sml_vector3(0.0f, 1.0f, 0.0f);

            DebugVtx.Push(Vertex);
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
            DebugIdx.Push(Base + Indices[Idx]);
        }

        Poly2D.Free();
    }

    auto DebugMesh = 
        sml_mesh<sml_vertex_color, sml_u32>(DebugVtx.Count, DebugIdx.Count);

    memcpy(DebugMesh.VtxData, DebugVtx.Values, DebugMesh.VtxHeap.Size);
    memcpy(DebugMesh.IdxData, DebugIdx.Values, DebugMesh.IdxHeap.Size);

    sml_u32 Material = Sml_SetupMaterial(nullptr, 0, SmlShaderFeat_Color, 0);
    sml_instance DebugInstance
        = Sml_SetupInstance(DebugMesh.VtxHeap, DebugMesh.IdxHeap,
                            DebugMesh.IndexCount(), Material,
                            SmlCommand_InstanceFreeHeap);

    Sml_UpdateInstance(sml_vector3(0.0f, 0.0f, 0.0f), DebugInstance); // NOTE: Position?

    return DebugInstance;
}

// ===================================
// User API
// ===================================

// WARN:
// 1) This leaks quite a large amount of data (List, Clusters)
// 2) Code is really ugly

static sml_dynamic_array<sml_nav_poly>
Sml_BuildNavMesh(sml_vector3 *Points, sml_u32 *Indices, sml_u32 IdxCount,
                 sml_f32 SlopeDegree)
{
    sml_walkable_list List = SmlInt_BuildWalkableList(Points, Indices, IdxCount,
                                                      SlopeDegree);

    sml_dynamic_array<sml_dynamic_array<sml_tri>> 
    Clusters = SmlInt_BuildPolygonClusters(&List);

    sml_dynamic_array<sml_nav_poly> 
    NavPolygons = SmlInt_BuildNavPolygons(Clusters, &List);

    return NavPolygons;
}
